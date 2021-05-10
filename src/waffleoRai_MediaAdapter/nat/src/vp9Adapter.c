
#include "vp9Adapter.h"

#define FALSE 0
#define TRUE 1

/*----- Debug -----*/

void debug_printVPCTX(vp9a_encode_ctx_t* ctx) {

    if (!ctx) {
        printf("VPCTX is NULL!\n");
        return;
    }

    printf("VPCTX @ %p\n", ctx);
    if (!ctx->outpath) printf("\tOutput Path: NULL\n");
    else printf("\tOutput Path: (@ %p) \"%s\"\n", ctx->outpath, ctx->outpath);
    if (!ctx->write_callback) printf("\tOn Write Callback: NULL\n");
    else printf("\tOn Write Callback: @ %p\n", ctx->write_callback);
    if (!ctx->func_frameload) printf("\tFrame Load Function: NULL\n");
    else printf("\tFrame Load Function: @ %p\n", ctx->func_frameload);
    if (!ctx->writerMethod) printf("\tWriter Method: NULL\n");
    else printf("\tWriter Method: @ %p\n", ctx->writerMethod);
    if (!ctx->target.writer) printf("\tWrite Target (Writer): NULL\n");
    else printf("\tWrite Target (Writer): @ %p\n", ctx->target.writer);
    if (!ctx->target.buffer_ptr) printf("\tWrite Target (Buffer): NULL\n");
    else printf("\tWrite Target (Buffer): @ %p\n", ctx->target.buffer_ptr);

    printf("\tInfo-- (@ %p)\n", &ctx->info);
    printf("\t\tWidth: %d\n", ctx->info.width);
    printf("\t\tHeight: %d\n", ctx->info.height);
    printf("\t\tFramerate: %f frames per second\n", ctx->info.fps);
    printf("\t\tTimebase: %d/%d\n", ctx->info.timebase_n, ctx->info.timebase_d);
    printf("\t\tFrame Count: %d\n", ctx->info.total_frames);
    printf("\t\tFlags: %04x\n", ctx->info.flags);
    printf("\t\tBitrate: %d\n", ctx->info.bitrate);
    printf("\t\tKeyframe Interval: %d\n", ctx->info.keyintr);

    printf("\tBytes per Frame: %d\n", ctx->bytes_per_frame);
    printf("\tTime Position: %lld\n", ctx->time_pos);
    printf("\tFrames Written: %d\n", ctx->frames_written);
    printf("\tFrames per Callback: %d\n", ctx->frames_per_callback);

    printf("\tImage Container Init?: %d\n", ctx->init_img_cont);
    printf("\tImage Container (@ %p)\n", &ctx->img_container);
    printf("\tCodec Init?: %d\n", ctx->init_codec);
    printf("\tVPX Codec Struct (@ %p)\n", &ctx->codec);
    printf("\tConfig Init?: %d\n", ctx->init_cfg);
    printf("\tVPX Config Struct (@ %p)\n", &ctx->cfg);
    printf("\tVPX Info Struct (@ %p)\n", &ctx->vpxinfo);

    printf("\tWritten in Last Packet: %x\n", ctx->amt_written);
    printf("\tColor Model: %d\n", ctx->input_clr);
    printf("\tPixel Format: %d\n", ctx->input_fmt);
    if (ctx->use_vp9) printf("\tCodec: VP9\n");
    else printf("\tCodec: VP8\n");
    printf("\tLossless: %d\n", ctx->is_lossless);
    printf("\t16-bit Input: %d\n", ctx->sixteen);
    printf("\tFull Color: %d\n", ctx->fullcolor);

    printf("\tCurrent Error: %d\n", ctx->error_code);
    printf("------------------------------------\n");
}

/*----- Writer Methods -----*/

vp9a_error_t writeToWriter(vp9a_encode_ctx_t* ctx, const vpx_codec_cx_pkt_t* pkt) {
    ctx->last_was_key = (pkt->data.frame.flags & VPX_FRAME_IS_KEY) != 0;
    if (!vpx_video_writer_write_frame(ctx->target.writer, pkt->data.frame.buf, pkt->data.frame.sz, pkt->data.frame.pts)) {
        return CODEC_ERROR;
    }
    ctx->time_pos = pkt->data.frame.pts;
    ctx->amt_written = (uint32_t)pkt->data.frame.sz;

    return NO_ERROR;
}

vp9a_error_t writeToBuffer(vp9a_encode_ctx_t* ctx, const vpx_codec_cx_pkt_t* pkt) {
    if (!pkt) return CODEC_ERROR;
    ctx->last_was_key = (pkt->data.frame.flags & VPX_FRAME_IS_KEY) != 0;
    ctx->time_pos = pkt->data.frame.pts;

    memcpy(ctx->target.buffer_ptr, pkt->data.frame.buf, pkt->data.frame.sz);

    return NO_ERROR;
}

/*----- Open/Alloc -----*/

