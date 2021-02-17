
#include "vp9Adapter.h"

#define FALSE 0
#define TRUE 1


/*----- Open/Alloc -----*/

vp9a_encode_ctx_t* alloc_ctx(char* outpath, vid_info_t* info){

    vp9a_encode_ctx_t* myctx = (vp9a_encode_ctx_t*)malloc(sizeof(vp9a_encode_ctx_t)); //Allocate

    //Set args
    myctx->outpath = outpath;
    myctx->info = info;
    myctx->frames_written = 0;
    myctx->frames_per_callback = -1;
    myctx->write_callback = NULL;
    myctx->bitrate = info->bitrate;
    myctx->keyframe_interval = info->keyintr;
    myctx->func_frameload = NULL;
    myctx->img_container = NULL;
    myctx->codec = NULL;
    myctx->cfg = NULL;
    myctx->writer = NULL;
    myctx->error_code = NO_ERROR;

    //Check parameters...
    if(info->width <= 0 || info->width % 2 != 0 || info->height <= 0 || info->height % 2 != 0){
        myctx->error_code = INVALID_FRAME_SIZE;
        return myctx;
    }
    if(info->fps <= 0 || info->timebase_n <= 0 || info->timebase_d <= 0){
        myctx->error_code = INVALID_FRAMERATE;
        return myctx;
    }

    //Flags
    myctx->use_vp9 = (info->flags & 0x1 != 0);
    myctx->is_lossless = (info->flags & 0x2 != 0);
    myctx->sixteen = (info->flags & 0x10 != 0);
    myctx->fullcolor = (info->flags & 0x20 != 0);
    if(!myctx->use_vp9 && myctx->is_lossless){
        //No lossless VP8 afaik, so throw error.
        myctx->error_code = UNSUPPORTED_FMT;
        return myctx;
    }

    //Color model/data format
    myctx->input_clr = ((info->flags & 0x1c) >> 2);
    myctx->input_fmt = ((info->flags & 0xe0) >> 5);
    //Check for incompatible combinations
    //TODO maybe later...

    //Open file
    //if(!(myctx->fhandle = fopen(myctx->outpath, "rb"))) myctx->error_code = FILE_IO_ERROR;

    return myctx;
}

const VpxInterface* gen_codec_info(vp9a_encode_ctx_t* myctx){
    myctx->vpxinfo = (VpxVideoInfo*)malloc(sizeof(VpxVideoInfo));

    const VpxInterface* encoder = NULL;
    if(myctx->info->flags & 0x1 != 0) encoder = get_vpx_encoder_by_name("vp9");
    else encoder = get_vpx_encoder_by_name("vp8");
    if(!encoder){
        myctx->error_code = CODEC_NOT_FOUND;
        return encoder;
    }
    memset(myctx->vpxinfo, 0, sizeof(VpxVideoInfo));
    myctx->vpxinfo->codec_fourcc = encoder->fourcc;
    myctx->vpxinfo->frame_width = myctx->info->width;
    myctx->vpxinfo->frame_height = myctx->info->height;
    myctx->vpxinfo->time_base.numerator = myctx->info->timebase_n;
    myctx->vpxinfo->time_base.denominator = myctx->info->timebase_d;

    return encoder;
}

vp9a_error_t configAndInit(vp9a_encode_ctx_t* myctx, VpxInterface* encoder){
   
    //Config
    myctx->cfg = (vpx_codec_enc_cfg_t*)malloc(sizeof(vpx_codec_enc_cfg_t));
    if(!myctx->cfg){myctx->error_code = ALLOC_FAILED; return ALLOC_FAILED;}
    vpx_codec_err_t result = vpx_codec_enc_config_default(encoder->codec_interface(), myctx->cfg, 0);
    if(!result){myctx->error_code = CFG_ALLOC_FAILED; return CFG_ALLOC_FAILED;}

    myctx->cfg->g_w = myctx->vpxinfo->frame_width;
    myctx->cfg->g_h = myctx->vpxinfo->frame_height;
    myctx->cfg->g_timebase.num = myctx->vpxinfo->time_base.numerator;
    myctx->cfg->g_timebase.den = myctx->vpxinfo->time_base.denominator;
    if(!myctx->is_lossless) myctx->cfg->rc_target_bitrate = myctx->bitrate;

    //Open writer
    myctx->writer = vpx_video_writer_open(myctx->outpath, kContainerIVF, myctx->vpxinfo);
    if(!myctx->writer){myctx->error_code = FILE_IO_ERROR; return FILE_IO_ERROR;}

    //Init codec
    if (vpx_codec_enc_init(myctx->codec, encoder->codec_interface(), myctx->cfg, 0)){
        myctx->error_code = CODEC_ERROR; return CODEC_ERROR;
    }
    if(myctx->is_lossless){
        if (vpx_codec_control_(myctx->codec, VP9E_SET_LOSSLESS, 1)){
            myctx->error_code = CODEC_ERROR; return CODEC_ERROR;
        }
    }

    return NO_ERROR;
}

