#include "flacAdapter.h"

namespace WaffleoMediaAdapter {

	const int FlacWriter::openWriter(const char* path) {
		if (isOpen) return -3;
		if (!path) return -1;

		encoder.set_sample_rate(info.sample_rate);
		encoder.set_bits_per_sample(info.bit_depth);
		encoder.set_channels(static_cast<uint32_t>(info.channels));
		encoder.set_total_samples_estimate(info.frame_count);
		encoder.set_compression_level(level);

		istat = encoder.init(path);
		if (istat != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
			//Fail
			encoder.finish();
			return -2;
		}

		isOpen = true;
		return 0;
	}

	const uint32_t FlacWriter::getBlockSize() const {
		return encoder.get_blocksize();
	}

	const int FlacWriter::writeSamples(void** data, int frames, bool pre_expanded) {
		if (!isOpen) return -1;

		//If pre-expanded, receiving samples already at 32 bits.
		//Otherwise, have to copy to new array, expanding from input bit depth
		if (pre_expanded) {
			FLAC__int32** fsamps = (FLAC__int32**)data;
			if (!encoder.process(fsamps, static_cast<uint32_t>(frames))) {
				//Get the error
				sstat = (FLAC__StreamEncoderState)encoder.get_state();
				return -2;
			}
		}
		else {
			//Need a new buffer.
			int32_t c, f, i;
			int32_t ch = static_cast<int32_t>(info.channels);
			if (ch < 1 || frames < 1) return -6;
		//--> malloc (fsamps)
			FLAC__int32** fsamps = (FLAC__int32**)malloc(sizeof(FLAC__int32*) * ch);
			if (!fsamps) { return -5; }
			for (c = 0; c < ch; c++) {
		//--> malloc (fsamps[c])
				fsamps[c] = (FLAC__int32*)malloc(static_cast<size_t>(frames) << 2);
				if (!fsamps[c]) {
					//Free what was malloc'd - looks like not enough memory
					if (c > 0) {
						int32_t d;
						for (d = c - 1; d >= 0; d--) free(fsamps[d]);
						free(fsamps);
					}
					return -5;
				}
			}

			//Expand & copy
			uint8_t** d8;
			int16_t** d16;
			uint8_t* chd8;
			int16_t* chd16;
			FLAC__int32* chout;
			switch (info.bit_depth) {
			case 8:
				d8 = (uint8_t**)data;
				for (c = 0; c < ch; c++) {
					chd8 = d8[c];
					chout = fsamps[c];
					for (f = 0; f < frames; f++) {
						FLAC__int32 samp = static_cast<FLAC__int32>(chd8[f]) & 0xFF;
						//samp -= 128;
						chout[f] = samp;
						//MSVC complains that this might cause a buffer overflow. I'm tagging that, but I already checked to make sure frames isn't 0
					}
				}
				break;
			case 16:
				d16 = (int16_t**)data;
				for (c = 0; c < ch; c++) {
					chd16 = d16[c];
					chout = fsamps[c];
					for (f = 0; f < frames; f++) {
						chout[f] = static_cast<FLAC__int32>(chd16[f]);
					}
				}
				break;
			case 24:
				d8 = (uint8_t**)data;
				for (c = 0; c < ch; c++) {
					chd8 = d8[c];
					chout = fsamps[c];
					for (f = 0; f < frames; f++) {
						FLAC__int32 samp = 0;
						for (i = 0; i < 3; i++) {
							samp |= static_cast<FLAC__int32>(*chd8++);
							samp >>= 8;
						}
						chout[f] = samp;
					}
				}
				break;
			default: return -3;
			}

			//Write
			if (!encoder.process(fsamps, static_cast<uint32_t>(frames))) {
				//Get the error
				sstat = (FLAC__StreamEncoderState)encoder.get_state();
				return -2;
			}

			//Free buffer
			for (c = 0; c < ch; c++) {
				free(fsamps[c]);
			}
			free(fsamps);
		}

		return 0;
	}

	const int FlacWriter::writeInterleavedSamples(void* data, int frames, bool pre_expanded) {
		if (!isOpen) return -1;

		if (pre_expanded) {
			FLAC__int32* fsamps = (FLAC__int32*)data;
			if (!encoder.process_interleaved(fsamps, static_cast<uint32_t>(frames))) {
				//Get the error
				sstat = (FLAC__StreamEncoderState)encoder.get_state();
				return -2;
			}
		}
		else {
			//Need a new buffer.
			uint32_t s, i;
			size_t scount;
			uint32_t ch = static_cast<uint32_t>(info.channels);
			scount = static_cast<size_t>(ch) * static_cast<size_t>(frames);
			if (scount < 1 || scount < ch) return -6;
			FLAC__int32* fsamps = (FLAC__int32*)malloc(scount << 2);
			if (!fsamps) return -5;

			//Expand & copy
			uint8_t* buff8;
			int16_t* buff16;
			switch (info.bit_depth) {
			case 8:
				buff8 = (uint8_t*)data;
				for (s = 0; s < scount; s++) {
					FLAC__int32 samp = static_cast<FLAC__int32>(buff8[s]) & 0xFF;
					//samp -= 128;
					fsamps[s] = samp;
				}
				break;
			case 16:
				buff16 = (int16_t*)data;
				for (s = 0; s < scount; s++) {
					fsamps[s] = static_cast<FLAC__int32>(buff16[s]);
				}
				break;
			case 24:
				buff8 = (uint8_t*)data;
				for (s = 0; s < scount; s++) {
					FLAC__int32 samp = 0;
					for (i = 0; i < 3; i++) {
						samp |= static_cast<FLAC__int32>(*buff8++);
						samp >>= 8;
					}
					fsamps[s] = samp;
				}
				break;
			default: return -3;
			}

			//Write
			if (!encoder.process_interleaved(fsamps, static_cast<uint32_t>(frames))) {
				//Get the error
				sstat = (FLAC__StreamEncoderState)encoder.get_state();
				return -2;
			}

			//Free buffer
			free(fsamps);
		}

		return 0;
	}

	const int FlacWriter::closeWriter() {
		if (!isOpen) return 0;
		encoder.finish();
		isOpen = false;
		return 0;
	}

}