vp9a_encode_ctx_t* alloc_ctx(const char* outpath, vid_info_t* info) {

    vp9a_encode_ctx_t* myctx = (vp9a_encode_ctx_t*)malloc(sizeof(vp9a_encode_ctx_t)); //Allocate
    if (!myctx) return NULL;
    printf("\tMALLOC vp9a_encode_ctx_t @ %p || size: %llx\n", myctx, sizeof(vp9a_encode_ctx_t));
    printf("\t outpath @ %p || size: %llx || type size: %llx\n", &myctx->outpath, sizeof(myctx->outpath), sizeof(char*));
    printf("\t write_callback @ %p || size: %llx || type size: %llx\n", &myctx->write_callback, sizeof(myctx->write_callback), sizeof(void*));
    printf("\t func_frameload @ %p || size: %llx || type size: %llx\n", &myctx->func_frameload, sizeof(myctx->func_frameload), sizeof(void*));
    printf("\t writerMethod @ %p || size: %llx || type size: %llx\n", &myctx->writerMethod, sizeof(myctx->writerMethod), sizeof(void*));
    printf("\t target @ %p || size: %llx || type size: %llx\n", &myctx->target.writer, sizeof(myctx->target.writer), sizeof(VpxVideoWriter*));
    printf("\t info @ %p || size: %llx || type size: %llx\n", &myctx->info, sizeof(myctx->info), sizeof(vid_info_t));
    printf("\t bytes_per_frame @ %p || size: %llx || type size: %llx\n", &myctx->bytes_per_frame, sizeof(myctx->bytes_per_frame), sizeof(int));
    printf("\t time_pos @ %p || size: %llx || type size: %llx\n", &myctx->time_pos, sizeof(myctx->time_pos), sizeof(uint64_t));
    printf("\t frames_written @ %p || size: %llx || type size: %llx\n", &myctx->frames_written, sizeof(myctx->frames_written), sizeof(int));
    printf("\t frames_per_callback @ %p || size: %llx || type size: %llx\n", &myctx->frames_per_callback, sizeof(myctx->frames_per_callback), sizeof(int));
    printf("\t img_container @ %p || size: %llx || type size: %llx\n", &myctx->img_container, sizeof(myctx->img_container), sizeof(vpx_image_t));
    printf("\t codec @ %p || size: %llx || type size: %llx\n", &myctx->codec, sizeof(myctx->codec), sizeof(vpx_codec_ctx_t));
    printf("\t cfg @ %p || size: %llx || type size: %llx\n", &myctx->cfg, sizeof(myctx->cfg), sizeof(vpx_codec_enc_cfg_t));
    printf("\t vpxinfo @ %p || size: %llx || type size: %llx\n", &myctx->vpxinfo, sizeof(myctx->vpxinfo), sizeof(VpxVideoInfo));
    printf("\t amt_written @ %p || size: %llx || type size: %llx\n", &myctx->amt_written, sizeof(myctx->amt_written), sizeof(uint32_t));
    printf("\t input_clr @ %p || size: %llx || type size: %llx\n", &myctx->input_clr, sizeof(myctx->input_clr), sizeof(colorModel_t));
    printf("\t input_fmt @ %p || size: %llx || type size: %llx\n", &myctx->input_fmt, sizeof(myctx->input_fmt), sizeof(pixfmt_t));
    printf("\t use_vp9 @ %p || size: %llx || type size: %llx\n", &myctx->use_vp9, sizeof(myctx->use_vp9), sizeof(ubyte));
    printf("\t is_lossless @ %p || size: %llx || type size: %llx\n", &myctx->amt_written, sizeof(myctx->is_lossless), sizeof(ubyte));
    printf("\t sixteen @ %p || size: %llx || type size: %llx\n", &myctx->sixteen, sizeof(myctx->sixteen), sizeof(ubyte));
    printf("\t fullcolor @ %p || size: %llx || type size: %llx\n", &myctx->fullcolor, sizeof(myctx->fullcolor), sizeof(ubyte));
    printf("\t init_img_cont @ %p || size: %llx || type size: %llx\n", &myctx->init_img_cont, sizeof(myctx->init_img_cont), sizeof(ubyte));
    printf("\t init_codec @ %p || size: %llx || type size: %llx\n", &myctx->init_codec, sizeof(myctx->init_codec), sizeof(ubyte));
    printf("\t init_cfg @ %p || size: %llx || type size: %llx\n", &myctx->init_cfg, sizeof(myctx->init_cfg), sizeof(ubyte));
    printf("\t last_was_key @ %p || size: %llx || type size: %llx\n", &myctx->last_was_key, sizeof(myctx->last_was_key), sizeof(ubyte));
    printf("\t error_code @ %p || size: %llx || type size: %llx\n", &myctx->error_code, sizeof(myctx->error_code), sizeof(vp9a_error_t));

    //Zero.
    memset(myctx, 0, sizeof(vp9a_encode_ctx_t));
    //printf("\tMEMSET 0 vp9a_encode_ctx_t @ %p\n", myctx);

    //Set args
    //Copy string.
    if (outpath) {
        size_t slen = strlen(outpath);
        myctx->outpath = (char*)malloc(slen + 1);
        //printf("\tMALLOC char* @ %p\n", myctx->outpath);
        if (!myctx->outpath) {
            myctx->error_code = ALLOC_FAILED;
            return myctx;
        }
        strcpy(myctx->outpath, outpath);
    }
    //printf("Outpath: %s\n", myctx->outpath);

    //Set default values for other variables
    //(Don't need to set NULL/0 values since zero'd out the struct)
    myctx->info = *info;
    myctx->frames_per_callback = -1;
    //myctx->bitrate = info->bitrate;
    //myctx->keyframe_interval = info->keyintr;
    myctx->error_code = NO_ERROR;

    //Check parameters...
    if (myctx->info.width <= 0 || myctx->info.width % 2 != 0 || myctx->info.height <= 0 || myctx->info.height % 2 != 0) {
        myctx->error_code = INVALID_FRAME_SIZE;
        return myctx;
    }
    if (myctx->info.fps <= 0 || myctx->info.timebase_n <= 0 || myctx->info.timebase_d <= 0) {
        myctx->error_code = INVALID_FRAMERATE;
        return myctx;
    }

    //Flags
    myctx->use_vp9 = ((info->flags & 0x1) != 0);
    myctx->is_lossless = ((info->flags & 0x2) != 0);
    myctx->sixteen = ((info->flags & 0x10) != 0);
    myctx->fullcolor = ((info->flags & 0x20) != 0);
    if (!myctx->use_vp9 && myctx->is_lossless) {
        //No lossless VP8 afaik, so throw error.
        myctx->error_code = UNSUPPORTED_FMT;
        return myctx;
    }

    //Color model/data format
    myctx->input_clr = ((info->flags & 0x1c) >> 2);
    myctx->input_fmt = ((info->flags & 0xe0) >> 5);
    //Check for incompatible combinations
    //TODO maybe later...

    //debug_printVPCTX(myctx);

    return myctx;
}