vp9a_encode_ctx_t* vp9a_openRGBEncoder(char* outpath, vid_info_t* info){
    //This uses the examples in the libvpx documentation

    //Allocate context and copy info to it
    if(outpath == NULL || info == NULL) return NULL;
    vp9a_encode_ctx_t* myctx = alloc_ctx(outpath, info);

    //Check format enums to make sure they match...
    if(myctx->input_fmt != STANDARD){
        if(myctx->input_fmt == SETME) myctx->input_fmt = STANDARD;
        else{
            myctx->error_code = FMT_MISMATCH;
            return myctx;
        }
    }
    if(myctx->input_clr != RGB){
        if(myctx->input_fmt == SETME) myctx->input_clr = RGB;
        else{
            myctx->error_code = FMT_MISMATCH;
            return myctx;
        }
    }

    //Generate codec info
    const VpxInterface* encoder = gen_codec_info(myctx);
    if(!encoder) return myctx;
    
    //Data gets rearranged to planar when read in
    myctx->img_container = !vpx_img_alloc(NULL, VPX_IMG_FMT_I444, info->width, info->height, 1);
    if(!myctx->img_container){
        myctx->error_code = IMGALLOC_FAILED;
        return myctx;
    }

    //Set color space
    myctx->img_container->cs = VPX_CS_SRGB;
    myctx->img_container->range = VPX_CR_FULL_RANGE;

    //Config
    if(configAndInit(myctx, encoder) != NO_ERROR) return myctx;
    myctx->func_frameload = readRGBFrame;
  
    return myctx;
}

vp9a_encode_ctx_t* vp9a_openARGBEncoder(char* outpath, vid_info_t* info){
    //This uses the examples in the libvpx documentation

    //Allocate context and copy info to it
    if(outpath == NULL || info == NULL) return NULL;
    vp9a_encode_ctx_t* myctx = alloc_ctx(outpath, info);

    //Check format enums to make sure they match...
    if(myctx->input_fmt != STANDARD){
        if(myctx->input_fmt == SETME) myctx->input_fmt = STANDARD;
        else{
            myctx->error_code = FMT_MISMATCH;
            return myctx;
        }
    }
    if(myctx->input_clr != ARGB){
        if(myctx->input_fmt == SETME) myctx->input_clr = ARGB;
        else{
            myctx->error_code = FMT_MISMATCH;
            return myctx;
        }
    }

    //Generate codec info
    const VpxInterface* encoder = gen_codec_info(myctx);
    if(!encoder) return myctx;
    
    //Data gets rearranged to planar when read in
    myctx->img_container = !vpx_img_alloc(NULL, VPX_IMG_FMT_I444, info->width, info->height, 1);
    if(!myctx->img_container){
        myctx->error_code = IMGALLOC_FAILED;
        return myctx;
    }

    //Set color space
    myctx->img_container->cs = VPX_CS_SRGB;
    myctx->img_container->range = VPX_CR_FULL_RANGE;

    //Config
    if(configAndInit(myctx, encoder) != NO_ERROR) return myctx;
    myctx->func_frameload = readARGBFrame;
  
    return myctx;
}

