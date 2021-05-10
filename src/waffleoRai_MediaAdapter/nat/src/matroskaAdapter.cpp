
#include "matroskaAdapter.h"

using std::string;
using namespace LIBEBML_NAMESPACE;
using namespace LIBMATROSKA_NAMESPACE;

namespace WaffleoMediaAdapter {

	/*-------- MKV Writer --------*/

	const size_t MatroskaWriter::getDuration() const {
		//Duration of longest track.
		size_t max_dur = 0L;

		int i, len;
		len = vtrack_files.size();
		for (i = 0; i < len; i++) {
			MkvVideoTrackInfo_t info = vtrack_files[i];
			double millis_per_frame = getAverageMillisPerFrame(info.fr_enum);
			double total = millis_per_frame * (double)info.total_frames;
			//Ceiling
			total = ceil(total);
			size_t comp = static_cast<size_t>(total);
			if (comp > max_dur) max_dur = comp;
		}

		len = atrack_files.size();
		for (i = 0; i < len; i++) {
			MkvAudioTrackInfo_t info = atrack_files[i];
			double millis_per_frame = 1000.0 / (double)info.sample_rate;
			double total = millis_per_frame * (double)info.total_frames;
			total = ceil(total);
			size_t comp = static_cast<size_t>(total);
			if (comp > max_dur) max_dur = comp;
		}

		return max_dur;
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

	const int MatroskaWriter::loadVidTrackInfo(MkvVideoTrackInfo_t& tinfo, KaxTracks& tracks, TrackBookmark_t& bookmark) {
		KaxTrackEntry& vtrack = GetChild<KaxTrackEntry>(tracks);
		vtrack.SetGlobalTimecodeScale(STD_TIMESCALE);
		bookmark.kaxtrack = &vtrack;
		bookmark.cluster_written_callback = tinfo.cluster_written_callback;

		KaxTrackNumber& tno = GetChild<KaxTrackNumber>(vtrack);
		*(static_cast<EbmlUInteger*>(&tno)) = tinfo.track_no;

		KaxTrackUID& tuid = GetChild<KaxTrackUID>(vtrack);
		*(static_cast<EbmlUInteger*>(&tuid)) = tinfo.track_uid;

		*(static_cast<EbmlUInteger*>(&GetChild<KaxTrackType>(vtrack))) = track_video;

		KaxCodecID& codecid = GetChild<KaxCodecID>(vtrack);
		string cid_str = "V_VP9";
		switch (tinfo.encoding) {
		case VENCODING_VP8: cid_str = "V_VP8"; break;
		case VENCODING_H264: cid_str = "V_MPEG4/ISO/AVC";  break;
		case VENCODING_H265: cid_str = "V_MPEGH/ISO/HEVC";  break;
		}
		*static_cast<EbmlString*>(&codecid) = cid_str;

		//Name and language
		if (tinfo.name.length() > 0) {
			KaxTrackName& tname = GetChild<KaxTrackName>(vtrack);
			*((EbmlUnicodeString*)&tname) = tinfo.name;
		}
		if (tinfo.lan.length() > 0) {
			KaxTrackLanguage& tlan = GetChild<KaxTrackLanguage>(vtrack);
			*((EbmlString*)&tlan) = tinfo.lan;
		}
		vtrack.EnableLacing(true);

		//Codec private...
		if (tinfo.codec_data) {
			KaxCodecPrivate& codecdat = GetChild<KaxCodecPrivate>(vtrack);
			((EbmlBinary*)&codecdat)->SetBuffer((binary*)tinfo.codec_data, tinfo.codec_data_size);
		}

		//Video specific stuff.
		KaxTrackVideo& viddat = GetChild<KaxTrackVideo>(vtrack);

		//Required
		KaxVideoPixelHeight& pheight = GetChild<KaxVideoPixelHeight>(viddat);
		*(static_cast<EbmlUInteger*>(&pheight)) = tinfo.pHeight;

		KaxVideoPixelWidth& pwidth = GetChild<KaxVideoPixelWidth>(viddat);
		*(static_cast<EbmlUInteger*>(&pwidth)) = tinfo.pWidth;

		KaxVideoFlagInterlaced& iflag = GetChild<KaxVideoFlagInterlaced>(viddat);
		if(tinfo.interlaced) *(static_cast<EbmlUInteger*>(&iflag)) = 1;
		else *(static_cast<EbmlUInteger*>(&iflag)) = 2;

		KaxVideoFieldOrder& fieldOrder = GetChild<KaxVideoFieldOrder>(viddat);
		switch (tinfo.field_order) {
		case FIELDORDER_NONE: *(static_cast<EbmlUInteger*>(&fieldOrder)) = 0; break;
		case FIELDORDER_LOWERFIRST: *(static_cast<EbmlUInteger*>(&fieldOrder)) = 6; break;
		case FIELDORDER_UPPERFIRST: *(static_cast<EbmlUInteger*>(&fieldOrder)) = 1; break;
		default: *(static_cast<EbmlUInteger*>(&fieldOrder)) = 2; break;
		}

		//If tinfo has noted
		//Stereo mode (later)
		//Alpha
		if (tinfo.alpha_mode == VALPHAMODE_ON) {
			KaxVideoAlphaMode& alpha = GetChild<KaxVideoAlphaMode>(viddat);
			*(static_cast<EbmlUInteger*>(&alpha)) = 1;
		}
		if (tinfo.resize_behavior != VRESIZE_UNSPECIFIED) {
			KaxVideoAspectRatio& rs = GetChild<KaxVideoAspectRatio>(viddat);
			*(static_cast<EbmlUInteger*>(&rs)) = tinfo.resize_behavior;
		}

		//(Eventually) color/pix fmt data
		//These require a lot of detail, so I'll see if it works without
		//Since those data should be in the compressed stream anyway

		return 0;
	}

	const int MatroskaWriter::loadAudTrackInfo(MkvAudioTrackInfo_t& tinfo, KaxTracks& tracks, TrackBookmark_t& bookmark) {
		KaxTrackEntry& atrack = GetChild<KaxTrackEntry>(tracks);
		atrack.SetGlobalTimecodeScale(STD_TIMESCALE);
		bookmark.kaxtrack = &atrack;
		bookmark.cluster_written_callback = tinfo.cluster_written_callback;

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

		//Audio specific stuff.
		KaxTrackAudio& auddat = GetChild<KaxTrackAudio>(atrack);
		KaxAudioSamplingFreq& sr = GetChild<KaxAudioSamplingFreq>(auddat);
		*(static_cast<EbmlFloat*>(&sr)) = tinfo.sample_rate;
		auddat.ValidateSize();

		KaxAudioChannels& ch = GetChild<KaxAudioChannels>(atrack);
		*(static_cast<EbmlUInteger*>(&ch)) = tinfo.channels;

		if (tinfo.encoding == AENCODING_PCM) {
			//Bit depth
			KaxAudioBitDepth& bd = GetChild<KaxAudioBitDepth>(atrack);
			*(static_cast<EbmlUInteger*>(&bd)) = tinfo.bitDepth;
		}

		return 0;
	}

	const int MatroskaWriter::beginMKVWrite(string& path) {
		//I cannot find good documentation from libmatroska, so this is from looking at their test6.cpp
		if (isOpen) return 1;
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
		seg_size += segsz;
		KaxTracks& tracks = GetChild<KaxTracks>(*segment);

		//[SEEKHEAD init] So, I think this is for the segment SeekHead
		//I think we're reserving space here because we don't know the other element locations yet
		mseek_dummy = new EbmlVoid();
		mseek_dummy -> SetSize(300);
		mseek_dummy->Render(*output, false);
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
		seg_size += infosz;
		metaseek->IndexThis(seginfo, *segment);

		writeTrackInfo(tracks);
		cues = new KaxCues();
		cues->SetGlobalTimecodeScale(STD_TIMESCALE);

		isOpen = true;
		return 0; //Success
	}

	void MatroskaWriter::adjustBlockSizing() {
		//TODO
		//If audio blocks can be chunked up into similar size or multiple of video block size, then try that.
		

	}

	const int MatroskaWriter::writeTrackInfo(KaxTracks& tracks) {

		adjustBlockSizing();

		//Do the tracks (and init track bookmarks)
		int i, len;
		len = vtrack_files.size();
		for (i = 0; i < len; i++) {
			MkvVideoTrackInfo_t& info = vtrack_files[i];
			vtrack_bookmarks.push_back(TrackBookmark_t());
			loadVidTrackInfo(info, tracks, vtrack_bookmarks.back());
		}

		len = atrack_files.size();
		for (i = 0; i < len; i++) {
			MkvAudioTrackInfo_t& info = atrack_files[i];
			atrack_bookmarks.push_back(TrackBookmark_t());
			loadAudTrackInfo(info, tracks, atrack_bookmarks.back());
		}

		//Render
		uint64 render_sz = tracks.Render(*output, false);
		seg_size += render_sz;

		//MetaSeek
		metaseek->IndexThis(tracks, *segment);

		return 0;
	}

	const bool MatroskaWriter::videoTrackClusterIsFull(const int idx) {
		if (!current_cluster) return false;

		TrackBookmark_t& bookmark = vtrack_bookmarks[idx];
		if (bookmark.ctime >= millis_per_cluster) return true;

		return false;
	}

	const bool MatroskaWriter::audioTrackClusterIsFull(const int idx) {
		if (!current_cluster) return false;

		TrackBookmark_t& bookmark = atrack_bookmarks[idx];
		if (bookmark.ctime >= millis_per_cluster) return true;

		return false;
	}

	const bool MatroskaWriter::clusterIsFull() {
		if (!current_cluster) return false;

		int i, len;
		len = vtrack_bookmarks.size();
		for (i = 0; i < len; i++) {
			TrackBookmark_t& bookmark = vtrack_bookmarks[i];
			if (bookmark.ctime < millis_per_cluster) return false;
		}

		len = atrack_bookmarks.size();
		for (i = 0; i < len; i++) {
			TrackBookmark_t& bookmark = atrack_bookmarks[i];
			if (bookmark.ctime < millis_per_cluster) return false;
		}

		return true;
	}

	void MatroskaWriter::clusterWriteCallback(uint64_t snaptime) {

		//Updates the writer timestamp
		int i, len;
		uint64_t inc = millis_per_cluster;
		if (snaptime) {
			//New time is end of block that is the furthest back.
			len = vtrack_bookmarks.size();
			for (i = 0; i < len; i++) {
				TrackBookmark_t& bookmark = vtrack_bookmarks[i];
				if (bookmark.ctime < inc) inc = bookmark.ctime;
			}

			len = atrack_bookmarks.size();
			for (i = 0; i < len; i++) {
				TrackBookmark_t& bookmark = atrack_bookmarks[i];
				if (bookmark.ctime < inc) inc = bookmark.ctime;
			}
		}
		cluster_time += inc;

		//Goes through all track bookmarks.
		//Update bookmark timestamps
		//Call bookmark callback if present

		len = vtrack_bookmarks.size();
		for (i = 0; i < len; i++) {
			TrackBookmark_t& bookmark = vtrack_bookmarks[i];
			bookmark.head_time = cluster_time;
			bookmark.ctime -= inc;
			if (bookmark.cluster_written_callback) bookmark.cluster_written_callback(cluster_time);
			MkvVideoTrackInfo_t& tinfo = vtrack_files[i];
			tinfo.new_clust_flag = true;
		}

		len = atrack_bookmarks.size();
		for (i = 0; i < len; i++) {
			TrackBookmark_t& bookmark = atrack_bookmarks[i];
			bookmark.head_time = cluster_time;
			bookmark.ctime -= inc;
			if (bookmark.cluster_written_callback) bookmark.cluster_written_callback(cluster_time);
			MkvAudioTrackInfo_t& tinfo = atrack_files[i];
			tinfo.new_clust_flag = true;
		}

	}

	const size_t MatroskaWriter::writeBlock(void* data, size_t datalen, int trackType, int trackNo, uint64_t time_dur, bool index) {

		if (!isOpen) return 0;

		if (clusterIsFull()) {
			//Write existing cluster
			//Null current cluster so a new one is generated.
			seg_size += current_cluster->Render(*output, *cues, false);
			current_cluster->ReleaseFrames();
			metaseek->IndexThis(*current_cluster, *segment);
			delete current_cluster;
			current_cluster = nullptr;
			clusterWriteCallback(false);
		}
		if (!current_cluster) {
			//New cluster
			current_cluster = new KaxCluster();
			current_cluster->SetParent(*segment); // mandatory to store references in this Cluster
			//TODO it isn't clear to me whether this method sets it in your timescale or nanos. Check that.
			current_cluster->SetPreviousTimecode(cluster_time * STD_TIMESCALE, STD_TIMESCALE); // the first timecode here
			current_cluster->EnableChecksum();
		}

		//If CURRENT track cluster is already full (but others aren't otherwise wouldn't pass above checks)
		//Reject and return 0.
		TrackBookmark_t* bookmark = NULL;
		if (trackType == TRACKTYPE_VIDEO) {
			if (videoTrackClusterIsFull(trackNo)) return 0;
			bookmark = &vtrack_bookmarks[trackNo];
			MkvVideoTrackInfo_t& vtinfo = vtrack_files[trackNo];
			vtinfo.new_clust_flag = false;
		}
		else if (trackType == TRACKTYPE_AUDIO) {
			if (audioTrackClusterIsFull(trackNo)) return 0;
			bookmark = &atrack_bookmarks[trackNo];
			MkvAudioTrackInfo_t& atinfo = atrack_files[trackNo];
			atinfo.new_clust_flag = false;
		}
		if (!bookmark) {
			return 0;
		}

		//Now add this new block and update bookmark
		DataBuffer* dbuff = new DataBuffer((binary*)data, datalen, NULL, true); //Eh, make it copy. It's not great, but I'll fix it later.
		KaxBlockGroup* bgrp;
		current_cluster->AddFrame(*bookmark->kaxtrack, (bookmark->ctime - bookmark->head_time) * STD_TIMESCALE, *dbuff, bgrp);
		bookmark->ctime += time_dur;

		//Index to Cues if requested
		if (index) {
			KaxBlockBlob* bb = new KaxBlockBlob(BLOCK_BLOB_NO_SIMPLE);
			bb->SetBlockGroup(*bgrp);
			cues->AddBlockBlob(*bb);
		}

		return datalen;
	}

	const size_t MatroskaWriter::forceClusterWrite() {
		if (!current_cluster || !isOpen) return 0;
		size_t sz = current_cluster->Render(*output, *cues, false);
		seg_size += sz;
		current_cluster->ReleaseFrames();
		metaseek->IndexThis(*current_cluster, *segment);
		delete current_cluster;
		current_cluster = nullptr;
		//cluster_time += millis_per_cluster;
		clusterWriteCallback(true);

		return sz;
	}

	const size_t MatroskaWriter::clusterMillisExpected(int trackType, int trackNo) {
		//Millis still expected form this track to fill the cluster

		if (trackNo < 0) return 0;
		if (trackType == TRACKTYPE_VIDEO) {
			if (trackNo >= vtrack_bookmarks.size()) return 0;
			TrackBookmark_t& bookmark = vtrack_bookmarks[trackNo];
			return millis_per_cluster - bookmark.ctime;
		}
		else if (trackType == TRACKTYPE_AUDIO) {
			if (trackNo >= atrack_bookmarks.size()) return 0;
			TrackBookmark_t& bookmark = atrack_bookmarks[trackNo];
			return millis_per_cluster - bookmark.ctime;
		}

		return 0;
	}

	const int MatroskaWriter::finishMKVWrite() {
		if (!isOpen) return 0;
		//Last cluster
		forceClusterWrite();

		//Write cues
		filepos_t cue_sz = cues->Render(*output, false);
		metaseek->IndexThis(*cues, *segment);

		//Finish MetaSeek
		filepos_t ms_size = mseek_dummy->ReplaceWith(*metaseek, *output, false);
		seg_size += cue_sz + ms_size;

		//Tidy up mkv
		if (segment->ForceSize(seg_size - segment->HeadSize())) {
			segment->OverwriteHead(*output);
		}
		output->close();

		//Free everything
		output = nullptr;
		delete mseek_dummy; mseek_dummy = nullptr;
		delete metaseek; metaseek = nullptr;
		delete segment; segment = nullptr;
		delete cues; cues = nullptr;
		vtrack_bookmarks.clear();
		atrack_bookmarks.clear();

		isOpen = false;
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

		//Block size should be set manually for now...

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

	const bool MkvFlacTrackWriter::clusterBlocksRemaining() const {
		if (!tinfo) return false;
		size_t needed_millis = target.clusterMillisExpected(TRACKTYPE_AUDIO, atrack_idx);
		if (needed_millis >= tinfo->millis_per_block - 1) return true;
		return false;
	}

	const size_t MkvFlacTrackWriter::writeUnit(void* data, bool indexme) {

		if (!data || !tinfo) return 0;
		bool res = false;
		uint32_t i, j,k;
		uint32_t  fpb = tinfo->frames_per_block;
		size_t sz = bytesPerUnit();
		uint64_t spos = spos_enc;

		if (input_type == FLACINPUT_PCM_LE) {
			//Everything is going to have to be expanded anyway.
			FLAC__int32** charr = (FLAC__int32**)malloc(tinfo->channels * sizeof(FLAC__int32*));
			if (!charr) return 0;
			FLAC__int32* inbuff = (FLAC__int32*)malloc(sz);
			if (!inbuff) {
				free(charr);
				return 0;
			}

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


			res = process(charr, fpb);
			free(inbuff);
			free(charr);
			if (!res) return 0;
		}
		else if (input_type == FLACINPUT_SAMP32_ARR) {
			if (input_interleaved) {
				//Should be able to cast and feed as-is
				FLAC__int32* input = (FLAC__int32*)data;
				res = process_interleaved(input, fpb);
				if (!res) return 0;
			}
			else {
				//Make ptr array for channel starts.
				FLAC__int32** charr = (FLAC__int32**)malloc((static_cast<size_t>(tinfo->channels) + 1) * sizeof(FLAC__int32*));
				if (!charr) return 0;
				FLAC__int32* ptr = (FLAC__int32*)data;
				for (i = 0; i < tinfo->channels; i++) {
					charr[i] = ptr;
					ptr += fpb;
				}
				res = process(charr, fpb);
				free(charr); //whew almost forgot
				if (!res) return 0;
			}
		}
		else return 0;

		//Calculate time of this block
		uint64_t samps = spos_enc - spos;
		uint64_t stime = (samps * 1000)/ tinfo->sample_rate; //Floored, though # of samples even to the ms would be ideal input

		//now pass to the mkv writer...
		size_t ressz = target.writeBlock(w_buffer, w_buff_size, TRACKTYPE_AUDIO, atrack_idx, stime, indexme || tinfo->new_clust_flag);
		//Clear buffer
		w_buff_size = 0;
		spos_raw += fpb;
		return ressz;
	}

	FLAC__StreamEncoderWriteStatus MkvFlacTrackWriter::write_callback(const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame) {
		//Make sure writer is initialized
		if (!w_buffer) return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;

		//Check space remaining in buffer. If not enough, realloc.
		size_t space = w_buff_max_size - w_buff_size;
		if (bytes > space) {
			w_buff_max_size = bytes + w_buff_size;
			ubyte* temp = (ubyte*)malloc(w_buff_max_size);
			if (!temp) return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
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

		/*//If there's anything left in the buffer now, write to the mkvwriter
		if (w_buff_size > 0) {
			target.writeBlock(w_buffer, w_buff_size, TRACKTYPE_AUDIO, atrack_idx);
		}*/

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
		//tinfo->frames_per_block = frames_per_block;
		tinfo->total_frames = vid_info->total_frames;
		tinfo->pWidth = vid_info->width;
		tinfo->pHeight = vid_info->height;
		tinfo->interlaced = false;
		tinfo->field_order = FIELDORDER_NONE;

		//Framerate nonsense
		tinfo->fr_enum = framerate_from_rational(vid_info->timebase_n, vid_info->timebase_d);
		tinfo->frames_per_block = getCycleFrameCount(tinfo->fr_enum);
		tinfo->block_millis = getCycleMillsLength(tinfo->fr_enum);
		if (tinfo->frames_per_block > 3) {
			tinfo->frames_per_block = 1;
			tinfo->block_millis = getCycleFrameLength(tinfo->fr_enum, 0);
		}

		//MKV does not appear to require any kind of codec private init data phew

		isOpen = true;
		return 0;
	}

	const size_t MkvVP9TrackWriter::bytesPerUnit() const {
		if (!encctx) return 0;
		return encctx->bytes_per_frame;
	}

	const bool MkvVP9TrackWriter::clusterBlocksRemaining() const {
		if (!tinfo) return false;
		size_t needed_millis = target.clusterMillisExpected(TRACKTYPE_VIDEO, vtrack_idx);
		if (needed_millis >= tinfo->block_millis - 1) return true;
		return false;
	}

	const size_t MkvVP9TrackWriter::writeUnit(void* data, bool indexme) {
		if (!encctx || !data) return 0;

		int res = write_frame(encctx, (ubyte*)data);
		if (res) return 0;

		w_buff_size += encctx->amt_written;

		if (++fcount >= tinfo->frames_per_block) {
			//Write to mkv writer and reset buffer.
			size_t sz = target.writeBlock(w_buffer, w_buff_size, TRACKTYPE_VIDEO, vtrack_idx, tinfo->block_millis, indexme || tinfo->new_clust_flag);
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

		//If anything left in buffer, write
		if (w_buff_size > 0) {
			double ratio = fcount / tinfo->frames_per_block;
			uint64_t dur = static_cast<uint64_t>(ratio * tinfo->block_millis + 1);
			target.writeBlock(w_buffer, w_buff_size, TRACKTYPE_VIDEO, tinfo->track_no, dur, false);
		}

		//Free buffer.
		free(w_buffer);
		w_buffer = nullptr;

		//Close context.
		vp9a_closeEncoder(encctx);
		encctx = nullptr;

		isOpen = false;
	}

	/*-------- Encoding Manager --------*/

	const int32_t MkvWriteManager::addVP9Track(vid_info_t* info) {
		MkvVP9TrackWriter* trackwriter = new MkvVP9TrackWriter(writer, info);
		int32_t addidx = static_cast<int32_t>(tracks.size());

		trackwriter->initTrackWriter();
		tracks.push_back(trackwriter);

		return addidx;
	}

	const int32_t MkvWriteManager::addFLACTrack(aud_info_t* info, int frames_per_block) {
		MkvFlacTrackWriter* trackwriter = new MkvFlacTrackWriter(writer);
		int32_t addidx = static_cast<int32_t>(tracks.size());

		//Have to manually set data.
		MkvAudioTrackInfo_t& tinfo = trackwriter->getTrackInfo();
		tinfo.bitDepth = info->bit_depth;
		tinfo.sample_rate = info->sample_rate;
		tinfo.channels = info->channels;
		tinfo.frames_per_block = frames_per_block;
		tinfo.millis_per_block = (frames_per_block * 1000) / tinfo.sample_rate;
		tinfo.total_frames = info->frame_count;

		trackwriter->setInputInterleaved(info->interleaved);
		
		trackwriter->initTrackWriter();
		tracks.push_back(trackwriter);
		return addidx;
	}

	const bool MkvWriteManager::open(string& path) {
		if (isOpen) return false;
		if (tracks.empty()) return false;

		writer.setTitle(title);
		writer.setWriteApp(write_app);
		writer.beginMKVWrite(path);

		isOpen = true;
		return true;
	}

	const size_t MkvWriteManager::writeDataUnit(int32_t track, void* data, size_t datalen) {
		if (!isOpen) return 0;
		if (track < 0 || track >= tracks.size()) return 0;

		MkvTrackWriter* tw = tracks[track];
		return tw->writeUnit(data, false);
	}

	const bool MkvWriteManager::close() {
		if (!isOpen) return true;

		//Close tracks FIRST!
		int i, len;
		len = tracks.size();
		for (i = 0; i < len; i++) tracks[i]->closeTrackWriter();

		writer.finishMKVWrite();

		for (i = 0; i < len; i++) delete tracks[i];

		isOpen = false;
		return true;
	}

}