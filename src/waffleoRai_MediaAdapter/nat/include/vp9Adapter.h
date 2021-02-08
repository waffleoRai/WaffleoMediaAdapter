#ifndef VP9ADAPTER_H_INCLUDED
#define VP9ADAPTER_H_INCLUDED

#include <stdio.h>

#include "libvpx/vpx/vpx_image.h"
#include "videoDefs.h"

//Pass a flags field with encoding info
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
        5-7 Pixel Format
            0   By pixel (8 bits per channel ARGB, RGB, or YUV)
            1   YUV I420
            2   YUV I422
            3   Planar 444

*/

enum colorModel{
    RGB = 0,
    ARGB = 1,
    BT601 = 2,
    BT701 = 3,
    BT2020 = 4
};

enum pixfmt{
    STANDARD = 0,
    YUV_I420 = 1,
    YUV_I422 = 2,
    PLANAR_444 = 3
};

enum vp9a_error{
    NO_ERROR = 0,
    UNSUPPORTED_FMT = 1
};

typedef struct vp9a_encode_ctx{

    char* outpath;
    //FILE* str_handle;

    vid_info_t* info;

    int frames_written;
    int frames_per_callback;
    //(Frame write callback ptr)

    //Codec info
    colorModel input_clr;
    pixfmt input_fmt;
    uchar use_vp9; //Flag
    uchar is_lossless; //Flag
    int bitrate;
    int keyframe_interval;

    //Codec interface
    //(Frame conversion callback)

    //Error
    vp9a_error error_code;

} vp9a_encode_ctx_t;

//vp9a_encode_ctx_t* vp9a_openEncoder(char* outpath, vid_info_t* info);
vp9a_encode_ctx_t* vp9a_openRGBEncoder(char* outpath, vid_info_t* info);
vp9a_encode_ctx_t* vp9a_openARGBEncoder(char* outpath, vid_info_t* info);
vp9a_encode_ctx_t* vp9a_openYUV420Encoder(char* outpath, vid_info_t* info);
vp9a_encode_ctx_t* vp9a_openYUV422Encoder(char* outpath, vid_info_t* info);

int vp9a_closeEncoder(vp9a_encode_ctx_t* ctx);

vpx_image_t* readRGBFrame(void* data, int width, int height);
vpx_image_t* readARGBFrame(void* data, int width, int height);
vpx_image_t* readYUV420Frame(void* data, int width, int height);
vpx_image_t* readYUV422Frame(void* data, int width, int height);


#endif // VP9ADAPTER_H_INCLUDED