vp9a_encode_ctx_t* vp9a_openYUV420Encoder(char* outpath, vid_info_t* info){
    //This uses the examples in the libvpx documentation

    //Allocate context and copy info to it
    if(outpath == NULL || info == NULL) return NULL;
    vp9a_encode_ctx_t* myctx = alloc_ctx(outpath, info);

    //Check format enums to make sure they match...
    if(myctx->input_fmt != YUV_I420){
        if(myctx->input_fmt == SETME) myctx->input_fmt = YUV_I420;
        else{
            myctx->error_code = FMT_MISMATCH;
            return myctx;
        }
    }
    if(!(myctx->input_clr == BT601 || myctx->input_clr == BT709 || myctx->input_clr == BT2020)){
        if(myctx->input_fmt == SETME) myctx->input_clr = BT601;
        else{
            myctx->error_code = FMT_MISMATCH;
            return myctx;
        }
    }

    //Generate codec info
    const VpxInterface* encoder = gen_codec_info(myctx);
    if(!encoder) return myctx;
    
    vpx_img_fmt_t fmt = myctx->sixteen?VPX_IMG_FMT_I42016:VPX_IMG_FMT_I420;
    myctx->img_container = !vpx_img_alloc(NULL, fmt, info->width, info->height, 1);
    if(!myctx->img_container){
        myctx->error_code = IMGALLOC_FAILED;
        return myctx;
    }

    //Set color space
    switch(myctx->input_clr){
        case BT601:
            myctx->img_container->cs = VPX_CS_BT_601;
            break;
        case BT709:
            myctx->img_container->cs = VPX_CS_BT_709;
            break;
        case BT2020:
            myctx->img_container->cs = VPX_CS_BT_2020;
            break;
        default:
            myctx->error_code = FMT_MISMATCH;
            return myctx;
    }
    myctx->img_container->range = myctx->fullcolor?VPX_CR_FULL_RANGE:VPX_CR_STUDIO_RANGE;

    //Config
    if(configAndInit(myctx, encoder) != NO_ERROR) return myctx;
    myctx->func_frameload = readPlanarFrame;
  
    return myctx;
}

vp9a_encode_ctx_t* vp9a_openYUV422Encoder(char* outpath, vid_info_t* info){
    //This uses the examples in the libvpx documentation

    //Allocate context and copy info to it
    if(outpath == NULL || info == NULL) return NULL;
    vp9a_encode_ctx_t* myctx = alloc_ctx(outpath, info);

    //Check format enums to make sure they match...
    if(myctx->input_fmt != YUV_I422){
        if(myctx->input_fmt == SETME) myctx->input_fmt = YUV_I422;
        else{
            myctx->error_code = FMT_MISMATCH;
            return myctx;
        }
    }
    if(!(myctx->input_clr == BT601 || myctx->input_clr == BT709 || myctx->input_clr == BT2020)){
        if(myctx->input_fmt == SETME) myctx->input_clr = BT601;
        else{
            myctx->error_code = FMT_MISMATCH;
            return myctx;
        }
    }

    //Generate codec info
    const VpxInterface* encoder = gen_codec_info(myctx);
    if(!encoder) return myctx;
    
    vpx_img_fmt_t fmt = myctx->sixteen?VPX_IMG_FMT_I42216:VPX_IMG_FMT_I422;
    myctx->img_container = !vpx_img_alloc(NULL, fmt, info->width, info->height, 1);
    if(!myctx->img_container){
        myctx->error_code = IMGALLOC_FAILED;
        return myctx;
    }

    //Set color space
    switch(myctx->input_clr){
        case BT601:
            myctx->img_container->cs = VPX_CS_BT_601;
            break;
        case BT709:
            myctx->img_container->cs = VPX_CS_BT_709;
            break;
        case BT2020:
            myctx->img_container->cs = VPX_CS_BT_2020;
            break;
        default:
            myctx->error_code = FMT_MISMATCH;
            return myctx;
    }
    myctx->img_container->range = myctx->fullcolor?VPX_CR_FULL_RANGE:VPX_CR_STUDIO_RANGE;

    //Config
    if(configAndInit(myctx, encoder) != NO_ERROR) return myctx;
    myctx->func_frameload = readPlanarFrame;
  
    return myctx;
}

