#ifndef VP9ADAPTER_H_INCLUDED
#define VP9ADAPTER_H_INCLUDED

#include <stdio.h>

#include "libvpx/vpx/vpx_image.h"
#include "libvpx/vpx/vpx_encoder.h"
#include "libvpx/vpx/vp8cx.h"
#include "libvpx/vp9/common/vp9_common.h"
#include "libvpx/tools_common.h"
#include "libvpx/video_writer.h"
#include "videoDefs.h"

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

typedef enum colorModel{
    RGB = 0,
    ARGB = 1,
    BT601 = 2,
    BT709 = 3,
    BT2020 = 4,
    SETME = 7
} colorModel_t;

typedef enum pixfmt{
    STANDARD = 0,
    YUV_I420 = 1,
    YUV_I422 = 2,
    PLANAR_444 = 3,
    SETME = 7
} pixfmt_t;

typedef enum vp9a_error{
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

typedef struct vp9a_encode_ctx{

    char* outpath;
    //FILE* fhandle;

    vid_info_t* info;

    int frames_written;
    int frames_per_callback;
    void* (*write_callback)();

    //Codec info
    colorModel_t input_clr;
    pixfmt_t input_fmt;
    boolean use_vp9; //Flag
    boolean is_lossless; //Flag
    boolean sixteen; //16 bits per channel/pixel
    boolean fullcolor;

    int bitrate;
    int keyframe_interval;

    //Codec interface
    vpx_image_t* (*func_frameload)(vpx_image_t*, void*);
    vpx_image_t* img_container;
    vpx_codec_ctx_t* codec;
    VpxVideoInfo* vpxinfo;
    vpx_codec_enc_cfg_t* cfg;
    VpxVideoWriter* writer;

    //Error
    vp9a_error_t error_code;

} vp9a_encode_ctx_t;

//vp9a_encode_ctx_t* vp9a_openEncoder(char* outpath, vid_info_t* info);
vp9a_encode_ctx_t* vp9a_openRGBEncoder(char* outpath, vid_info_t* info);
vp9a_encode_ctx_t* vp9a_openARGBEncoder(char* outpath, vid_info_t* info);
vp9a_encode_ctx_t* vp9a_openYUV420Encoder(char* outpath, vid_info_t* info);
vp9a_encode_ctx_t* vp9a_openYUV422Encoder(char* outpath, vid_info_t* info);
vp9a_encode_ctx_t* vp9a_openYUV444Encoder(char* outpath, vid_info_t* info);

int write_frame(vp9a_encode_ctx_t* ctx, byte* data);
int vp9a_closeEncoder(vp9a_encode_ctx_t* ctx);

vpx_image_t* readRGBFrame(vpx_image_t* container, void* data);
vpx_image_t* readARGBFrame(vpx_image_t* container, void* data);
vpx_image_t* readPlanarFrame(vpx_image_t* container, void* data);

void setTimebaseFromFramerate(vid_info_t* info);


#endif // VP9ADAPTER_H_INCLUDED