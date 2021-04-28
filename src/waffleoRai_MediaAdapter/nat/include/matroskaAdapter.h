#ifndef MATROSKAADAPTER_H_INCLUDED
#define MATROSKAADAPTER_H_INCLUDED

#include "mediaadapterutils.h"
#include <vector>
#include <queue>
#include <cstdlib>

#include "ebml/StdIOCallback.h"
#include "ebml/EbmlHead.h"
#include "ebml/EbmlSubHead.h"
#include "ebml/EbmlVoid.h"

#include "matroska/FileKax.h"
#include "matroska/KaxSegment.h"
#include "matroska/KaxTracks.h"
#include "matroska/KaxSeekHead.h"
#include "matroska/KaxCues.h"
#include "matroska/KaxCluster.h"

#include "FLAC++/encoder.h"

#include "vp9Adapter.h"

#define VENCODING_VP8 1
#define VENCODING_VP9 2
#define VENCODING_H264 3
#define VENCODING_H265 4

#define AENCODING_FLAC 1
#define AENCODING_VORBIS 2
#define AENCODING_PCM 3

#define TRACKTYPE_VIDEO 1
#define TRACKTYPE_AUDIO 2
#define TRACKTYPE_SUBTITLE 3

#define FIELDORDER_UNSPECIFIED -1
#define FIELDORDER_NONE 1
#define FIELDORDER_LOWERFIRST 2
#define FIELDORDER_UPPERFIRST 3

#define VSTEREOMODE_UNSPECIFIED -1
#define VSTEREOMODE_OFF 0

#define VALPHAMODE_UNSPECIFIED -1
#define VALPHAMODE_OFF 0
#define VALPHAMODE_ON 1

#define VRESIZE_UNSPECIFIED -1
#define VRESIZE_FREE 0
#define VRESIZE_KEEP_RATIO 1
#define VRESIZE_FIXED 2

#define VCOLORSCALE_UNSPECIFIED -1

#define FLACINPUT_PCM_LE 0 //Raw PCM as would see in regular block of data
#define FLACINPUT_SAMP32_ARR 1 //Samples already expanded to 32-bit int array (ie. 32 bits per sample regardless of target bit depth). Takes machine endianness.

using std::string;
using std::vector;
//using std::list;
using std::queue;
using icu::UnicodeString;
using LIBEBML_NAMESPACE::UTFstring;

namespace WaffleoMediaAdapter {

	//# of nanoseconds per time unit (ie. if 1000000, then time unit is in millis)
	WRMA_DLL_API const uint64_t STD_TIMESCALE = 1000000L;

	typedef struct BufferLoc {
		ubyte* loc;
		size_t size;

	}BufferLoc_t;

	typedef struct TrackBookmark {
		uint64_t head_time = 0; //Time at cluster start
		uint64_t ctime = 0; //Millis written to cluster so far.
		KaxTrackEntry* kaxtrack = NULL;
		void (*cluster_written_callback)(uint64_t) = NULL; //Passes timestamp of next cluster (end of written cluster)
	}TrackBookmark_t;

	typedef WRMA_DLL_API struct MkvAudioTrackInfo {
		UTFstring name = L"untitled_track";
		string lan = "eng";

		uint32_t frames_per_block = 441;
		uint32_t millis_per_block = 10;
		int8_t blocks_per_cluster = 100;

		uint32_t total_frames = 0; //Estimated length in audio frames
		int encoding = AENCODING_FLAC; //Encoding pseudo enum

		int32_t track_no = -1;
		uint32_t track_uid = 0;
		uint64_t time_pos = 0; //For mkv to track current position
		void (*cluster_written_callback)(uint64_t) = NULL;

		int16_t sample_rate = 44100;
		int16_t channels = 2;
		int8_t bitDepth = 16;

		void* codec_data = NULL;
		size_t codec_data_size = 0;

		bool new_clust_flag = true;

	} MkvAudioTrackInfo_t;

	typedef WRMA_DLL_API struct MkvVideoTrackInfo {
		UTFstring name = L"untitled_track";
		string lan = "eng";

		//Defaults to 60 fps
		uint32_t frames_per_block = 3;
		uint32_t blocks_per_cluster = 20;
		uint32_t block_millis = 50;
		vid_frame_rate_t fr_enum = FPS_UNKNOWN;

		uint32_t total_frames = 0;
		int encoding = VENCODING_VP9; //Encoding pseudo enum

		int32_t track_no = -1;
		uint32_t track_uid = 0;
		uint64_t time_pos = 0; //For mkv to track current position
		void (*cluster_written_callback)(uint64_t) = NULL;

		bool interlaced = false;
		int8_t field_order = FIELDORDER_UNSPECIFIED;
		int8_t stereo_mode = VSTEREOMODE_UNSPECIFIED;
		int8_t alpha_mode = VALPHAMODE_UNSPECIFIED;

		int32_t pWidth = -1;
		int32_t pHeight = -1;
		int32_t dWidth = -1;
		int32_t dHeight = -1;

		int8_t resize_behavior = VRESIZE_KEEP_RATIO;
		colorModel_t color = CLR_SETME;

		void* codec_data = NULL;
		size_t codec_data_size = 0;

		bool new_clust_flag = true;

	} MkvVideoTrackInfo_t;