const VpxInterface* gen_codec_info(vp9a_encode_ctx_t* myctx) {
    if (!myctx) return NULL;
   // myctx->vpxinfo = (VpxVideoInfo*)malloc(sizeof(VpxVideoInfo));
    //if (!myctx->vpxinfo) return NULL;

    const VpxInterface* encoder = NULL;
    if ((myctx->info.flags & 0x1) != 0) encoder = get_vpx_encoder_by_name("vp9");
    else encoder = get_vpx_encoder_by_name("vp8");
    if (!encoder) {
        myctx->error_code = CODEC_NOT_FOUND;
        return encoder;
    }
    //printf("\tOBTAIN VpxInterface @ %p\n", encoder);

    //memset(myctx->vpxinfo, 0, sizeof(VpxVideoInfo)); //Zero'd at ctx alloc
    myctx->vpxinfo.codec_fourcc = encoder->fourcc;
    myctx->vpxinfo.frame_width = myctx->info.width;
    myctx->vpxinfo.frame_height = myctx->info.height;
    myctx->vpxinfo.time_base.numerator = myctx->info.timebase_n;
    myctx->vpxinfo.time_base.denominator = myctx->info.timebase_d;

    //printf("--DEBUG-- gen_codec_info: encoder pointer: %p\n", encoder);
    return encoder;
}

vp9a_error_t configAndInit(vp9a_encode_ctx_t* myctx, const VpxInterface* encoder) {

    //Config
   // myctx->cfg = (vpx_codec_enc_cfg_t*)malloc(sizeof(vpx_codec_enc_cfg_t));
   // if (!myctx->cfg) { myctx->error_code = ALLOC_FAILED; return ALLOC_FAILED; }
   // printf("--DEBUG-- configAndInit: encoder pointer: %p\n", encoder);
    vpx_codec_err_t result = vpx_codec_enc_config_default(encoder->codec_interface(), &myctx->cfg, 0);
    if (result != VPX_CODEC_OK) { myctx->error_code = CFG_ALLOC_FAILED; return CFG_ALLOC_FAILED; }
    myctx->init_cfg = TRUE;
    //printf("\tSTRUCT_INIT vpx_codec_enc_cfg_t @ %p\n", &myctx->cfg);
   // printf("--DEBUG-- vpx_codec_enc_config_default returned. Error code: %d\n", myctx->error_code);

    myctx->cfg.g_w = myctx->vpxinfo.frame_width;
    myctx->cfg.g_h = myctx->vpxinfo.frame_height;
    myctx->cfg.g_timebase.num = myctx->vpxinfo.time_base.numerator;
    myctx->cfg.g_timebase.den = myctx->vpxinfo.time_base.denominator;
    if (!myctx->is_lossless) myctx->cfg.rc_target_bitrate = myctx->info.bitrate;
    //printf("--DEBUG-- config setting passed. Error code: %d\n", myctx->error_code);

    //Open writer (if applicable)
    if (myctx->outpath) {
        //printf("Opening writer. Target path: %s\n", myctx->outpath);
        myctx->target.writer = vpx_video_writer_open(myctx->outpath, kContainerIVF, &myctx->vpxinfo);
        if (!myctx->target.writer) { myctx->error_code = FILE_IO_ERROR; return FILE_IO_ERROR; }
        //printf("\tSTREAM_OPEN VpxVideoWriter @ %p\n", myctx->target.writer);
        myctx->writerMethod = &writeToWriter;
        //printf("--DEBUG-- writer opening passed. Error code: %d\n", myctx->error_code);
    }
    else myctx->writerMethod = &writeToBuffer;

    //Init codec
//TODO might need a flag here for VPX if 16-bit??
    //printf("Encoder Name: %s || Codec Interface @ %p\n", encoder->name, encoder->codec_interface);
    vpx_codec_flags_t codecflags = 0;
    if (myctx->sixteen) codecflags |= VPX_CODEC_USE_HIGHBITDEPTH;
    result = vpx_codec_enc_init(&myctx->codec, encoder->codec_interface(), &myctx->cfg, codecflags);
    if (result != VPX_CODEC_OK) {
        myctx->error_code = CODEC_ERROR; 
        printf("--DEBUG-- vpx_codec_enc_init failed. Adapter error code: %d || VPX error code: %d\n", myctx->error_code, result);
        return CODEC_ERROR;
    }
    myctx->init_codec = TRUE;
    //printf("--DEBUG-- vpx_codec_enc_init returned. Error code: %d\n", myctx->error_code);
    //printf("\tSTRUCT_INIT vpx_codec_ctx_t @ %p\n", &myctx->codec);
    if (myctx->is_lossless) {
        if (vpx_codec_control_(&myctx->codec, VP9E_SET_LOSSLESS, 1)) {
            myctx->error_code = CODEC_ERROR; return CODEC_ERROR;
        }
        //printf("--DEBUG-- lossless setting passed. Error code: %d\n", myctx->error_code);
    }

    //debug_printVPCTX(myctx);
    return NO_ERROR;
}

