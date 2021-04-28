
#include "oggvorbisAdapter.h"

oggvorb_comment_set_t* initCommentContainer(int initial_capacity) {

	if (initial_capacity < 1) initial_capacity = 16;
	oggvorb_comment_set_t* cmt = (oggvorb_comment_set_t*)malloc(sizeof(oggvorb_comment_set_t));
	cmt->capacity = initial_capacity;
	cmt->cmt_count = 0;
	cmt->comments = (char**)malloc(cmt->capacity * sizeof(char*));

	return cmt;
}

int submitPacketToOgg(oggvorb_writer_ctx_t* ctx, ogg_packet* pkt) {
	//Does both packet in and page out.
	int res;
	size_t written;
	res = ogg_stream_packetin(&ctx->oggstr, pkt);
	if (res != 0) {
		ctx->err = res;
		return res;
	}

	ogg_page page;
	memset(&page, 0, sizeof(ogg_page));
	res = ogg_stream_pageout(&ctx->oggstr, &page);

	if (!res) {
		//Not enough for page yet.
		ctx->err = 1;
		return 1;
	}

	//Write to output stream
	written = fwrite(page.header, 1, page.header_len, ctx->output);
	if (written != page.header_len) {
		ctx->err = 1;
		return 1;
	}

	written = fwrite(page.body, 1, page.body_len, ctx->output);
	if (written != page.body_len) {
		ctx->err = 1;
		return 1;
	}
	return 0;
}

oggvorb_writer_ctx_t* initOggVorbisWriter(aud_info_t* audio_info, oggvorb_comment_set_t* comments, const char* path, int quality) {
	int i, res;

	oggvorb_writer_ctx_t* ctx = (oggvorb_writer_ctx_t*)malloc(sizeof(oggvorb_writer_ctx_t));
	memset(ctx, 0, sizeof(oggvorb_writer_ctx_t));
	ctx->ogg_serial = rand();

	ctx->info = *audio_info; //Does this copy all struct by val? I hope so.

//Try to init vorbis info
	vorbis_info_init(&ctx->vorbinfo);

	res = vorbis_encode_init_vbr(&ctx->vorbinfo, (long)ctx->info.channels, (long)ctx->info.sample_rate, (float)quality * 0.1);
	if (res != 0) {ctx->err = res; return ctx;}

	res = vorbis_analysis_init(&ctx->dsp_state, &ctx->vorbinfo);
	if (res != 0) { ctx->err = res; return ctx; }

	//Open the ogg stream
	ctx->output = fopen(path, "w");
	res = ogg_stream_init(&ctx->oggstr, ctx->ogg_serial);
	if (res != 0) { ctx->err = res; return ctx; }

	//Vorbis comments...
	vorbis_comment_init(&ctx->vorb_cmt);
	if (comments) {
		for (i = 0; i < comments->cmt_count; i++) {
			vorbis_comment_add(&ctx->vorb_cmt, comments->comments[i]);
		}
	}
	vorbis_comment_add(&ctx->vorb_cmt, "WRAPPERLIB=wrmedadap");

	//Some vorbis head oggpackets...
	ogg_packet op, op_comm, op_code;
	memset(&op, 0, sizeof(ogg_packet));
	memset(&op_comm, 0, sizeof(ogg_packet));
	memset(&op_code, 0, sizeof(ogg_packet));
	res = vorbis_analysis_headerout(&ctx->dsp_state, &ctx->vorb_cmt, &op, &op_comm, &op_code);
	if (res != 0) { ctx->err = res; return ctx; }

	//Submit these packets to the ogg stream...
	res = submitPacketToOgg(ctx, &op);
	if (res != 0) { return ctx; }
	res = submitPacketToOgg(ctx, &op_comm);
	if (res != 0) { return ctx; }
	res = submitPacketToOgg(ctx, &op_code);
	if (res != 0) { return ctx; }

	res = vorbis_block_init(&ctx->dsp_state, &ctx->vorb_block);
	if (res != 0) { ctx->err = res; return ctx; }

	return ctx;
}

