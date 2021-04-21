
#include "matroskaAdapter.h"

using std::string;
using namespace LIBEBML_NAMESPACE;
using namespace LIBMATROSKA_NAMESPACE;

namespace WaffleoMediaAdapter {

	/*-------- MKV Writer --------*/

	const uint32_t MatroskaWriter::getDuration() const {
		//TODO
		return 0;
	}

	MkvVideoTrackInfo_t& MatroskaWriter::addVideoTrack(int enc) {
		vtrack_files.push_back(MkvVideoTrackInfo_t());
		MkvVideoTrackInfo_t tinfo = vtrack_files.back();
		tinfo.encoding = enc;

		//Assign track number and UID
		tinfo.track_no = (vtrack_files.size() + atrack_files.size());
		tinfo.track_uid = static_cast<uint32_t>(rand());

		return tinfo;
	}

	MkvAudioTrackInfo_t& MatroskaWriter::addAudioTrack(int enc) {
		atrack_files.push_back(MkvAudioTrackInfo_t());
		MkvAudioTrackInfo_t tinfo = atrack_files.back();
		tinfo.encoding = enc;

		//Assign track number and UID
		tinfo.track_no = (vtrack_files.size() + atrack_files.size());
		tinfo.track_uid = static_cast<uint32_t>(rand());

		return tinfo;
	}

	const int MatroskaWriter::writeVidTrackInfo(MkvVideoTrackInfo_t& tinfo, KaxTracks& tracks) {
		//TODO
		return 0;
	}

	const int MatroskaWriter::writeAudTrackInfo(MkvAudioTrackInfo_t& tinfo, KaxTracks& tracks) {
		//TODO
		KaxTrackEntry& atrack = GetChild<KaxTrackEntry>(tracks);
		atrack.SetGlobalTimecodeScale(STD_TIMESCALE);

		KaxTrackNumber& tno = GetChild<KaxTrackNumber>(atrack);
		*(static_cast<EbmlUInteger*>(&tno)) = tinfo.track_no;

		KaxTrackUID& tuid = GetChild<KaxTrackUID>(atrack);
		*(static_cast<EbmlUInteger*>(&tuid)) = tinfo.track_uid;

		*(static_cast<EbmlUInteger*>(&GetChild<KaxTrackType>(atrack))) = track_audio;

		KaxCodecID& codecid = GetChild<KaxCodecID>(atrack);
		string cid_str = "A_FLAC";
		switch (tinfo.encoding) {
		case AENCODING_VORBIS: cid_str = "A_VORBIS"; break;
		case AENCODING_PCM: cid_str = "A_PCM/INT/LIT";  break;
		}

		*static_cast<EbmlString*>(&codecid) = cid_str;

		//Name and language
		if (tinfo.name.length() > 0) {
			KaxTrackName& tname = GetChild<KaxTrackName>(atrack);
			*((EbmlUnicodeString*)&tname) = tinfo.name;
		}
		if (tinfo.lan.length() > 0) {
			KaxTrackLanguage& tlan = GetChild<KaxTrackLanguage>(atrack);
			*((EbmlString*)&tlan) = tinfo.lan;
		}

		//Codec private...
		if (tinfo.codec_data) {
			KaxCodecPrivate& codecdat = GetChild<KaxCodecPrivate>(atrack);
			((EbmlBinary*)&codecdat)->SetBuffer((binary*)tinfo.codec_data, tinfo.codec_data_size);
		}

		atrack.EnableLacing(true);

		return 0;
	}

	const int MatroskaWriter::beginMKVWrite(string& path) {
		//I cannot find good documentation from libmatroska, so this is from looking at their test6.cpp

		output = new StdIOCallback(path.c_str(), MODE_CREATE);

		//EBML Header
		EbmlHead FileHead;

		EDocType& MyDocType = GetChild<EDocType>(FileHead);
		*static_cast<EbmlString*>(&MyDocType) = "matroska";
		EDocTypeVersion& MyDocTypeVer = GetChild<EDocTypeVersion>(FileHead);
		*(static_cast<EbmlUInteger*>(&MyDocTypeVer)) = 4;
		EDocTypeReadVersion& MyDocTypeReadVer = GetChild<EDocTypeReadVersion>(FileHead);
		*(static_cast<EbmlUInteger*>(&MyDocTypeReadVer)) = 1;
		FileHead.Render(*output, false);

		//[SEGMENT] Spawn segment (the matroska element that contains all the data I think)
		segment = new KaxSegment();
		uint64 segsz = segment->WriteHead(*output, 5, false);
		KaxTracks& tracks = GetChild<KaxTracks>(*segment);

		//[SEEKHEAD init] So, I think this is for the segment SeekHead
		//I think we're reserving space here because we don't know the other element locations yet
		EbmlVoid vdummy;
		vdummy.SetSize(300);
		vdummy.Render(*output, false);
		metaseek = new KaxSeekHead();

		//[INFO] Segment Info
		KaxInfo& seginfo = GetChild<KaxInfo>(*segment);
		KaxTimecodeScale& timescale = GetChild<KaxTimecodeScale>(seginfo);
		*(static_cast<EbmlUInteger*>(&timescale)) = STD_TIMESCALE;

		KaxDuration& segdur = GetChild<KaxDuration>(seginfo);
		*(static_cast<EbmlFloat*>(&segdur)) = static_cast<double>(getDuration());

		*((EbmlUnicodeString*)&GetChild<KaxMuxingApp>(seginfo)) = L"libmatroska 1.6.2";
		*((EbmlUnicodeString*)&GetChild<KaxWritingApp>(seginfo)) = write_app;
		*((EbmlUnicodeString*)&GetChild<KaxTitle>(seginfo)) = title;
		GetChild<KaxWritingApp>(seginfo).SetDefaultSize(25);

		filepos_t infosz = seginfo.Render(*output);
		metaseek->IndexThis(seginfo, *segment);

		return 0; //Success
	}