vp9a_encode_ctx_t* vp9a_openFileEncoder(const char* outpath, vid_info_t* info) {
    //Get format and switch on format...

    uint32_t pfmt = (uint32_t)info->flags;
    pfmt &= 0x00e0;
    pfmt >>= 5;

    pixfmt_t e = (pixfmt_t)pfmt;
    switch (e) {
    case STANDARD:
        return vp9a_openRGBFileEncoder(outpath, info);
    case YUV_I420:
        //printf("Format I420 detected\n");
        return vp9a_openYUV420FileEncoder(outpath, info);
    case YUV_I422:
        return vp9a_openYUV422FileEncoder(outpath, info);
    case PLANAR_444:
        return vp9a_openYUV444FileEncoder(outpath, info);
    }

    return NULL;
}

vp9a_encode_ctx_t* vp9a_openStreamEncoder(ubyte* buffer, vid_info_t* info) {
    uint32_t pfmt = (uint32_t)info->flags;
    pfmt &= 0x00e0;
    pfmt >>= 5;

    pixfmt_t e = (pixfmt_t)pfmt;
    switch (e) {
    case STANDARD:
        return vp9a_openRGBStreamEncoder(buffer, info);
    case YUV_I420:
        return vp9a_openYUV420StreamEncoder(buffer, info);
    case YUV_I422:
        return vp9a_openYUV422StreamEncoder(buffer, info);
    case PLANAR_444:
        return vp9a_openYUV444StreamEncoder(buffer, info);
    }

    return NULL;
}

// --- RGB
vp9a_encode_ctx_t* vp9a_openRGBEncoder(const char* outpath, vid_info_t* info) {
    //This uses the examples in the libvpx documentation

    //Allocate context and copy info to it
    if (info == NULL) return NULL;
    vp9a_encode_ctx_t* myctx = alloc_ctx(outpath, info);

    //Check format enums to make sure they match...
    if (myctx->input_fmt != STANDARD) {
        if (myctx->input_fmt == SETME) myctx->input_fmt = STANDARD;
        else {
            myctx->error_code = FMT_MISMATCH;
            return myctx;
        }
    }
    if (myctx->input_clr != RGB) {
        if (myctx->input_clr == CLR_SETME) myctx->input_clr = RGB;
        else {
            myctx->error_code = FMT_MISMATCH;
            return myctx;
        }
    }

    //Calculate bytes per frame
    //(3*w*h)
    myctx->bytes_per_frame = 3 * myctx->info.width * myctx->info.height;

    //Generate codec info
    const VpxInterface* encoder = gen_codec_info(myctx);
    if (!encoder) return myctx;

    //Data gets rearranged to planar when read in
   // myctx->img_container = !vpx_img_alloc(NULL, VPX_IMG_FMT_I444, info->width, info->height, 1);
    if (!vpx_img_alloc(&myctx->img_container, VPX_IMG_FMT_I444, info->width, info->height, 1)) {
        myctx->error_code = IMGALLOC_FAILED;
        return myctx;
    }
    myctx->init_img_cont = TRUE;

    //Set color space
    myctx->img_container.cs = VPX_CS_SRGB;
    myctx->img_container.range = VPX_CR_FULL_RANGE;

    //Config
    if (configAndInit(myctx, encoder) != NO_ERROR) return myctx;
    myctx->func_frameload = readRGBFrame;

    return myctx;
}

vp9a_encode_ctx_t* vp9a_openRGBFileEncoder(const char* outpath, vid_info_t* info) {
    return vp9a_openRGBEncoder(outpath, info);
}

