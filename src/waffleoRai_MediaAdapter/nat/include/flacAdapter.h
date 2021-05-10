#ifndef WRMAFLACADAP_H_INCLUDED
#define WRMAFLACADAP_H_INCLUDED

#include "FLAC++/encoder.h"
#include "mediaadapterutils.h"
#include "videoDefs.h"

//I decided to just put this in a common module for easier debugging
namespace WaffleoMediaAdapter {

	//Oh hell the flac classes are abstract. ughhhh whyyyyyyy
	class WRMA_DLL_API SimpleFlacFileEncoder : public FLAC::Encoder::File {

	protected:
		FLAC__StreamEncoderReadStatus read_callback_(const ::FLAC__StreamEncoder* encoder, FLAC__byte buffer[], size_t* bytes, void* client_data) {
			return FLAC__STREAM_ENCODER_READ_STATUS_CONTINUE;
		}
		FLAC__StreamEncoderWriteStatus write_callback_(const ::FLAC__StreamEncoder* encoder, const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame, void* client_data) {
			return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
		}
		FLAC__StreamEncoderSeekStatus seek_callback_(const FLAC__StreamEncoder* encoder, FLAC__uint64 absolute_byte_offset, void* client_data) {
			return FLAC__STREAM_ENCODER_SEEK_STATUS_UNSUPPORTED;
		}
		FLAC__StreamEncoderTellStatus tell_callback_(const FLAC__StreamEncoder* encoder, FLAC__uint64* absolute_byte_offset, void* client_data) {
			return FLAC__STREAM_ENCODER_TELL_STATUS_UNSUPPORTED;
		}

	public:
		SimpleFlacFileEncoder() {}

		virtual ~SimpleFlacFileEncoder() {}

	};

	class WRMA_DLL_API FlacWriter {

	private:
		aud_info_t info;
		SimpleFlacFileEncoder encoder;
		uint32_t level; //Must be 0-8 (compression quality)

		FLAC__StreamEncoderInitStatus istat = FLAC__STREAM_ENCODER_INIT_STATUS_OK;
		FLAC__StreamEncoderState sstat = FLAC__STREAM_ENCODER_OK;
		bool isOpen = false;

	public:
		FlacWriter(aud_info_t& aud_info, uint32_t complvl) :info(aud_info),encoder(),level(complvl) {
			if (level > 8) level = 8;
			if (level < 0) level = 0;
		}

		const aud_info_t& getAudioInfo() const { return info; }
		const FLAC__StreamEncoderInitStatus getInitStatus() const { return istat; };
		const FLAC__StreamEncoderState getStreamStatus() const { return sstat; };

		const int openWriter(const char* path);
		const uint32_t getBlockSize() const;
		const int writeSamples(void** data, int frames, bool pre_expanded);
		const int writeInterleavedSamples(void* data, int frames, bool pre_expanded);
		const int closeWriter();

		virtual ~FlacWriter() { closeWriter(); }
	};

}

#endif //WRMAFLACADAP_H_INCLUDED