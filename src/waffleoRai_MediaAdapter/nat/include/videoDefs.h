#ifndef WRMA_VIDDEFS_H_INCLUDED
#define WRMA_VIDDEFS_H_INCLUDED

#include "quickDefs.h"

typedef struct argb_pixel{
    uint8_t alpha;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} argb_pixel_t;

typedef struct rgb_pixel{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} rgb_pixel_t;

typedef struct argb_img{

    int width;
    int height;

    argb_pixel_t* data;

} argb_img_t;

typedef struct rgb_img{

    int width;
    int height;

    rgb_pixel_t* data;

} rgb_img_t;

typedef struct vid_info{

    int width;
    int height;

    float fps;
    int total_frames;

    byte flags;
    int32_t bitrate;
    int32_t keyintr;

} vid_info_t;

typedef struct vid_codec_args{

    int argc;
    char** argv;

} vid_codec_args_t;

#endif // WRMA_VIDDEFS_H_INCLUDED