#ifndef WRMA_VIDDEFS_H_INCLUDED
#define WRMA_VIDDEFS_H_INCLUDED

#include "mediaadapterDefs.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef WRMA_DLL_API struct argb_pixel {
        uint8_t alpha;
        uint8_t red;
        uint8_t green;
        uint8_t blue;
    } argb_pixel_t;

    typedef WRMA_DLL_API struct rgb_pixel {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
    } rgb_pixel_t;

    typedef WRMA_DLL_API struct argb_img {

        int width;
        int height;

        argb_pixel_t* data;

    } argb_img_t;

    typedef WRMA_DLL_API struct rgb_img {

        int width;
        int height;

        rgb_pixel_t* data;

    } rgb_img_t;

    typedef WRMA_DLL_API struct vid_info {

        int width;
        int height;

        float fps;
        int timebase_n;
        int timebase_d;
        int total_frames;

        uint16_t flags;
        int32_t bitrate;
        int32_t keyintr;

    } vid_info_t;

    typedef WRMA_DLL_API struct aud_info {

        uint32_t sample_rate;
        uint32_t bit_depth;
        uint8_t channels;

        uint32_t frame_count;
        bool interleaved;

    } aud_info_t;

    typedef WRMA_DLL_API struct vid_codec_args {

        int argc;
        char** argv;

    } vid_codec_args_t;

    typedef WRMA_DLL_API enum vid_frame_rate {

        FPS_UNKNOWN,
        FPS_14_985,
        FPS_15,
        FPS_20,
        FPS_23_976,
        FPS_24,
        FPS_25,
        FPS_29_97,
        FPS_30,
        FPS_50,
        FPS_59_94,
        FPS_60

    } vid_frame_rate_t;

#ifdef __cplusplus
}
#endif

#endif // WRMA_VIDDEFS_H_INCLUDED