vp9a_encode_ctx_t* vp9a_openYUV444Encoder(char* outpath, vid_info_t* info){
    //This uses the examples in the libvpx documentation

    //Allocate context and copy info to it
    if(outpath == NULL || info == NULL) return NULL;
    vp9a_encode_ctx_t* myctx = alloc_ctx(outpath, info);

    //Check format enums to make sure they match...
    if(myctx->input_fmt != PLANAR_444){
        if(myctx->input_fmt == SETME) myctx->input_fmt = PLANAR_444;
        else{
            myctx->error_code = FMT_MISMATCH;
            return myctx;
        }
    }
    if(!(myctx->input_clr == BT601 || myctx->input_clr == BT709 || myctx->input_clr == BT2020)){
        if(myctx->input_fmt == SETME) myctx->input_clr = BT601;
        else{
            myctx->error_code = FMT_MISMATCH;
            return myctx;
        }
    }

    //Generate codec info
    const VpxInterface* encoder = gen_codec_info(myctx);
    if(!encoder) return myctx;
    
    vpx_img_fmt_t fmt = myctx->sixteen?VPX_IMG_FMT_I44416:VPX_IMG_FMT_I444;
    myctx->img_container = !vpx_img_alloc(NULL, fmt, info->width, info->height, 1);
    if(!myctx->img_container){
        myctx->error_code = IMGALLOC_FAILED;
        return myctx;
    }

    //Set color space
    switch(myctx->input_clr){
        case BT601:
            myctx->img_container->cs = VPX_CS_BT_601;
            break;
        case BT709:
            myctx->img_container->cs = VPX_CS_BT_709;
            break;
        case BT2020:
            myctx->img_container->cs = VPX_CS_BT_2020;
            break;
        default:
            myctx->error_code = FMT_MISMATCH;
            return myctx;
    }
    myctx->img_container->range = myctx->fullcolor?VPX_CR_FULL_RANGE:VPX_CR_STUDIO_RANGE;

    //Config
    if(configAndInit(myctx, encoder) != NO_ERROR) return myctx;
    myctx->func_frameload = readPlanarFrame;

    return myctx;
}

/*----- Write/Close -----*/

int write_frame(vp9a_encode_ctx_t* ctx, byte* data){

    //Wrap data
    boolean any_written = FALSE;
    if(ctx->func_frameload == NULL) return 1;
    if(ctx->func_frameload(ctx->img_container, data) == NULL) return 2;

    //Do encoding
    vpx_codec_iter_t iter = NULL;
    const vpx_codec_cx_pkt_t *pkt = NULL;

    int flags = 0;
    if(ctx->keyframe_interval > 0 && ctx->frames_written % ctx->keyframe_interval == 0){
        flags |= VPX_EFLAG_FORCE_KF;
    }

    const vpx_codec_err_t res = vpx_codec_encode(ctx->codec, ctx->img_container, ctx->frames_written++, 1, flags, VPX_DL_GOOD_QUALITY);
    if (res != VPX_CODEC_OK) return 3;
  
    while ((pkt = vpx_codec_get_cx_data(ctx->codec, &iter)) != NULL) {
        any_written = TRUE;
        if (pkt->kind == VPX_CODEC_CX_FRAME_PKT) {
            const int keyframe = (pkt->data.frame.flags & VPX_FRAME_IS_KEY) != 0;
            if (!vpx_video_writer_write_frame(ctx->writer, pkt->data.frame.buf, pkt->data.frame.sz, pkt->data.frame.pts)) {
                return 4;
            }
        }
   }
  
    //Check for callback
    if(ctx->write_callback != NULL && ctx->frames_per_callback > 0 && ctx->frames_written % ctx->frames_per_callback == 0){
        ctx->write_callback();
    }

   return any_written?0:-1;
}

boolean flush_encoder(vp9a_encode_ctx_t* ctx){
    vpx_codec_iter_t iter = NULL;
    const vpx_codec_cx_pkt_t *pkt = NULL;
    boolean haspkts = FALSE;

    do{
        haspkts = FALSE;
        const vpx_codec_err_t res = vpx_codec_encode(ctx->codec, NULL, -1, 1, 0, VPX_DL_GOOD_QUALITY);
        if (res != VPX_CODEC_OK) return FALSE;

        while ((pkt = vpx_codec_get_cx_data(ctx->codec, &iter)) != NULL) {
            haspkts = TRUE;
  
            if (pkt->kind == VPX_CODEC_CX_FRAME_PKT) {
                if (!vpx_video_writer_write_frame(ctx->writer, pkt->data.frame.buf, pkt->data.frame.sz, pkt->data.frame.pts)) {return FALSE;}
                fflush(stdout);
            }
        }

    }while(haspkts);

    return TRUE;
}