size_t writeAudioData(oggvorb_writer_ctx_t* ctx, void** data, int frames) {
	/*
	TODO
	One thing I'm not sure about is whether it needs a new Vorbis block every time?
	Because I'm just recycling the same one without re-init and clearing
	*/

	//Endian-ness is assumed to match system.
	if (!ctx || !data || frames < 1) return 0;
	size_t ctr = 0;
	int c, ch, f, i, res, res2;
	ch = ctx->info.channels;
	int vals = ch * frames;
	float** vorb_buff = vorbis_analysis_buffer(&ctx->dsp_state, vals);
	//TODO ^^ pass #frames to vorbis_analysis_buffer? or # total samples in all channels?

	//Load buffer
	//Not sure how to scale to float amplitude? So I'll just try -1 to 1?
	//BD is expected to be 8u, 16s or 24s
	uint8_t* ch_in_8;
	int16_t* ch_in_16;
	for (c = 0; c < ch; c++) {
		//Iterate thru channels
		float* ch_buff = vorb_buff[c];
		switch (ctx->info.bit_depth) {
		case 8:
			ch_in_8 = (uint8_t*)(data[c]);
			for (f = 0; f < frames; f++) {
				uint8_t raw = ch_in_8[f];
				int32_t s32 = ((int32_t)raw) & 0xFF;
				s32 -= 128;
				if (s32 > 0) {
					ch_buff[f] = ((float)s32)/127.0;
				}
				else if (s32 < 0) {
					ch_buff[f] = ((float)s32) / 128.0;
				}
				else ch_buff[f] = 0.0;
				ctr += 4;
			}
			break;
		case 16:
			ch_in_16 = (int16_t*)(data[c]);
			for (f = 0; f < frames; f++) {
				int32_t s32 = (int32_t)ch_in_16[f];
				ch_buff[f] = ((float)s32) / 32767.0;
				ctr += 4;
			}
			break;
		case 24:
			ch_in_8 = (uint8_t*)(data[c]);
			for (f = 0; f < frames; f++) {
				int32_t s32 = 0;
				for (i = 0; i < 3; i++) {
					int32_t raw = (int32_t)(*ch_in_8++);
					s32 |= (raw << 24);
					s32 >>= 8;
				}
				ch_buff[f] = ((float)s32) / 8388607.0;
				ctr += 4;
			}
			break;
		}
	}

	//Write to vorb.
	res = vorbis_analysis_wrote(&ctx->dsp_state, vals);
	if (res != 0) {
		ctx->err = res;
		return 0;
	}

	//Extract packets.
	res = 1;
	ogg_packet op;
	memset(&op, 0, sizeof(ogg_packet));
	while (res != 0) {
		res = vorbis_analysis_blockout(&ctx->dsp_state, &ctx->vorb_block);
		if (res < 0) {
			ctx->err = res;
			return 0;
		}
		res2 = vorbis_analysis(&ctx->vorb_block, &op);
		if (res2 < 0) { ctx->err = res2; return 0; }
		//Add bitrate management here if needed...
		res2 = submitPacketToOgg(ctx, &op);
		if (res2 != 0) { return 0; }
	}

	return ctr;
}

int closeOggVorbisWriter(oggvorb_writer_ctx_t* ctx) {
	//Close up the encoder (send end packets)
	int res, res2;
	res = vorbis_analysis_wrote(&ctx->dsp_state, 0);
	if (res != 0) {
		ctx->err = res;
		return -1;
	}
	res = 1;
	ogg_packet op;
	memset(&op, 0, sizeof(ogg_packet));
	while (res != 0) {
		res = vorbis_analysis_blockout(&ctx->dsp_state, &ctx->vorb_block);
		if (res < 0) {
			ctx->err = res;
			return -1;
		}
		res2 = vorbis_analysis(&ctx->vorb_block, &op);
		if (res2 < 0) { ctx->err = res2; return 0; }
		//Add bitrate management here if needed...
		res2 = submitPacketToOgg(ctx, &op);
		if (res2 != 0) { return -1; }
	}

	//Close up the ogg writer and file
	res = 1;
	ogg_page opg;
	size_t written = 0;
	while (res != 0) {
		res = ogg_stream_flush(&ctx->oggstr, &opg);
		if (res < 0) {
			ctx->err = res;
			return -1;
		}

		written = fwrite(opg.header, 1, opg.header_len, ctx->output);
		if (written != opg.header_len) {
			ctx->err = 1;
			return -1;
		}

		written = fwrite(opg.body, 1, opg.body_len, ctx->output);
		if (written != opg.body_len) {
			ctx->err = 1;
			return -1;
		}
	}

	//Free things that need to be freed
	fclose(ctx->output);
	ogg_stream_clear(&ctx->oggstr);
	vorbis_block_clear(&ctx->vorb_block);
	vorbis_dsp_clear(&ctx->dsp_state);
	vorbis_comment_clear(&ctx->vorb_cmt);
	vorbis_info_clear(&ctx->vorbinfo);

	free(ctx);

	return 0;
}

void addComment(oggvorb_comment_set_t* cmt_cont, const char* comment) {
	if (cmt_cont->cmt_count >= cmt_cont->capacity) {
		//Realloc array... yuck.
		char** old = cmt_cont->comments;
		int newcap = cmt_cont->capacity << 1;
		cmt_cont->capacity = newcap;
		cmt_cont->comments = (char**)malloc(cmt_cont->capacity * sizeof(char*));

		int i = 0;
		for (i = 0; i < cmt_cont->cmt_count; i++) {
			cmt_cont->comments[i] = old[i];
		}

		free(old);
	}

	size_t slen = strlen(comment);
	char* copy = (char*)malloc(slen+1);
	memcpy(copy, comment, slen + 1);
	cmt_cont->comments[cmt_cont->cmt_count++] = copy;
}

void freeCommentContainer(oggvorb_comment_set_t* cmt_cont) {

	//Free comments themselves.
	int i;
	for (i = 0; i < cmt_cont->cmt_count; i++) {
		free(cmt_cont->comments[i]);
	}

	free(cmt_cont->comments);

	//Not that these matter since the stuct gets freed but it makes me feel better
	cmt_cont->capacity = 0;
	cmt_cont->cmt_count = 0;

	free(cmt_cont);
}