vp9a_encode_ctx_t* vp9a_openRGBStreamEncoder(ubyte* buffer, vid_info_t* info) {
    vp9a_encode_ctx_t* ctx = vp9a_openRGBEncoder(NULL, info);
    if (!ctx) return NULL;
    ctx->target.buffer_ptr = buffer;
    return ctx;
}

// --- ARGB

vp9a_encode_ctx_t* vp9a_openARGBEncoder(const char* outpath, vid_info_t* info) {
    //This uses the examples in the libvpx documentation

    //Allocate context and copy info to it
    if (info == NULL) return NULL;
    vp9a_encode_ctx_t* myctx = alloc_ctx(outpath, info);

    //Check format enums to make sure they match...
    if (myctx->input_fmt != STANDARD) {
        if (myctx->input_fmt == SETME) myctx->input_fmt = STANDARD;
        else {
            myctx->error_code = FMT_MISMATCH;
            return myctx;
        }
    }
    if (myctx->input_clr != ARGB) {
        if (myctx->input_clr == CLR_SETME) myctx->input_clr = ARGB;
        else {
            myctx->error_code = FMT_MISMATCH;
            return myctx;
        }
    }

    //Calculate bytes per frame
    //(4*w*h)
    myctx->bytes_per_frame = (myctx->info.width * myctx->info.height) << 2;

    //Generate codec info
    const VpxInterface* encoder = gen_codec_info(myctx);
    if (!encoder) return myctx;

    //Data gets rearranged to planar when read in
    //myctx->img_container = !vpx_img_alloc(NULL, VPX_IMG_FMT_I444, info->width, info->height, 1);
    if (!vpx_img_alloc(&myctx->img_container, VPX_IMG_FMT_I444, info->width, info->height, 1)) {
        myctx->error_code = IMGALLOC_FAILED;
        return myctx;
    }
    myctx->init_img_cont = TRUE;

    //Set color space
    myctx->img_container.cs = VPX_CS_SRGB;
    myctx->img_container.range = VPX_CR_FULL_RANGE;

    //Config
    if (configAndInit(myctx, encoder) != NO_ERROR) return myctx;
    myctx->func_frameload = readARGBFrame;

    return myctx;
}

vp9a_encode_ctx_t* vp9a_openARGBFileEncoder(const char* outpath, vid_info_t* info) {
    return vp9a_openARGBEncoder(outpath, info);
}

vp9a_encode_ctx_t* vp9a_openARGBStreamEncoder(ubyte* buffer, vid_info_t* info) {
    vp9a_encode_ctx_t* ctx = vp9a_openARGBEncoder(NULL, info);
    if (!ctx) return NULL;
    ctx->target.buffer_ptr = buffer;
    return ctx;
}

// --- YUV420

vp9a_encode_ctx_t* vp9a_openYUV420Encoder(const char* outpath, vid_info_t* info) {
    //This uses the examples in the libvpx documentation

    //Allocate context and copy info to it
    if (outpath == NULL || info == NULL) return NULL;
    vp9a_encode_ctx_t* myctx = alloc_ctx(outpath, info);
    //printf("--DEBUG-- alloc_ctx returned. Error code: %d\n", myctx->error_code);

    //Check format enums to make sure they match...
    if (myctx->input_fmt != YUV_I420) {
        if (myctx->input_fmt == SETME) myctx->input_fmt = YUV_I420;
        else {
            myctx->error_code = FMT_MISMATCH;
            return myctx;
        }
    }
    if (!(myctx->input_clr == BT601 || myctx->input_clr == BT709 || myctx->input_clr == BT2020)) {
        if (myctx->input_clr == CLR_SETME) myctx->input_clr = BT601;
        else {
            myctx->error_code = FMT_MISMATCH;
            return myctx;
        }
    }
    //printf("--DEBUG-- Format check passed. Error code: %d\n", myctx->error_code);

    //Calculate bytes per frame
    myctx->bytes_per_frame = 3 * myctx->info.width * myctx->info.height; //This gives double (2 * 1.5 * w * h)
    if (!myctx->sixteen) myctx->bytes_per_frame >>= 1; //Half if not wide

    //Generate codec info
    const VpxInterface* encoder = gen_codec_info(myctx);
    if (!encoder) return myctx;
    //printf("--DEBUG-- gen_codec_info returned. Error code: %d\n", myctx->error_code);

    vpx_img_fmt_t fmt = myctx->sixteen ? VPX_IMG_FMT_I42016 : VPX_IMG_FMT_I420;
    //myctx->img_container = !vpx_img_alloc(NULL, fmt, info->width, info->height, 1);
    if (!vpx_img_alloc(&myctx->img_container, fmt, myctx->info.width, myctx->info.height, 1)) {
        myctx->error_code = IMGALLOC_FAILED;
        return myctx;
    }
    myctx->init_img_cont = TRUE;
    //printf("\tSTRUCT_INIT vpx_image_t @ %p\n", &myctx->img_container);
    //printf("--DEBUG-- VPX image alloc passed. Error code: %d\n", myctx->error_code);

    //Set color space
    switch (myctx->input_clr) {
    case BT601:
        myctx->img_container.cs = VPX_CS_BT_601;
        break;
    case BT709:
        myctx->img_container.cs = VPX_CS_BT_709;
        break;
    case BT2020:
        myctx->img_container.cs = VPX_CS_BT_2020;
        break;
    default:
        myctx->error_code = FMT_MISMATCH;
        return myctx;
    }
    myctx->img_container.range = myctx->fullcolor ? VPX_CR_FULL_RANGE : VPX_CR_STUDIO_RANGE;
    //printf("--DEBUG-- Color space setting passed. Error code: %d\n", myctx->error_code);

    //Config
    if (configAndInit(myctx, encoder) != NO_ERROR) {
        //printf("--DEBUG-- configAndInit failed. Error code: %d\n", myctx->error_code);
        return myctx;
    }
    myctx->func_frameload = readPlanarFrame;
    //printf("--DEBUG-- configAndInit returned. Error code: %d\n", myctx->error_code);

    //debug_printVPCTX(myctx);
    return myctx;
}