	const int MatroskaWriter::writeTrackInfo() {
		//TODO
		return 0;
	}

	const size_t MatroskaWriter::writeBlock(void* data, size_t datalen, int trackType, int trackNo) {
		//TODO
		//TAKE TIMECODE!
		return 0;
	}

	const int MatroskaWriter::finishMKVWrite() {
		//TODO
		return 0;
	}

	/*-------- FLAC --------*/

	const int MkvFlacTrackWriter::initTrackWriter() {
		if (!tinfo) return 1; //Bad

		//Init buffer
		//Buffer holds two units.
		size_t usz = bytesPerUnit();
		w_buff_max_size = usz << 1;
		w_buffer = (ubyte*)malloc(w_buff_max_size);
		w_buff_size = 0;

		//Set FLAC params
		set_channels(tinfo->channels);
		set_bits_per_sample(tinfo->bitDepth);
		set_sample_rate(tinfo->sample_rate);
		set_compression_level(6);
		set_total_samples_estimate(tinfo->total_frames);

		//Generate header
		init_stat = init();
		if (init_stat != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
			free(w_buffer);
			return 1;
		}

		//I believe init() for the stream automatically writes the header. So that's handy.
		//Copy whatever's in the buffer to the codec private buffer in the tinfo
		if (w_buff_size < 1) return 1;
		tinfo->codec_data_size = static_cast<size_t>(w_buff_size);
		tinfo->codec_data = malloc(tinfo->codec_data_size);
		memcpy(tinfo->codec_data, w_buffer, tinfo->codec_data_size);

		isOpen = true;
		return 0;
	}

	const size_t MkvFlacTrackWriter::bytesPerUnit() const {
		if (!tinfo) return 0;

		//Frames per block * bytes per frame
		size_t sz = static_cast<size_t>(tinfo->channels);
		if (input_type == FLACINPUT_SAMP32_ARR) sz <<= 2;
		else sz *= (static_cast<size_t>(tinfo->bitDepth) >> 3);
		sz *= static_cast<size_t>(tinfo->frames_per_block);

		return sz;
	}

	const size_t MkvFlacTrackWriter::unitsPerBlock() const {
		return 1;
	}

	const size_t MkvFlacTrackWriter::blocksPerCluster() const {
		if (!tinfo) return 0;
		return tinfo->blocks_per_cluster;
	}

	const size_t MkvFlacTrackWriter::writeUnit(void* data) {

		if (!data || !tinfo) return 0;
		uint32_t i, j,k;
		uint32_t  fpb = tinfo->frames_per_block;
		size_t sz = bytesPerUnit();

		switch (input_type) {
		case FLACINPUT_PCM_LE:
			//Everything is going to have to be expanded anyway.
			FLAC__int32** charr = (FLAC__int32**)malloc(tinfo->channels * sizeof(FLAC__int32*));
			FLAC__int32* inbuff = (FLAC__int32*)malloc(sz);

			FLAC__int32* ptr = inbuff;
			for (i = 0; i < tinfo->channels; i++) {
				charr[i] = ptr;
				ptr += fpb;
			}

			uint32_t bytesPerSamp = static_cast<uint32_t>(tinfo->bitDepth) >> 3;

			ubyte* pos = (ubyte*)data;
			int shamt = 32 - tinfo->bitDepth;
			if (input_interleaved) {
				for (i = 0; i < fpb; i++) {
					//For each frame...
					for (j = 0; j < tinfo->channels; j++) {
						//For each channel...
						FLAC__int32 samp = 0;
						for (k = 0; k < bytesPerSamp; k++) {
							samp >>= 8;
							FLAC__int32 b = static_cast<FLAC__int32>(*pos++) << 24;
							samp |= b;
						}
						samp >>= shamt;
						*(charr[j] + i) = samp;
					}
				}
			}
			else {
				for (j = 0; j < tinfo->channels; j++) {
					//For each channel...
					for (i = 0; i < fpb; i++) {
						//For each frame...
						FLAC__int32 samp = 0;
						for (k = 0; k < bytesPerSamp; k++) {
							samp >>= 8;
							FLAC__int32 b = static_cast<FLAC__int32>(*pos++) << 24;
							samp |= b;
						}
						samp >>= shamt;
						*(charr[j] + i) = samp;
					}
				}
			}


			bool res = process(charr, fpb);
			free(inbuff);
			free(charr);
			if (!res) return 0;
			break;
		case FLACINPUT_SAMP32_ARR:
			if (input_interleaved) {
				//Should be able to cast and feed as-is
				FLAC__int32* input = (FLAC__int32*)data;
				bool res = process_interleaved(input, fpb);
				if (!res) return 0;
			}
			else {
				//Make ptr array for channel starts.
				FLAC__int32** charr = (FLAC__int32**)malloc(tinfo->channels * sizeof(FLAC__int32*));
				FLAC__int32* ptr = (FLAC__int32*)data;
				for (i = 0; i < tinfo->channels; i++) {
					charr[i] = ptr;
					ptr += fpb;
				}
				bool res = process(charr, fpb);
				free(charr); //whew almost forgot
				if (!res) return 0;
			}
			break;
		default: return 0; //Doesn't know what to do with it then, does it!
		}

		//now pass to the mkv writer...
		size_t res = target.writeBlock(w_buffer, w_buff_size, TRACKTYPE_AUDIO, atrack_idx);
		//Clear buffer
		w_buff_size = 0;
		spos_raw += fpb;
		return res;
	}

