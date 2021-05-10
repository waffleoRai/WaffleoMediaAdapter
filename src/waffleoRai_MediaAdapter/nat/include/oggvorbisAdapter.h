#ifndef OGGVORBADAPTER_H_INCLUDED
#define OGGVORBADAPTER_H_INCLUDED

#include <stdio.h>

#include "videoDefs.h"
#include "vorbis/vorbisenc.h"

#ifdef __cplusplus
extern "C" {
#endif

	//TODO Deinterleaving if needed?

	typedef WRMA_DLL_API struct oggvorb_comment_set {

		char** comments;
		int cmt_count;
		int capacity;

	} oggvorb_comment_set_t;

	typedef WRMA_DLL_API struct oggvorb_writer_ctx {

		aud_info_t info;

		vorbis_info vorbinfo;
		vorbis_dsp_state dsp_state;
		vorbis_comment vorb_cmt;
		vorbis_block vorb_block;

		FILE* output;
		ogg_stream_state oggstr;
		int ogg_serial;

		int err;

	} oggvorb_writer_ctx_t;

	WRMA_DLL_API oggvorb_writer_ctx_t* initOggVorbisWriter(aud_info_t* audio_info, oggvorb_comment_set_t* comments, const char* path, int quality); //Quality should be from 1-10
	WRMA_DLL_API size_t writeAudioData(oggvorb_writer_ctx_t* ctx, void** data, int frames);
	WRMA_DLL_API int closeOggVorbisWriter(oggvorb_writer_ctx_t* ctx);

	WRMA_DLL_API oggvorb_comment_set_t* initCommentContainer(int initial_capacity);
	WRMA_DLL_API void addComment(oggvorb_comment_set_t* cmt_cont, const char* comment);
	WRMA_DLL_API void freeCommentContainer(oggvorb_comment_set_t* cmt_cont);

#ifdef __cplusplus
}
#endif

#endif // OGGVORBADAPTER_H_INCLUDED