vp9a_encode_ctx_t* vp9a_openYUV420FileEncoder(const char* outpath, vid_info_t* info) {
    return vp9a_openYUV420Encoder(outpath, info);
}

vp9a_encode_ctx_t* vp9a_openYUV420StreamEncoder(ubyte* buffer, vid_info_t* info) {
    vp9a_encode_ctx_t* ctx = vp9a_openYUV420Encoder(NULL, info);
    if (!ctx) return NULL;
    ctx->target.buffer_ptr = buffer;
    return ctx;
}

// --- YUV422

vp9a_encode_ctx_t* vp9a_openYUV422Encoder(const char* outpath, vid_info_t* info) {
    //This uses the examples in the libvpx documentation

    //Allocate context and copy info to it
    if (outpath == NULL || info == NULL) return NULL;
    vp9a_encode_ctx_t* myctx = alloc_ctx(outpath, info);

    //Check format enums to make sure they match...
    if (myctx->input_fmt != YUV_I422) {
        if (myctx->input_fmt == SETME) myctx->input_fmt = YUV_I422;
        else {
            myctx->error_code = FMT_MISMATCH;
            return myctx;
        }
    }
    if (!(myctx->input_clr == BT601 || myctx->input_clr == BT709 || myctx->input_clr == BT2020)) {
        if (myctx->input_clr == CLR_SETME) myctx->input_clr = BT601;
        else {
            myctx->error_code = FMT_MISMATCH;
            return myctx;
        }
    }

    //Calculate bytes per frame
    myctx->bytes_per_frame = 2 * myctx->info.width * myctx->info.height; //[ (1Y + 0.5U + 0.5V) * w * h]
    if (myctx->sixteen) myctx->bytes_per_frame <<= 1; //Double if wide

    //Generate codec info
    const VpxInterface* encoder = gen_codec_info(myctx);
    if (!encoder) return myctx;

    vpx_img_fmt_t fmt = myctx->sixteen ? VPX_IMG_FMT_I42216 : VPX_IMG_FMT_I422;
   // myctx->img_container = !vpx_img_alloc(NULL, fmt, info->width, info->height, 1);
    if (!vpx_img_alloc(&myctx->img_container, fmt, info->width, info->height, 1)) {
        myctx->error_code = IMGALLOC_FAILED;
        return myctx;
    }
    myctx->init_img_cont = TRUE;

    //Set color space
    switch (myctx->input_clr) {
    case BT601:
        myctx->img_container.cs = VPX_CS_BT_601;
        break;
    case BT709:
        myctx->img_container.cs = VPX_CS_BT_709;
        break;
    case BT2020:
        myctx->img_container.cs = VPX_CS_BT_2020;
        break;
    default:
        myctx->error_code = FMT_MISMATCH;
        return myctx;
    }
    myctx->img_container.range = myctx->fullcolor ? VPX_CR_FULL_RANGE : VPX_CR_STUDIO_RANGE;

    //Config
    if (configAndInit(myctx, encoder) != NO_ERROR) return myctx;
    myctx->func_frameload = readPlanarFrame;

    return myctx;
}

vp9a_encode_ctx_t* vp9a_openYUV422FileEncoder(const char* outpath, vid_info_t* info) {
    return vp9a_openYUV422Encoder(outpath, info);
}

vp9a_encode_ctx_t* vp9a_openYUV422StreamEncoder(ubyte* buffer, vid_info_t* info) {
    vp9a_encode_ctx_t* ctx = vp9a_openYUV422Encoder(NULL, info);
    if (!ctx) return NULL;
    ctx->target.buffer_ptr = buffer;
    return ctx;
}

// --- YUV444