	FLAC__StreamEncoderWriteStatus MkvFlacTrackWriter::write_callback(const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame) {
		//Make sure writer is initialized
		if (!w_buffer) return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;

		//Check space remaining in buffer. If not enough, realloc.
		size_t space = w_buff_max_size - w_buff_size;
		if (bytes > space) {
			w_buff_max_size = bytes + w_buff_size;
			ubyte* temp = (ubyte*)malloc(w_buff_max_size);
			memcpy(temp, w_buffer, w_buff_size);
			free(w_buffer);
			w_buffer = temp;
		}

		//Copy to buffer.
		FLAC__byte* wpos = (FLAC__byte*)(w_buffer + w_buff_size);
		memcpy(wpos, buffer, bytes);
		w_buff_size += bytes;

		//Do I want to track samples as well? That might be a good idea...
		spos_enc += samples;

		return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
	}

	void MkvFlacTrackWriter::closeTrackWriter() {
		if (!isOpen) return;
		finish(); //Close the encoder and flush it.

		//If there's anything left in the buffer now, write to the mkvwriter
		if (w_buff_size > 0) {
			target.writeBlock(w_buffer, w_buff_size, TRACKTYPE_AUDIO, atrack_idx);
		}

		free(w_buffer);
		if (tinfo->codec_data) free(tinfo->codec_data);

		w_buff_size = 0;
		w_buffer = nullptr;
		tinfo->codec_data = nullptr;
		isOpen = false;
	}

	/*-------- Vorbis --------*/

	/*-------- VP8/9 --------*/

	const int MkvVP9TrackWriter::initTrackWriter() {
		//Allocate buffer
		//Start at 10MB
		w_buff_max_size = 0xa00000;
		w_buffer = (ubyte*)malloc(w_buff_max_size);

		//Ctx
		encctx = vp9a_openStreamEncoder(w_buffer, vid_info);
		if (!encctx) return 1;

		//Update track info seen by mkv writer
		if (!encctx->use_vp9) tinfo->encoding = VENCODING_VP8;
		tinfo->frames_per_block = frames_per_block;
		tinfo->total_frames = vid_info->total_frames;
		tinfo->pWidth = vid_info->width;
		tinfo->pHeight = vid_info->height;

		//MKV does not appear to require any kind of codec private init data phew

		isOpen = true;
		return 0;
	}

	const size_t MkvVP9TrackWriter::bytesPerUnit() const {
		if (!encctx) return 0;
		return encctx->bytes_per_frame;
	}

	const size_t MkvVP9TrackWriter::writeUnit(void* data) {
		if (!encctx || !data) return 0;

		int res = write_frame(encctx, (ubyte*)data);
		if (res) return 0;

		w_buff_size += encctx->amt_written;

		if (++fcount >= frames_per_block) {
			//Write to mkv writer and reset buffer.
			size_t sz = target.writeBlock(w_buffer, w_buff_size, TRACKTYPE_VIDEO, vtrack_idx);
			if (sz == 0) return 0;
			w_buff_size = 0;
			fcount = 0;
			encctx->target.buffer_ptr = w_buffer;
			//Not tracking time pos yet, so return this I guess.
		}
		else {
			//Move the ctx pointer forward.
			encctx->target.buffer_ptr = (ubyte*)(w_buffer + w_buff_size);
		}

		return encctx->amt_written;
	}

	void MkvVP9TrackWriter::closeTrackWriter() {
		if (!isOpen) return;

		//Free buffer.
		free(w_buffer);
		w_buffer = nullptr;

		//Close context.
		vp9a_closeEncoder(encctx);
		encctx = nullptr;

		isOpen = false;
	}

	/*-------- Encoding Manager --------*/

}