int vp9a_closeEncoder(vp9a_encode_ctx_t* ctx){

    //Release vpx resources
    if(!flush_encoder(ctx)) ctx->error_code = FAILED_CLOSE;
    vpx_img_free(ctx->img_container);
    if (vpx_codec_destroy(ctx->codec)) ctx->error_code = FAILED_CLOSE;
    vpx_video_writer_close(ctx->writer);

    //vpxinfo and vpxcfg must also be freed as they are malloc'd by open protocol
    free(ctx->vpxinfo);
    free(ctx->cfg);

    //Release adapter resources
    free(ctx);

    return ctx->error_code != NO_ERROR;
}

/*----- Frame Wrapping/Conversion -----*/

vpx_image_t* readRGBFrame(vpx_image_t* container, void* data){
    if(container == NULL) return NULL;
    if(data == NULL) return NULL;

    int plane;

    for (plane = 0; plane < 3; plane++) {
        unsigned char* datapos = (unsigned char*)data + plane;
        unsigned char* buf = container->planes[plane];
        const int w = vpx_img_plane_width(container, plane);
        const int h = vpx_img_plane_height(container, plane);
        const int pixcount = w*h;

        int p;
        for(p = 0; p < pixcount; p++){
            *(buf++) = *(datapos);
            datapos += 3;
        }

    }

    return container;
}

vpx_image_t* readARGBFrame(vpx_image_t* container, void* data){
    //Right now, it ignores the alpha channel. Will add when can figure out how (doesn't seem compatible with VPX)
    if(container == NULL) return NULL;
    if(data == NULL) return NULL;

    int plane;

    for (plane = 0; plane < 3; plane++) {
        unsigned char* datapos = (unsigned char*)data + 1 + plane; //Skips A phase
        unsigned char* buf = container->planes[plane];
        const int w = vpx_img_plane_width(container, plane);
        const int h = vpx_img_plane_height(container, plane);
        const int pixcount = w*h;

        int p;
        for(p = 0; p < pixcount; p++){
            *(buf++) = *(datapos);
            datapos += 4;
        }

    }

    return container;
}

//Modified version of vpx_img_read that deals with data in memory instead of loading from FILE
vpx_image_t* readPlanarFrame(vpx_image_t* container, void* data){
    if(container == NULL) return NULL;
    if(data == NULL) return NULL;

    int plane;
    unsigned char* datapos = (unsigned char*)data;

    /* At some point for speed, I can probably just make the containers planes
    point to the existing data instead of copying all the data. But for now I don't want to mess
    with vpx structures that directly, especially since they have their own alloc and stuff.
    */
    for (plane = 0; plane < 3; plane++) {
        unsigned char* buf = container->planes[plane];
        const int stride = container->stride[plane];
        const int w = vpx_img_plane_width(container, plane) *
                    ((container->fmt & VPX_IMG_FMT_HIGHBITDEPTH) ? 2 : 1);
        const int h = vpx_img_plane_height(container, plane);
        int y;

        for (y = 0; y < h; y++) {
            //if (fread(buf, 1, w, file) != (size_t)w) return NULL;
            memcpy(buf, datapos, (size_t)w);
            buf += stride; datapos += stride;
        }
    }

    return container;
}

/*----- Misc. Utils -----*/

void setTimebaseFromFramerate(vid_info_t* info){
    if(info == NULL) return;
    if(roundf(info->fps) == info->fps){
        //Integer framerate.
        info->timebase_n = 1;
        info->timebase_d = roundf(info->fps);
    }
    else{
        //ehhh f if I know.
        //Best I can think of is check for common ones?
        //Otherwise, just have to get program to pass n/d in the first place...
        //TODO
    }

}