vp9a_encode_ctx_t* vp9a_openYUV444Encoder(const char* outpath, vid_info_t* info) {
    //This uses the examples in the libvpx documentation

    //Allocate context and copy info to it
    if (outpath == NULL || info == NULL) return NULL;
    vp9a_encode_ctx_t* myctx = alloc_ctx(outpath, info);

    //Check format enums to make sure they match...
    if (myctx->input_fmt != PLANAR_444) {
        if (myctx->input_fmt == SETME) myctx->input_fmt = PLANAR_444;
        else {
            myctx->error_code = FMT_MISMATCH;
            return myctx;
        }
    }
    if (!(myctx->input_clr == BT601 || myctx->input_clr == BT709 || myctx->input_clr == BT2020)) {
        if (myctx->input_clr == CLR_SETME) myctx->input_clr = BT601;
        else {
            myctx->error_code = FMT_MISMATCH;
            return myctx;
        }
    }

    //Calculate bytes per frame
    myctx->bytes_per_frame = 3 * myctx->info.width * myctx->info.height; //[ (1Y + 1U + 1V) * w * h]
    if (myctx->sixteen) myctx->bytes_per_frame <<= 1; //Double if wide

    //Generate codec info
    const VpxInterface* encoder = gen_codec_info(myctx);
    if (!encoder) return myctx;

    vpx_img_fmt_t fmt = myctx->sixteen ? VPX_IMG_FMT_I44416 : VPX_IMG_FMT_I444;
   // myctx->img_container = !vpx_img_alloc(NULL, fmt, info->width, info->height, 1);
    if (!vpx_img_alloc(&myctx->img_container, fmt, info->width, info->height, 1)) {
        myctx->error_code = IMGALLOC_FAILED;
        return myctx;
    }
    myctx->init_img_cont = TRUE;

    //Set color space
    switch (myctx->input_clr) {
    case BT601:
        myctx->img_container.cs = VPX_CS_BT_601;
        break;
    case BT709:
        myctx->img_container.cs = VPX_CS_BT_709;
        break;
    case BT2020:
        myctx->img_container.cs = VPX_CS_BT_2020;
        break;
    default:
        myctx->error_code = FMT_MISMATCH;
        return myctx;
    }
    myctx->img_container.range = myctx->fullcolor ? VPX_CR_FULL_RANGE : VPX_CR_STUDIO_RANGE;

    //Config
    if (configAndInit(myctx, encoder) != NO_ERROR) return myctx;
    myctx->func_frameload = readPlanarFrame;

    return myctx;
}

vp9a_encode_ctx_t* vp9a_openYUV444FileEncoder(const char* outpath, vid_info_t* info) {
    return vp9a_openYUV444Encoder(outpath, info);
}

vp9a_encode_ctx_t* vp9a_openYUV444StreamEncoder(ubyte* buffer, vid_info_t* info) {
    vp9a_encode_ctx_t* ctx = vp9a_openYUV444Encoder(NULL, info);
    if (!ctx) return NULL;
    ctx->target.buffer_ptr = buffer;
    return ctx;
}

/*----- Write/Close -----*/

int write_frame(vp9a_encode_ctx_t* ctx, ubyte* data) {

    //Wrap data
    boolean any_written = FALSE;
    if (ctx->func_frameload == NULL) return 1;
    if (!ctx->func_frameload(&ctx->img_container, data)) return 2;
    //printf("--DEBUG-- Frameload complete\n");

    //Do encoding
    vpx_codec_iter_t iter = NULL;
    const vpx_codec_cx_pkt_t* pkt = NULL;

    int flags = 0;
    if (ctx->info.keyintr > 0 && ctx->frames_written % ctx->info.keyintr == 0) {
        flags |= VPX_EFLAG_FORCE_KF;
    }

    const vpx_codec_err_t res = vpx_codec_encode(&ctx->codec, &ctx->img_container, ctx->frames_written++, 1, flags, VPX_DL_GOOD_QUALITY);
    if (res != VPX_CODEC_OK) return 3;
    //printf("--DEBUG-- Codec encode complete. Error: %d (%s)\n", ctx->codec.err, ctx->codec.err_detail);

   // vpx_codec_ctx_t* cptr = &ctx->codec;
    //pkt = cptr->iface->enc;

    while (pkt = vpx_codec_get_cx_data(&ctx->codec, &iter)) {
        //printf("--DEBUG-- Non-null packet found\n");
        any_written = TRUE;
        if (pkt->kind == VPX_CODEC_CX_FRAME_PKT) {
            /*const int keyframe = (pkt->data.frame.flags & VPX_FRAME_IS_KEY) != 0;
            if (!vpx_video_writer_write_frame(ctx->writer, pkt->data.frame.buf, pkt->data.frame.sz, pkt->data.frame.pts)) {
                return 4;
            }*/
            ctx->error_code = ctx->writerMethod(ctx, pkt);
            if (ctx->error_code != NO_ERROR) return 4;
        }
    }
    //printf("--DEBUG-- Package processing complete. Error: %d (%s)\n", ctx->codec.err, ctx->codec.err_detail);

    //Check for callback
    if (ctx->write_callback != NULL && ctx->frames_per_callback > 0 && ctx->frames_written % ctx->frames_per_callback == 0) {
        ctx->write_callback();
    }

    //debug_printVPCTX(ctx);
    return any_written ? 0 : -1;
}

