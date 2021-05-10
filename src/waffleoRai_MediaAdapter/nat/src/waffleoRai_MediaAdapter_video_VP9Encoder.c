
#include "waffleoRai_MediaAdapter_video_VP9Encoder.h"

//TODO this MUST be updated to reflect new alloc behavior in vp9Adapter!

const char* unwrap_jstring(JNIEnv * env, jstring str){
	if(str == NULL || env == NULL) return NULL;
	
	const char* cstr = (*env)->GetStringUTFChars(env, str, NULL);
	if(cstr == NULL) return NULL;
	//Determine length
	size_t slen = strlen(cstr);
	
	//Copy
	char* copy = (char*)malloc(slen+1);
	if (!copy) return NULL;
#if defined(_WIN32)
	strcpy_s(copy, slen, cstr);
#else
	strcpy(copy, cstr);
#endif
	
	(*env)->ReleaseStringUTFChars(env, str, cstr);
	
	return copy;
}

vp9a_encode_ctx_t* openEncoder(const char* outpath, vid_info_t* info) {

	if (info == NULL || outpath == NULL) return NULL;

	//Common to lossy and lossless calls.
	vp9a_encode_ctx_t* ctx = NULL;
	pixfmt_t pfmt = (info->flags >> 5) & 0x7;
	colorModel_t cmdl = (info->flags >> 2) & 0x7;
	switch (pfmt) {
	case STANDARD:
	case SETME:
		switch (cmdl) {
		case RGB:
		case SETME:
			ctx = vp9a_openRGBEncoder(outpath, info);
			break;
		case ARGB:
			ctx = vp9a_openARGBEncoder(outpath, info);
			break;
		default: return NULL;
		}
		break;
	case YUV_I420:
		ctx = vp9a_openYUV420Encoder(outpath, info);
		break;
	case YUV_I422:
		ctx = vp9a_openYUV422Encoder(outpath, info);
		break;
	case PLANAR_444:
		ctx = vp9a_openYUV444Encoder(outpath, info);
		break;
	default: return NULL;
	}

	//If there is an error, handle that and free anything that needs to be freed.
	if (ctx == NULL) return NULL;
	if (ctx->error_code != NO_ERROR) {
		//vp9a_freeError(ctx);
		vp9a_closeEncoder(ctx);
		return NULL;
	}

	return ctx;
}

JNIEXPORT jlong JNICALL Java_waffleoRai_1MediaAdapter_video_VP9Encoder_openLosslessStream
(JNIEnv* env, jobject obj, jstring path, jint bitfields, jint frameWidth, jint frameHeight, jint fps_n, jint fps_d) {
	//Alloc info struct
	vid_info_t* vinfo = (vid_info_t*)malloc(sizeof(vid_info_t));

	//Unwrap number arguments
	vinfo->flags = (uint16_t)bitfields;
	vinfo->width = (int)frameWidth;
	vinfo->height = (int)frameHeight;
	vinfo->timebase_n = (int)fps_n;
	vinfo->timebase_d = (int)fps_d;
	vinfo->fps = (float)fps_n / (float)fps_d;

	//Unwrap path string (Copy so can free the source without having to worry about it in future calls)
	const char* cpath = unwrap_jstring(env, path);

	vp9a_encode_ctx_t* ectx = openEncoder(cpath, vinfo);

	return (jlong)ectx;
}

JNIEXPORT jlong JNICALL Java_waffleoRai_1MediaAdapter_video_VP9Encoder_openLossyStream
(JNIEnv* env, jobject obj, jstring path, jint bitfields, jint frameWidth, jint frameHeight, jint fps_n, jint fps_d, jint bitrate, jint keyint) {
	vid_info_t* vinfo = (vid_info_t*)malloc(sizeof(vid_info_t));

	//Unwrap number arguments
	vinfo->flags = (uint16_t)bitfields;
	vinfo->width = (int)frameWidth;
	vinfo->height = (int)frameHeight;
	vinfo->timebase_n = (int)fps_n;
	vinfo->timebase_d = (int)fps_d;
	vinfo->fps = (float)fps_n / (float)fps_d;
	vinfo->bitrate = (int32_t)bitrate;
	vinfo->keyintr = (int32_t)keyint;

	//Unwrap path string (Copy so can free the source without having to worry about it in future calls)
	const char* cpath = unwrap_jstring(env, path);

	vp9a_encode_ctx_t* ectx = openEncoder(cpath, vinfo);

	return (jlong)ectx;
}

JNIEXPORT jint JNICALL Java_waffleoRai_1MediaAdapter_video_VP9Encoder_setCallbackTime
(JNIEnv* env, jobject obj, jlong handle, jint framesPerCallback) {
	vp9a_encode_ctx_t* ctx = (vp9a_encode_ctx_t*)handle;
	if (!ctx) return 1;

	ctx->frames_per_callback = (int)framesPerCallback;
	return 0;
}

JNIEXPORT jint JNICALL Java_waffleoRai_1MediaAdapter_video_VP9Encoder_writeFrame
(JNIEnv* env, jobject obj, jlong handle, jbyteArray data) {
	vp9a_encode_ctx_t* ctx = (vp9a_encode_ctx_t*)handle;
	if (!ctx) return 1;

	//Unwrap byte array.
	jbyte* carr = (*env)->GetByteArrayElements(env, data, NULL);
	ubyte* darr = (ubyte*)carr;

	int res = write_frame(ctx, darr);
	(*env)->ReleaseByteArrayElements(env, data, carr, NULL);

	return (jint)res;
}

JNIEXPORT jint JNICALL Java_waffleoRai_1MediaAdapter_video_VP9Encoder_writeFrames
(JNIEnv* env, jobject obj, jlong handle, jbyteArray data, jint frameCount) {
	vp9a_encode_ctx_t* ctx = (vp9a_encode_ctx_t*)handle;
	if (!ctx) return 1;

	jbyte* carr = (*env)->GetByteArrayElements(env, data, NULL);
	ubyte* darr = (ubyte*)carr;

	int res = 0;
	int fcount = (int)frameCount;
	int f = 0;
	for (f = 0; f < fcount; f++) {
		res |= write_frame(ctx, darr);
		darr += ctx->bytes_per_frame;
	}

	(*env)->ReleaseByteArrayElements(env, data, carr, NULL);
	return (jint)res;
}

JNIEXPORT jint JNICALL Java_waffleoRai_1MediaAdapter_video_VP9Encoder_closeStream
(JNIEnv* env, jobject obj, jlong handle) {
	//Don't forget to free the path too (not automatically freed by vp9a close func)
	vp9a_encode_ctx_t* ctx = (vp9a_encode_ctx_t*)handle;
	if (!ctx) return 1;

	const char* outpath = ctx->outpath;
	vp9a_error_t err = vp9a_closeEncoder(ctx);

	if (outpath) free(outpath);

	return (jint)err;
}