	class WRMA_DLL_API MatroskaWriter {

	private:
		//Info
		vector<MkvVideoTrackInfo_t> vtrack_files;
		vector<MkvAudioTrackInfo_t> atrack_files;

		UTFstring write_app;
		UTFstring title;

		uint32_t millis_per_cluster = 1000;
		uint64_t cluster_time = 0;
		//uint8_t blocks_per_cluster = 5;

		//Output state
		StdIOCallback* output = nullptr;
		EbmlVoid* mseek_dummy = nullptr;
		KaxSeekHead* metaseek = nullptr;
		KaxSegment* segment = nullptr;
		KaxCues* cues = nullptr;
		uint64_t seg_size = 0;
		bool isOpen = false;
		//queue<void*> block_ptr_q = queue<void*>(); //Copy block data here, then free after cluster written.

		vector<TrackBookmark_t> vtrack_bookmarks = vector<TrackBookmark_t>(16);
		vector<TrackBookmark_t> atrack_bookmarks = vector<TrackBookmark_t>(16);
		//uint64_t cluster_ts;
		KaxCluster* current_cluster = nullptr;

		const int loadVidTrackInfo(MkvVideoTrackInfo_t& tinfo, KaxTracks& tracks, TrackBookmark_t& bookmark);
		const int loadAudTrackInfo(MkvAudioTrackInfo_t& tinfo, KaxTracks& tracks, TrackBookmark_t& bookmark);

		const int writeTrackInfo(KaxTracks& tracks);

		void adjustBlockSizing(); //Auto-tweak cluster size and track block sizes to fit
		const bool videoTrackClusterIsFull(const int idx);
		const bool audioTrackClusterIsFull(const int idx);
		const bool clusterIsFull(); //Current cluster ready to write
		void clusterWriteCallback(uint64_t snaptime);

	public:
		MatroskaWriter():vtrack_files(16), atrack_files(16),write_app(L"waffleoRaiMediaAdapter"),title(L"UntitledMKV"),output(nullptr) {};

		//Getters
		const UTFstring& getWriteApp() const { return write_app; }
		const UTFstring& getTitle() const { return title; }
		const uint32_t getMillisPerCluster() const { return millis_per_cluster; }
		//const uint8_t getBlocksPerCluster() const { return blocks_per_cluster; }
		const size_t getVideoTrackCount()const { return vtrack_files.size(); }
		const size_t getAudioTrackCount()const { return atrack_files.size(); }
		MkvVideoTrackInfo_t& getVideoTrackInfo(int idx) { return vtrack_files[idx]; }
		MkvAudioTrackInfo_t& getAudioTrackInfo(int idx) { return atrack_files[idx]; }
		const size_t getDuration() const; //In millis

		//Setters
		MkvVideoTrackInfo_t& addVideoTrack(int enc);
		MkvAudioTrackInfo_t& addAudioTrack(int enc);
		void setWriteApp(UTFstring& s) { write_app = s; }
		void setTitle(UTFstring& s) { title = s; }
		void setMillisPerCluster(uint32_t millis) { millis_per_cluster = millis; }
		//void setBlocksPerCluster(uint8_t blocks) { blocks_per_cluster = blocks; }

		//In pieces
		const int beginMKVWrite(string& path);
		const size_t writeBlock(void* data, size_t datalen, int trackType, int trackNo, uint64_t time_dur, bool index);
		const size_t forceClusterWrite(); //Normally, it will refuse to add to a cluster if all blocks for a track have already been added. Use this func to force it to dump the current cluster and start a new one.
		const size_t clusterMillisExpected(int trackType, int trackNo);
		const int finishMKVWrite();

		virtual ~MatroskaWriter() {}
	};

	class WRMA_DLL_API MkvTrackWriter {
		//Abstract class for manager to write data to.
	public:
		virtual const size_t bytesPerUnit() const = 0;
		virtual const size_t unitsPerBlock() const = 0;
		virtual const size_t blocksPerCluster() const = 0;
		virtual const bool clusterBlocksRemaining() const = 0;
		virtual const size_t writeUnit(void* data, bool indexme) = 0;
		virtual void closeTrackWriter() = 0;

		virtual ~MkvTrackWriter() { closeTrackWriter(); }
	};

	class WRMA_DLL_API MkvFlacTrackWriter:public MkvTrackWriter, protected FLAC::Encoder::Stream {
	private:
		MkvAudioTrackInfo_t* tinfo;
		MatroskaWriter& target;
		int atrack_idx;