boolean flush_encoder(vp9a_encode_ctx_t* ctx) {
    vpx_codec_iter_t iter = NULL;
    const vpx_codec_cx_pkt_t* pkt = NULL;
    boolean haspkts = FALSE;

    do {
        haspkts = FALSE;
        const vpx_codec_err_t res = vpx_codec_encode(&ctx->codec, NULL, -1, 1, 0, VPX_DL_GOOD_QUALITY);
        if (res != VPX_CODEC_OK) return FALSE;

        while ((pkt = vpx_codec_get_cx_data(&ctx->codec, &iter)) != NULL) {
            haspkts = TRUE;

            if (pkt->kind == VPX_CODEC_CX_FRAME_PKT) {
                //if (!vpx_video_writer_write_frame(ctx->writer, pkt->data.frame.buf, pkt->data.frame.sz, pkt->data.frame.pts)) {return FALSE;}
                ctx->error_code = ctx->writerMethod(ctx, pkt);
                if (ctx->error_code != NO_ERROR) return FALSE;
                fflush(stdout);
            }
        }

    } while (haspkts);

    return TRUE;
}

vp9a_error_t vp9a_closeEncoder(vp9a_encode_ctx_t* ctx) {
    //outpath is now mallocd to this struct (copy of what was passed at alloc), so must be freed

    //Release vpx resources
    if (ctx->error_code == NO_ERROR) {
        if (!flush_encoder(ctx)) ctx->error_code = FAILED_CLOSE;
    }

    if(ctx->init_img_cont) vpx_img_free(&ctx->img_container);
    if (ctx->init_codec) {
        if (vpx_codec_destroy(&ctx->codec)) ctx->error_code = FAILED_CLOSE;
    }

    if (ctx->outpath) {
        //Assumes it's written to a file. So close writer.
        vpx_video_writer_close(ctx->target.writer);
        free(ctx->outpath);
    }

    //Release adapter resources
    vp9a_error_t err = ctx->error_code;
    //free(ctx->info);
    free(ctx);

    return err;
}

/*----- Frame Wrapping/Conversion -----*/

vpx_image_t* readRGBFrame(vpx_image_t* container, void* data) {
    if (container == NULL) return NULL;
    if (data == NULL) return NULL;

    int plane;

    for (plane = 0; plane < 3; plane++) {
        unsigned char* datapos = (unsigned char*)data + plane;
        unsigned char* buf = container->planes[plane];
        const int w = vpx_img_plane_width(container, plane);
        const int h = vpx_img_plane_height(container, plane);
        const int pixcount = w * h;

        int p;
        for (p = 0; p < pixcount; p++) {
            *(buf++) = *(datapos);
            datapos += 3;
        }

    }

    return container;
}

vpx_image_t* readARGBFrame(vpx_image_t* container, void* data) {
    //Right now, it ignores the alpha channel. Will add when can figure out how (doesn't seem compatible with VPX)
    if (container == NULL) return NULL;
    if (data == NULL) return NULL;

    int plane;

    for (plane = 0; plane < 3; plane++) {
        unsigned char* datapos = (unsigned char*)data + 1 + plane; //Skips A phase
        unsigned char* buf = container->planes[plane];
        const int w = vpx_img_plane_width(container, plane);
        const int h = vpx_img_plane_height(container, plane);
        const int pixcount = w * h;

        int p;
        for (p = 0; p < pixcount; p++) {
            *(buf++) = *(datapos);
            datapos += 4;
        }

    }

    return container;
}

//Modified version of vpx_img_read that deals with data in memory instead of loading from FILE
vpx_image_t* readPlanarFrame(vpx_image_t* container, void* data) {
    if (container == NULL) return NULL;
    if (data == NULL) return NULL;
    //printf("Loading data @ %p to vpx_image_t @ %p\n", data, container);

    int plane;
    unsigned char* datapos = (unsigned char*)data;

    /* At some point for speed, I can probably just make the containers planes
    point to the existing data instead of copying all the data. But for now I don't want to mess
    with vpx structures that directly, especially since they have their own alloc and stuff.
    */
    for (plane = 0; plane < 3; plane++) {
        unsigned char* buf = container->planes[plane];
        const int stride = container->stride[plane];
        const int w = vpx_img_plane_width(container, plane) *((container->fmt & VPX_IMG_FMT_HIGHBITDEPTH) ? 2 : 1);
        const int h = vpx_img_plane_height(container, plane);
        int y;
        //printf("Doing plane %d | Dim: %d x %d | Stride: %d\n", plane, w, h, stride);

        for (y = 0; y < h; y++) {
            //if (fread(buf, 1, w, file) != (size_t)w) return NULL;
            memcpy(buf, datapos, (size_t)w);
            buf += stride; datapos += stride;
        }
    }

    return container;
}

/*----- Misc. Utils -----*/

void setTimebaseFromFramerate(vid_info_t* info) {
    if (info == NULL) return;
    if (roundf(info->fps) == info->fps) {
        //Integer framerate.
        info->timebase_n = 1;
        info->timebase_d = roundf(info->fps);
    }
    else {
        //ehhh f if I know.
        //Best I can think of is check for common ones?
        //Otherwise, just have to get program to pass n/d in the first place...
        //TODO
    }

}
