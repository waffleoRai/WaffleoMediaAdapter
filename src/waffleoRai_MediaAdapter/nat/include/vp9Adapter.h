#ifndef VP9ADAPTER_H_INCLUDED
#define VP9ADAPTER_H_INCLUDED

#include <stdio.h>

#include "vpx/vpx_image.h"
#include "vpx/vpx_encoder.h"
#include "vpx/vp8cx.h"
#include "vp9/common/vp9_common.h"
#include "tools_common.h"
#include "video_writer.h"
#include "vidUtils.h"

//Pass a flags field (16 bits) with encoding info
/*
    Bit...
        0   VP9? (If unset, then use VP8)
        1   Lossless?
        2-4 Color Model
            0   RGB
            1   ARGB
            2   YUV BT.601 (SD)
            3   YUV BT.701 (HD)
            4   YUV BT.2020
            7   (Unset - set by open func)
        5-7 Pixel Format
            0   By pixel (8 bits per channel ARGB, RGB, or YUV)
            1   YUV I420
            2   YUV I422
            3   Planar 444
            7   (Unset - set by open func)
        8   Wide? (16-bit?)
        9   Full color (0-255 as opposed to analog 16-240)

*/

#ifdef __cplusplus
extern "C" {
#endif

typedef WRMA_DLL_API enum colorModel{
    RGB = 0,
    ARGB = 1,
    BT601 = 2,
    BT709 = 3,
    BT2020 = 4,
    CLR_SETME = 7
} colorModel_t;

typedef WRMA_DLL_API enum pixfmt{
    STANDARD = 0,
    YUV_I420 = 1,
    YUV_I422 = 2,
    PLANAR_444 = 3,
    SETME = 7
} pixfmt_t;

typedef WRMA_DLL_API enum vp9a_error{
    NO_ERROR = 0,
    UNSUPPORTED_FMT = 1,
    CODEC_NOT_FOUND = 2,
    INVALID_FRAME_SIZE = 3,
    INVALID_FRAMERATE = 4,
    FILE_IO_ERROR = 5,
    NULL_INFO_ARG = 6,
    FMT_MISMATCH = 7,
    IMGALLOC_FAILED = 8,
    ALLOC_FAILED = 9,
    CFG_ALLOC_FAILED = 10,
    CODEC_ERROR = 11,
    FAILED_CLOSE = 12
} vp9a_error_t;

typedef WRMA_DLL_API struct vp9a_encode_ctx{

    char* outpath;
    void* (*write_callback)(void);
    vpx_image_t* (*func_frameload)(vpx_image_t*, void*);
    vp9a_error_t(*writerMethod)(struct vp9a_encode_ctx*, const vpx_codec_cx_pkt_t*);

    union WriteTarget {
        VpxVideoWriter* writer;
        ubyte* buffer_ptr;
    } target;


    vid_info_t info; //36
    int bytes_per_frame;
    uint64_t time_pos; //Time coord of last frame copied to output
    int frames_written;
    int frames_per_callback;
    
    vpx_image_t img_container; //136 bytes
    vpx_codec_ctx_t codec; //56
    vpx_codec_enc_cfg_t cfg; //504
    VpxVideoInfo vpxinfo; //20

   // int bitrate;
   // int keyframe_interval;
    uint32_t amt_written; //Marks how much was written from the last packet

    //Codec info
    colorModel_t input_clr;
    pixfmt_t input_fmt;
    ubyte use_vp9; //Flag
    ubyte is_lossless; //Flag
    ubyte sixteen; //16 bits per channel/pixel
    ubyte fullcolor;

    //So know what to call vpx lib for release on close/free
    ubyte init_img_cont;
    ubyte init_codec;
    ubyte init_cfg;

    ubyte last_was_key; //Last frame copied to output was a keyframe
    //Add new smaller fields here []

    //Error
    vp9a_error_t error_code;

} vp9a_encode_ctx_t;

//vp9a_encode_ctx_t* vp9a_openEncoder(char* outpath, vid_info_t* info);
WRMA_DLL_API vp9a_encode_ctx_t* vp9a_openFileEncoder(const char* outpath, vid_info_t* info);
WRMA_DLL_API vp9a_encode_ctx_t* vp9a_openRGBFileEncoder(const char* outpath, vid_info_t* info);
WRMA_DLL_API vp9a_encode_ctx_t* vp9a_openARGBFileEncoder(const char* outpath, vid_info_t* info);
WRMA_DLL_API vp9a_encode_ctx_t* vp9a_openYUV420FileEncoder(const char* outpath, vid_info_t* info);
WRMA_DLL_API vp9a_encode_ctx_t* vp9a_openYUV422FileEncoder(const char* outpath, vid_info_t* info);
WRMA_DLL_API vp9a_encode_ctx_t* vp9a_openYUV444FileEncoder(const char* outpath, vid_info_t* info);

WRMA_DLL_API vp9a_encode_ctx_t* vp9a_openStreamEncoder(ubyte* buffer, vid_info_t* info);
WRMA_DLL_API vp9a_encode_ctx_t* vp9a_openRGBStreamEncoder(ubyte* buffer, vid_info_t* info);
WRMA_DLL_API vp9a_encode_ctx_t* vp9a_openARGBStreamEncoder(ubyte* buffer, vid_info_t* info);
WRMA_DLL_API vp9a_encode_ctx_t* vp9a_openYUV420StreamEncoder(ubyte* buffer, vid_info_t* info);
WRMA_DLL_API vp9a_encode_ctx_t* vp9a_openYUV422StreamEncoder(ubyte* buffer, vid_info_t* info);
WRMA_DLL_API vp9a_encode_ctx_t* vp9a_openYUV444StreamEncoder(ubyte* buffer, vid_info_t* info);

WRMA_DLL_API int write_frame(vp9a_encode_ctx_t* ctx, ubyte* data);
WRMA_DLL_API vp9a_error_t vp9a_closeEncoder(vp9a_encode_ctx_t* ctx);

WRMA_DLL_API vpx_image_t* readRGBFrame(vpx_image_t* container, void* data);
WRMA_DLL_API vpx_image_t* readARGBFrame(vpx_image_t* container, void* data);
WRMA_DLL_API vpx_image_t* readPlanarFrame(vpx_image_t* container, void* data);

//WRMA_DLL_API int vp9a_freeError(vp9a_encode_ctx_t* ctx);
WRMA_DLL_API void setTimebaseFromFramerate(vid_info_t* info);

#ifdef __cplusplus
}
#endif

#endif // VP9ADAPTER_H_INCLUDED