		ubyte* w_buffer = nullptr;
		uint32_t w_buff_size = 0; //Current size
		uint32_t w_buff_max_size = 0;

		//bool new_clust_flag = true;
		uint64_t spos_raw; //Expected next sample based on block size
		uint64_t spos_enc; //Next sample expected from encoder based on encoder reporting

		bool input_interleaved = true;
		int input_type = FLACINPUT_PCM_LE;

		//queue<void*> pending_blocks = queue<void*>();

		bool isOpen = false;
		FLAC__StreamEncoderInitStatus init_stat = FLAC__STREAM_ENCODER_INIT_STATUS_OK;

		//void onClusterWrite(uint64_t timestamp) { new_clust_flag = true; }

	protected:
		FLAC__StreamEncoderWriteStatus write_callback(const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame) override;

	public:
		MkvFlacTrackWriter(MatroskaWriter& writer) :target(writer) {
			tinfo = &writer.addAudioTrack(AENCODING_FLAC);
			atrack_idx = writer.getAudioTrackCount()-1;
		}

		MkvAudioTrackInfo_t& getTrackInfo() { return *tinfo; }
		const bool inputInterleaved() const { return input_interleaved; } //Does it expect the input to be channel interleaved?
		const int inputFormat() const { return input_type; }
		void setInputInterleaved(bool b) { input_interleaved = b; }
		void setInputFormat(int type) { input_type = type; }

		const int initTrackWriter(); //Renders the FLAC header to the tinfo so can mkv writer can write track element
		const FLAC__StreamEncoderInitStatus& getInitStatus() { return init_stat; }

		const size_t bytesPerUnit() const override;
		const size_t unitsPerBlock() const override;
		const size_t blocksPerCluster() const override;
		const size_t writeUnit(void* data, bool indexme) override;
		const bool clusterBlocksRemaining() const override;

		void closeTrackWriter() override; //Closes any open streams and frees any buffers used by this track

		virtual ~MkvFlacTrackWriter() { closeTrackWriter(); }
	};

	class WRMA_DLL_API MkvVP9TrackWriter :public MkvTrackWriter {
	private:
		MkvVideoTrackInfo_t* tinfo = nullptr;
		MatroskaWriter& target;
		int vtrack_idx;

		vid_info_t* vid_info;

		//uint32_t frames_per_block = 1;
		//uint32_t blocks_per_cluster = 60;

		uint32_t fcount = 0; //Frames prepped for current block.

		vp9a_encode_ctx_t* encctx = nullptr;
		ubyte* w_buffer = nullptr;
		uint32_t w_buff_size = 0; //Current size
		uint32_t w_buff_max_size = 0;

		bool isOpen = false;

	public:
		MkvVP9TrackWriter(MatroskaWriter& writer, vid_info_t* vidinfo):target(writer) {
			//Don't forget to update encoding on init when flags are parsed!
			tinfo = &target.addVideoTrack(VENCODING_VP9);
			vtrack_idx = writer.getVideoTrackCount() - 1;
			vid_info = vidinfo;
		}

		const int initTrackWriter();

		const size_t bytesPerUnit() const override;
		const size_t unitsPerBlock() const override { return tinfo?tinfo->frames_per_block:0; }
		const size_t blocksPerCluster() const override { return tinfo ? tinfo->blocks_per_cluster : 0; }
		const size_t writeUnit(void* data, bool indexme) override;
		const bool clusterBlocksRemaining() const override;

		void setFramesPerBlock(uint32_t val) { if(tinfo)tinfo->frames_per_block = val; }
		//void setBlocksPerCluster(uint32_t val) { if (tinfo)tinfo->blocks_per_cluster = val; }

		void closeTrackWriter() override;

		virtual ~MkvVP9TrackWriter() { closeTrackWriter(); }
	};

	class WRMA_DLL_API MkvWriteManager {

	private:
		MatroskaWriter writer = MatroskaWriter();
		vector<MkvTrackWriter*> tracks = vector<MkvTrackWriter*>(16);

		UTFstring write_app = L"waffleoRaiMediaAdapter";
		UTFstring title = L"untitled_mkv";

		bool isOpen = false;

	public:
		MkvWriteManager() {}

		const int32_t addVP9Track(vid_info_t* info);
		const int32_t addFLACTrack(aud_info_t* info, int frames_per_block);

		void setWriteAppName(UTFstring& name) { write_app = name; }
		void setTitle(UTFstring& name) { title = name; }

		const bool open(string& path);
		const size_t writeDataUnit(int32_t track, void* data, size_t datalen);
		const bool close();

		virtual ~MkvWriteManager() { close(); }

	};

}


#endif // MATROSKAADAPTER_H_INCLUDED