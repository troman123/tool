
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
//#include <stdlib.h>


static AVFormatContext *ifmt_pic_ctx;
static AVFormatContext *ifmt_ctx;
static AVFormatContext *ofmt_ctx;


typedef struct FilteringContext {
    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc_ctx;
    AVFilterGraph *filter_graph;
} FilteringContext;
static FilteringContext *filter_ctx;

static void  AVCodecContextPrint(char * name, int streamid, AVCodecContext *cctx){
			av_log(NULL,AV_LOG_INFO, "=========: %s, stream:%u, codec_type:%u，  =======================\n"
					"sample_rate:%u, time_base:%u/%u，bit_rate:%d, \n"
					"bit_rate_tolerance:%d, global_quality:%d,\n"
					"delay:%d, width:%d, height:%d, gop_size:%d, pix_fmt:%d,\n"
					"max_b_frames:%d, b_quant_factor:%f, \n"
					"b_frame_strategy:%d, b_quant_offset:%d, has_b_frames:%d, \n"
					"mpeg_quant:%d, i_quant_factor:%f, \n"
					"i_quant_offset:%f, lumi_masking:%f, temporal_cplx_masking:%f, \n"
					"spatial_cplx_masking:%f, p_masking:%f, dark_masking:%f, \n"
					"slice_count:%d, prediction_method:%d, slice_offset:%d,\n"
					"sample_aspect_ratio:%d/%d, me_cmp:%d, me_sub_cmp:%d, mb_cmp:%d, \n"
					"ildct_cmp:%d, dia_size:%d, last_predictor_count:%d, \n"
					"pre_me:%d, me_pre_cmp:%d, pre_dia_size:%d, me_subpel_quality:%d, \n"
					"me_range:%d, slice_flags:%d, mb_decision:%d, intra_matrix:%d, \n"
					"inter_matrix:%d, scenechange_threshold:%d, noise_reduction:%d, \n"
					"intra_dc_precision:%d, skip_top:%d, skip_bottom:%d, \n"
					"mb_lmin:%d, mb_lmax:%d, me_penalty_compensation:%d, \n"
					"bidir_refine:%d, brd_scale:%d, keyint_min:%d, \n"
					"refs:%d, chromaoffset:%d, \n"
					"mv0_threshold:%d, b_sensitivity:%d, \n"
					"color_primaries:%d, color_trc:%d, colorspace:%d, \n"
					"color_range:%d, chroma_sample_location:%d, \n"
					"slices:%d, field_order:%d, sample_rate:%d, channels:%d, \n"
					"sample_fmt:%d, frame_size:%d, frame_number:%d, \n"
					"block_align:%d, cutoff:%d, channel_layout:%d, \n"
					"request_channel_layout:%d, audio_service_type:%d, \n"
					"request_sample_fmt:%d, refcounted_frames:%d, \n"
					"qcompress:%d, qblur:%d, qmin:%d, qmax:%d, max_qdiff:%d, \n"
					"rc_buffer_size:%d, rc_override_count:%d, \n"
					"rc_override:xxx, rc_max_rate:%d, \n"
					"rc_min_rate:%d, rc_max_available_vbv_use:%f, \n"
					"rc_min_vbv_overflow_use:%f, rc_initial_buffer_occupancy:%d, \n"
					"coder_type:%d, context_model:%d, \n"
					"frame_skip_threshold:%d, frame_skip_factor:%d,\n"
					" frame_skip_exp:%d, frame_skip_cmp:%d, \n"
					"trellis:%d, min_prediction_order:%d, max_prediction_order:%d, \n"
					"timecode_frame_start:%d, rtp_payload_size:%d,\n"
					"mv_bits:%d, header_bits:%d, i_tex_bits:%d, \n"
					"p_tex_bits:%d, i_count:%d, p_count:%d, \n"
					"skip_count:%d, misc_bits:%d, frame_bits:%d, \n"
					"stats_out:%s, stats_in:%s, workaround_bugs:%d, \n"
					"strict_std_compliance:%d, error_concealment:%d,\n"
					"debug:%d, debug_mv:%d, err_recognition:%d,\n"
					"reordered_opaque:%d, error:%d, \n"
					"dct_algo:%d, idct_algo:%d, \n"
					"bits_per_coded_sample:%d, bits_per_raw_sample:%d, \n"
					"lowres:%d, thread_count:%d, thread_type:%d, \n"
					"active_thread_type:%d, thread_safe_callbacks:%d, \n"
					"nsse_weight:%d, profile:%d, level:%d, \n"
					"skip_loop_filter:%d, skip_idct:%d, skip_frame:%d, \n"
					"subtitle_header:%s, subtitle_header_size:%d, \n"
					"vbv_delay:%d, side_data_only_packets:%d, \n"
					"initial_padding:%d, \n"
					"framerate:%d/%d, sw_pix_fmt:%d, pkt_timebase:%d/%d, \n"
					"pts_correction_num_faulty_pts:%d, pts_correction_num_faulty_dts:%d,\n"
					"pts_correction_last_pts:%d, pts_correction_last_dts:%d, \n"
					"sub_charenc:%s, sub_charenc_mode:%d, skip_alpha:%d,\n"
					"seek_preroll:%d, chroma_intra_matrix:%d, dump_separator:%d,  \n"
					"codec_whitelist:%s, properties:%d \n",
					name, streamid, cctx->codec_type, 
					cctx->sample_rate, cctx->time_base.num, cctx->time_base.den, cctx->bit_rate, cctx->bit_rate_tolerance, cctx->global_quality,
					cctx->delay, cctx->width, cctx->height, cctx->gop_size, cctx->pix_fmt, cctx->max_b_frames, cctx->b_quant_factor,
					cctx->b_frame_strategy, cctx->b_quant_offset, cctx->has_b_frames, cctx->mpeg_quant, cctx->i_quant_factor,
					cctx->i_quant_offset, cctx->lumi_masking, cctx->temporal_cplx_masking, cctx->spatial_cplx_masking, cctx->p_masking, cctx->dark_masking,
					cctx->slice_count, cctx->prediction_method, cctx->slice_offset, cctx->sample_aspect_ratio.num, cctx->sample_aspect_ratio.den,
					cctx->me_cmp, cctx->me_sub_cmp, cctx->mb_cmp,
					cctx->ildct_cmp, cctx->dia_size, cctx->last_predictor_count, cctx->pre_me, cctx->me_pre_cmp, cctx->pre_dia_size, cctx->me_subpel_quality, 
					cctx->me_range, cctx->slice_flags, cctx->mb_decision, cctx->intra_matrix, 
					cctx->inter_matrix, cctx->scenechange_threshold, cctx->noise_reduction, 
					cctx->intra_dc_precision, cctx->skip_top, cctx->skip_bottom, cctx->mb_lmin, cctx->mb_lmax, cctx->me_penalty_compensation, 
					cctx->bidir_refine, cctx->brd_scale, cctx->keyint_min, cctx->refs, cctx->chromaoffset, 
					cctx->mv0_threshold, cctx->b_sensitivity, cctx->color_primaries, cctx->color_trc, cctx->colorspace,
					cctx->color_range, cctx->chroma_sample_location, cctx->slices, cctx->field_order, cctx->sample_rate, cctx->channels,
					cctx->sample_fmt, cctx->frame_size, cctx->frame_number, cctx->block_align, cctx->cutoff, cctx->channel_layout,
					cctx->request_channel_layout, cctx->audio_service_type, cctx->request_sample_fmt, cctx->refcounted_frames,
					cctx->qcompress, cctx->qblur, cctx->qmin, cctx->qmax, cctx->max_qdiff, 
					cctx->rc_buffer_size, cctx->rc_override_count, cctx->rc_max_rate,
					cctx->rc_min_rate, cctx->rc_max_available_vbv_use,
					cctx->rc_min_vbv_overflow_use, cctx->rc_initial_buffer_occupancy, cctx->coder_type, cctx->context_model,
					cctx->frame_skip_threshold, cctx->frame_skip_factor, cctx->frame_skip_exp,
					cctx->frame_skip_cmp, cctx->trellis, cctx->min_prediction_order, cctx->max_prediction_order,
					cctx->timecode_frame_start, cctx->rtp_payload_size, cctx->mv_bits, cctx->header_bits, cctx->i_tex_bits,
					cctx->p_tex_bits, cctx->i_count, cctx->p_count, cctx->skip_count, cctx->misc_bits, cctx->frame_bits,
					cctx->stats_out, cctx->stats_in, cctx->workaround_bugs, cctx->strict_std_compliance, cctx->error_concealment,
					cctx->debug, cctx->debug_mv, cctx->err_recognition, cctx->reordered_opaque, cctx->error,
					cctx->dct_algo, cctx->idct_algo, cctx->bits_per_coded_sample, cctx->bits_per_raw_sample,
					cctx->lowres, cctx->thread_count, cctx->thread_type, cctx->active_thread_type, cctx->thread_safe_callbacks,
					cctx->nsse_weight, cctx->profile, cctx->level, cctx->skip_loop_filter, cctx->skip_idct, cctx->skip_frame,
					cctx->subtitle_header, cctx->subtitle_header_size, cctx->vbv_delay, cctx->side_data_only_packets,
					cctx->initial_padding, cctx->framerate.num, cctx->framerate.den, cctx->sw_pix_fmt, cctx->pkt_timebase.num, cctx->pkt_timebase.den,
					cctx->pts_correction_num_faulty_pts, cctx->pts_correction_num_faulty_dts, cctx->pts_correction_last_pts, cctx->pts_correction_last_dts,
					cctx->sub_charenc, cctx->sub_charenc_mode, cctx->skip_alpha, cctx->seek_preroll, cctx->chroma_intra_matrix, cctx->dump_separator,
					cctx->codec_whitelist, cctx->properties );

}

static int open_input_file(const char *filename)
{
    int ret;
    unsigned int i;
	ifmt_ctx = NULL;
    if ((ret = avformat_open_input(&ifmt_ctx, filename, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream *stream;
        AVCodecContext *codec_ctx;
        stream = ifmt_ctx->streams[i];
        codec_ctx = stream->codec;
        /* Reencode video & audio and remux subtitles etc. */
        if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
                || codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
            /* Open decoder */
            ret = avcodec_open2(codec_ctx,
                    avcodec_find_decoder(codec_ctx->codec_id), NULL);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Failed to open decoder for stream #%u\n", i);
                return ret;
            }
			AVCodecContextPrint("Decodec", i,codec_ctx);
        }
    }

    av_dump_format(ifmt_ctx, 0, filename, 0);
    return 0;
}

static int open_output_file(const char *filename)
{
    AVStream *out_stream;
    AVStream *in_stream;
    AVCodecContext *dec_ctx, *enc_ctx;
    AVCodec *encoder;
    int ret;
    unsigned int i;

    ofmt_ctx = NULL;
    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, filename);
    if (!ofmt_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
        return AVERROR_UNKNOWN;
    }


    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        out_stream = avformat_new_stream(ofmt_ctx, NULL);
        if (!out_stream) {
            av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream\n");
            return AVERROR_UNKNOWN;
        }

        in_stream = ifmt_ctx->streams[i];
        dec_ctx = in_stream->codec;
        enc_ctx = out_stream->codec;
        if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
                || dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
            /* in this example, we choose transcoding to same codec */

            encoder = avcodec_find_encoder(dec_ctx->codec_id);
            if (!encoder) {
                av_log(NULL, AV_LOG_FATAL, "Necessary encoder not found\n");
                return AVERROR_INVALIDDATA;
            }

			//enc_ctx->codec_id = dec_ctx->codec_id;
			av_log(NULL, AV_LOG_INFO, "Set encodec codec_id:%d\n",enc_ctx->codec_id);
            /* In this example, we transcode to same properties (picture size,
             * sample rate etc.). These properties can be changed for output
             * streams easily using filters */
            if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
				enc_ctx->me_range = 16;
				enc_ctx->max_qdiff = 4; 
				enc_ctx->qmin = 2; 
				enc_ctx->qmax = 51; 
				enc_ctx->qcompress = 0.6;  
				enc_ctx->me_cmp = 256;
				enc_ctx->mb_cmp = 10;
				enc_ctx->max_qdiff = 4;
				enc_ctx->trellis = 2;

                enc_ctx->height = dec_ctx->height;
                enc_ctx->width = dec_ctx->width;
                enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
                // take first format from list of supported formats //
                // video time_base can be set to whatever is handy and supported by encoder //
                enc_ctx->time_base = dec_ctx->time_base;
				enc_ctx->pkt_timebase = dec_ctx->pkt_timebase;
			    enc_ctx->framerate = dec_ctx->framerate;	
                enc_ctx->pix_fmt = encoder->pix_fmts[0];
				enc_ctx->frame_size = dec_ctx->frame_size;
				enc_ctx->gop_size = 250;//dec_ctx->gop_size;
				/*
				enc_ctx->bit_rate_tolerance = dec_ctx->bit_rate_tolerance;
				enc_ctx->b_quant_factor = dec_ctx->b_quant_factor;
				enc_ctx->b_quant_offset = dec_ctx->b_quant_offset;
				enc_ctx->i_quant_factor = dec_ctx->i_quant_factor;
				enc_ctx->i_quant_offset = dec_ctx->i_quant_offset;
				enc_ctx->ildct_cmp = dec_ctx->ildct_cmp;
				enc_ctx->mb_lmin = dec_ctx->mb_lmin;
				enc_ctx->mb_lmax = dec_ctx->mb_lmax;
				enc_ctx->me_penalty_compensation = dec_ctx->me_penalty_compensation;
				enc_ctx->bidir_refine = dec_ctx->bidir_refine;
				enc_ctx->keyint_min = dec_ctx->keyint_min;
				enc_ctx->refs = dec_ctx->refs;
				enc_ctx->color_primaries = dec_ctx->color_primaries;
				enc_ctx->chroma_sample_location = dec_ctx->chroma_sample_location;
				enc_ctx->sample_fmt = dec_ctx->sample_fmt;
				enc_ctx->timecode_frame_start = dec_ctx->timecode_frame_start;
				enc_ctx->profile = dec_ctx->profile;
				enc_ctx->level = dec_ctx->level;
				enc_ctx->bits_per_raw_sample = dec_ctx->bits_per_raw_sample;
				enc_ctx->rc_min_vbv_overflow_use = dec_ctx->rc_min_vbv_overflow_use;

				enc_ctx->b_sensitivity = dec_ctx->b_sensitivity;
				enc_ctx->mv0_threshold = dec_ctx->mv0_threshold;
				//enc_ctx->delay = dec_ctx->delay;

				*/

				AVCodecContextPrint("Video Encodec", i,enc_ctx);
            } else {

				ret = avcodec_copy_context(enc_ctx, dec_ctx);  
				if (ret < 0) {  
					av_log(NULL,AV_LOG_ERROR, "Failed to copy context from input to output stream codec context\n");  
					return ret;
				}  

                

			    enc_ctx->time_base = dec_ctx->time_base;
				enc_ctx->pkt_timebase = dec_ctx->pkt_timebase;
			    enc_ctx->framerate = dec_ctx->framerate;	
				enc_ctx->frame_size = dec_ctx->frame_size;
				enc_ctx->bit_rate_tolerance = dec_ctx->bit_rate_tolerance;
				enc_ctx->gop_size = 250;//dec_ctx->gop_size;
				enc_ctx->b_quant_factor = dec_ctx->b_quant_factor;
				enc_ctx->b_quant_offset = dec_ctx->b_quant_offset;
				enc_ctx->i_quant_factor = dec_ctx->i_quant_factor;
				enc_ctx->i_quant_offset = dec_ctx->i_quant_offset;
				enc_ctx->ildct_cmp = dec_ctx->ildct_cmp;
				enc_ctx->mb_lmin = dec_ctx->mb_lmin;
				enc_ctx->mb_lmax = dec_ctx->mb_lmax;
				enc_ctx->me_penalty_compensation = dec_ctx->me_penalty_compensation;
				enc_ctx->bidir_refine = dec_ctx->bidir_refine;
				enc_ctx->keyint_min = dec_ctx->keyint_min;
				enc_ctx->refs = dec_ctx->refs;
				enc_ctx->color_primaries = dec_ctx->color_primaries;
				enc_ctx->chroma_sample_location = dec_ctx->chroma_sample_location;
				enc_ctx->timecode_frame_start = dec_ctx->timecode_frame_start;
				enc_ctx->profile = dec_ctx->profile;
				enc_ctx->level = dec_ctx->level;
				enc_ctx->bits_per_raw_sample = dec_ctx->bits_per_raw_sample;
				enc_ctx->rc_min_vbv_overflow_use = dec_ctx->rc_min_vbv_overflow_use;


				enc_ctx->b_sensitivity = dec_ctx->b_sensitivity;
				enc_ctx->mv0_threshold = dec_ctx->mv0_threshold;
				

				enc_ctx->sample_fmt = encoder->sample_fmts[0];
				//enc_ctx->sample_fmt = dec_ctx->sample_fmt;//AV_SAMPLE_FMT_S16P;//encoder->sample_fmts[0];
                enc_ctx->sample_rate = dec_ctx->sample_rate;
                enc_ctx->channel_layout = dec_ctx->channel_layout;
                enc_ctx->channels = av_get_channel_layout_nb_channels(enc_ctx->channel_layout);
				



                // take first format from list of supported formats //
                //enc_ctx->time_base = (AVRational){1, enc_ctx->sample_rate};
				AVCodecContextPrint("Audio Encodec", i,enc_ctx);
            }

            /* Third parameter can be used to pass settings to encoder */
            ret = avcodec_open2(enc_ctx, encoder, NULL);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Cannot open video encoder for stream #%u\n", i);
                return ret;
            }
        } else if (dec_ctx->codec_type == AVMEDIA_TYPE_UNKNOWN) {
            av_log(NULL, AV_LOG_FATAL, "Elementary stream #%d is of unknown type, cannot proceed\n", i);
            return AVERROR_INVALIDDATA;
        } else {
            /* if this stream must be remuxed */
            ret = avcodec_copy_context(ofmt_ctx->streams[i]->codec,
                    ifmt_ctx->streams[i]->codec);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Copying stream context failed\n");
                return ret;
            }
        }

        if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    }
    av_dump_format(ofmt_ctx, 0, filename, 1);

    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'", filename);
            return ret;
        }
    }

    /* init muxer, write output file header */
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file\n");
        return ret;
    }

    return 0;
}

static int init_filter(FilteringContext* fctx, AVCodecContext *dec_ctx,
        AVCodecContext *enc_ctx, const char *filter_spec)
{
    char args[512];
    int ret = 0;
    AVFilter *buffersrc = NULL;
    AVFilter *buffersink = NULL;
    AVFilterContext *buffersrc_ctx = NULL;
    AVFilterContext *buffersink_ctx = NULL;
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    AVFilterGraph *filter_graph = avfilter_graph_alloc();

    if (!outputs || !inputs || !filter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
        buffersrc = avfilter_get_by_name("buffer");
        buffersink = avfilter_get_by_name("buffersink");
        if (!buffersrc || !buffersink) {
            av_log(NULL, AV_LOG_ERROR, "filtering source or sink element not found\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }

        snprintf(args, sizeof(args),
                "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
                dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
                dec_ctx->time_base.num, dec_ctx->time_base.den,
                dec_ctx->sample_aspect_ratio.num,
                dec_ctx->sample_aspect_ratio.den);

        ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                args, NULL, filter_graph);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
            goto end;
        }

        ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                NULL, NULL, filter_graph);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
            goto end;
        }

        ret = av_opt_set_bin(buffersink_ctx, "pix_fmts",
                (uint8_t*)&enc_ctx->pix_fmt, sizeof(enc_ctx->pix_fmt),
                AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
            goto end;
        }
    } else if (dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
        buffersrc = avfilter_get_by_name("abuffer");
        buffersink = avfilter_get_by_name("abuffersink");
        if (!buffersrc || !buffersink) {
            av_log(NULL, AV_LOG_ERROR, "filtering source or sink element not found\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }

        if (!dec_ctx->channel_layout)
            dec_ctx->channel_layout =
                av_get_default_channel_layout(dec_ctx->channels);
        snprintf(args, sizeof(args),
                "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%"PRIx64,
                dec_ctx->time_base.num, dec_ctx->time_base.den, dec_ctx->sample_rate,
                av_get_sample_fmt_name(dec_ctx->sample_fmt),
                dec_ctx->channel_layout);
        ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                args, NULL, filter_graph);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer source\n");
            goto end;
        }

        ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                NULL, NULL, filter_graph);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer sink\n");
            goto end;
        }

        ret = av_opt_set_bin(buffersink_ctx, "sample_fmts",
                (uint8_t*)&enc_ctx->sample_fmt, sizeof(enc_ctx->sample_fmt),
                AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot set output sample format\n");
            goto end;
        }

        ret = av_opt_set_bin(buffersink_ctx, "channel_layouts",
                (uint8_t*)&enc_ctx->channel_layout,
                sizeof(enc_ctx->channel_layout), AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot set output channel layout\n");
            goto end;
        }

        ret = av_opt_set_bin(buffersink_ctx, "sample_rates",
                (uint8_t*)&enc_ctx->sample_rate, sizeof(enc_ctx->sample_rate),
                AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot set output sample rate\n");
            goto end;
        }
    } else {
        ret = AVERROR_UNKNOWN;
        goto end;
    }

    /* Endpoints for the filter graph. */
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    inputs->name       = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    if (!outputs->name || !inputs->name) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    if ((ret = avfilter_graph_parse_ptr(filter_graph, filter_spec,
                    &inputs, &outputs, NULL)) < 0)
        goto end;

    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
        goto end;

    /* Fill FilteringContext */
    fctx->buffersrc_ctx = buffersrc_ctx;
    fctx->buffersink_ctx = buffersink_ctx;
    fctx->filter_graph = filter_graph;

end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return ret;
}

static int init_filters(void)
{
    const char *filter_spec;
    unsigned int i;
    int ret;
    filter_ctx = av_malloc_array(ifmt_ctx->nb_streams, sizeof(*filter_ctx));
    if (!filter_ctx)
        return AVERROR(ENOMEM);

    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        filter_ctx[i].buffersrc_ctx  = NULL;
        filter_ctx[i].buffersink_ctx = NULL;
        filter_ctx[i].filter_graph   = NULL;
        if (!(ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO
                || ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO))
            continue;


        if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
            filter_spec = "null"; /* passthrough (dummy) filter for video */
        else
            filter_spec = "anull"; /* passthrough (dummy) filter for audio */
        ret = init_filter(&filter_ctx[i], ifmt_ctx->streams[i]->codec,
                ofmt_ctx->streams[i]->codec, filter_spec);
        if (ret)
            return ret;
    }
    return 0;
}

static int encode_write_frame(AVFrame *filt_frame, unsigned int stream_index, int *got_frame) {
    int ret;
    int got_frame_local;
    AVPacket enc_pkt;
    int (*enc_func)(AVCodecContext *, AVPacket *, const AVFrame *, int *) =
        (ifmt_ctx->streams[stream_index]->codec->codec_type ==
         AVMEDIA_TYPE_VIDEO) ? avcodec_encode_video2 : avcodec_encode_audio2;

    if (!got_frame)
        got_frame = &got_frame_local;

	if(filt_frame != NULL){
		av_log(NULL, AV_LOG_INFO, "Encoding frame, streamid:%d, pts:%ld,pkt_pts:%ld, dts:%ld\n",
			stream_index,  filt_frame->pts, filt_frame->pkt_pts,filt_frame->pkt_dts);
	}
    /* encode filtered frame */
    enc_pkt.data = NULL;
    enc_pkt.size = 0;
    av_init_packet(&enc_pkt);
    ret = enc_func(ofmt_ctx->streams[stream_index]->codec, &enc_pkt,
            filt_frame, got_frame);
    av_frame_free(&filt_frame);
    if (ret < 0)
        return ret;
    if (!(*got_frame))
        return 0;

    /* prepare packet for muxing */
    enc_pkt.stream_index = stream_index;
    av_packet_rescale_ts(&enc_pkt,
                         ofmt_ctx->streams[stream_index]->codec->time_base,
                         ofmt_ctx->streams[stream_index]->time_base);

    av_log(NULL, AV_LOG_INFO, "Muxing frame, streamid:%d, pts:%ld, dts:%ld\n",
			stream_index, enc_pkt.pts,  enc_pkt.dts);
    /* mux encoded frame */
    ret = av_interleaved_write_frame(ofmt_ctx, &enc_pkt);
    return ret;
}

static int filter_encode_write_frame(AVFrame *frame, unsigned int stream_index)
{
    int ret;
    AVFrame *filt_frame;

    av_log(NULL, AV_LOG_INFO, "Pushing decoded frame to filters\n");
    /* push the decoded frame into the filtergraph */
    ret = av_buffersrc_add_frame_flags(filter_ctx[stream_index].buffersrc_ctx,
            frame, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
        return ret;
    }

    /* pull filtered frames from the filtergraph */
    while (1) {
        filt_frame = av_frame_alloc();
        if (!filt_frame) {
            ret = AVERROR(ENOMEM);
            break;
        }
        av_log(NULL, AV_LOG_INFO, "Pulling filtered frame from filters\n");
        ret = av_buffersink_get_frame(filter_ctx[stream_index].buffersink_ctx,
                filt_frame);
        if (ret < 0) {
            /* if no more frames for output - returns AVERROR(EAGAIN)
             * if flushed and no more frames for output - returns AVERROR_EOF
             * rewrite retcode to 0 to show it as normal procedure completion
             */
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                ret = 0;
            av_frame_free(&filt_frame);
            break;
        }

        filt_frame->pict_type = AV_PICTURE_TYPE_NONE;
		//filt_frame->pts = frame->pts;
		//filt_frame.pkt_pts = frame.pkt_pts;
		//filt_frame.pkt_dts = frame.pkt_dts;
        ret = encode_write_frame(filt_frame, stream_index, NULL);
        if (ret < 0)
            break;
    }

    return ret;
}

static int flush_encoder(unsigned int stream_index)
{
    int ret;
    int got_frame;

    if (!(ofmt_ctx->streams[stream_index]->codec->codec->capabilities &
                AV_CODEC_CAP_DELAY))
        return 0;

    while (1) {
        av_log(NULL, AV_LOG_INFO, "Flushing stream #%u encoder\n", stream_index);
        ret = encode_write_frame(NULL, stream_index, &got_frame);
        if (ret < 0)
            break;
        if (!got_frame)
            return 0;
    }
    return ret;
}

int last_frame2video(char * input, char * output, double duration){
    int ret;
    AVPacket packet = { .data = NULL, .size = 0 };
	//AVPacket pkg_out;
    AVFrame *frame = NULL;
	AVFrame *lastvideoframe = NULL;
	AVFrame *lastaudioframe = NULL;
    enum AVMediaType type;
    unsigned int stream_index;
    unsigned int i;
    int got_frame;
    int (*dec_func)(AVCodecContext *, AVFrame *, int *, const AVPacket *);

	int videoid =0;
	int audioid = 0;
	//double  duration = 1;
	AVRational vtimebase;
	AVRational atimebase;
	unsigned int adurationc ;
	unsigned int vdurationc ;
	unsigned int acount = 1;
	unsigned int vcount = 1;
	int audio_frame_size_out_encode ;
	int video_frame_size_out_encode ;


    av_register_all();
    avfilter_register_all();

	//duration = atof(du);
	if (0 == duration){
		return 1;
	}

    if ((ret = open_input_file(input) < 0))
        goto end;
    if ((ret = open_output_file(output)) < 0)
        goto end;
    if ((ret = init_filters()) < 0)
        goto end;

	/*read last packets*/
    while (1) {
        if ((ret = av_read_frame(ifmt_ctx, &packet)) < 0)
            break;
        stream_index = packet.stream_index;
        type = ifmt_ctx->streams[packet.stream_index]->codec->codec_type;
        av_log(NULL, AV_LOG_DEBUG, "Demuxer gave frame of stream_index %u\n",
                stream_index);

        if (filter_ctx[stream_index].filter_graph) {
            av_log(NULL, AV_LOG_DEBUG, "Going to reencode&filter the frame\n");
            frame = av_frame_alloc();
            if (!frame) {
                ret = AVERROR(ENOMEM);
                break;
            }
            av_packet_rescale_ts(&packet,
                                 ifmt_ctx->streams[stream_index]->time_base,
                                 ifmt_ctx->streams[stream_index]->codec->time_base);
            dec_func = (type == AVMEDIA_TYPE_VIDEO) ? avcodec_decode_video2 :
                avcodec_decode_audio4;
			if (type == AVMEDIA_TYPE_VIDEO){
				//vtimebase = ifmt_ctx->streams[stream_index]->codec->time_base;
				vtimebase = ofmt_ctx->streams[videoid]->codec->time_base;
				videoid = stream_index;
			}else if(type == AVMEDIA_TYPE_AUDIO){
				audioid = stream_index;
				atimebase = ifmt_ctx->streams[stream_index]->codec->time_base;
			}
            ret = dec_func(ifmt_ctx->streams[stream_index]->codec, frame,
                    &got_frame, &packet);
            if (ret < 0) {
                av_frame_free(&frame);
                av_log(NULL, AV_LOG_ERROR, "Decoding failed\n");
                break;
            }



            if (got_frame && type == AVMEDIA_TYPE_VIDEO) {

				if (lastvideoframe != NULL){
					av_frame_free(&lastvideoframe);
				}
                frame->pts = av_frame_get_best_effort_timestamp(frame);
				lastvideoframe = frame;

            } else if (got_frame && type == AVMEDIA_TYPE_AUDIO){
				if (lastaudioframe != NULL){
					av_frame_free(&lastaudioframe);
				}
                frame->pts = av_frame_get_best_effort_timestamp(frame);
				lastaudioframe = frame;

			} else {

				if(type == AVMEDIA_TYPE_VIDEO && lastvideoframe == NULL){
					av_log(NULL, AV_LOG_INFO, "---------------------- Decoding got_frame:%d, frame:%d\n",
							got_frame, frame);
					lastvideoframe = frame;
					
				}else{
					av_frame_free(&frame);
					
				}

            }
        } 
		av_free_packet(&packet);
    }
	if (lastvideoframe == NULL || lastaudioframe == NULL){
            av_log(NULL, AV_LOG_ERROR, "Get last frame failly.lastvideoframe:%d, lastaudioframe:%d\n",
					lastvideoframe, lastaudioframe);
			goto end;
	}

	audio_frame_size_out_encode = ofmt_ctx->streams[audioid]->codec->frame_size;
	// 1/timebase * 1/framerate = how many count base on codec->time_base,
	unsigned  int ccount = ofmt_ctx->streams[videoid]->codec->time_base.den * ofmt_ctx->streams[videoid]->codec->framerate.den/ \
						   (ofmt_ctx->streams[videoid]->codec->time_base.num * ofmt_ctx->streams[videoid]->codec->framerate.num); 
	video_frame_size_out_encode = ccount;

	adurationc = duration * ofmt_ctx->streams[audioid]->codec->time_base.den /ofmt_ctx->streams[audioid]->codec->time_base.num;
	vdurationc = duration * ofmt_ctx->streams[videoid]->codec->time_base.den /ofmt_ctx->streams[videoid]->codec->time_base.num; 
	lastaudioframe->channels = av_get_channel_layout_nb_channels(lastaudioframe->channel_layout);  

	av_samples_set_silence(lastaudioframe->data, 0, lastaudioframe->nb_samples, lastaudioframe->channels, lastaudioframe->format); 
	//don't ask me why 8.
	lastaudioframe->pts =  av_rescale_q(lastvideoframe->pts + 8,
							ofmt_ctx->streams[videoid]->codec->time_base,
							ofmt_ctx->streams[audioid]->codec->time_base);

	av_log(NULL, AV_LOG_INFO, "+++++++++ adurationc:%d, vdurationc:%d +++++++\n"
							  "+++++++++ audio_frame_size_out_encode:%d,video_frame_size_out_encode:%d ++++++\n"
							  "+++++++++ lastvideoframe->pts:%d, v_time_base:%d/%d, lastaudioframe->pts:%d, a_time_base:%d/%d ++++++\n",
							  adurationc,vdurationc,
							  audio_frame_size_out_encode,
							  video_frame_size_out_encode,
							  lastvideoframe->pts, 
							  ofmt_ctx->streams[videoid]->codec->time_base.num,
							  ofmt_ctx->streams[videoid]->codec->time_base.den,
							  lastaudioframe->pts,
							  ofmt_ctx->streams[audioid]->codec->time_base.num,
							  ofmt_ctx->streams[audioid]->codec->time_base.den);

	int ac = 0;
	int vc = 0;
	while(acount <= adurationc+1 || vcount <= vdurationc +1){
			if(vcount <= vdurationc+1 && vdurationc/vcount >=  adurationc/acount){
				ret = filter_encode_write_frame(lastvideoframe, videoid);
				if (ret < 0)
					goto end;
				vc +=1;
				vcount += video_frame_size_out_encode ;	
				lastvideoframe->pts += video_frame_size_out_encode;
				lastvideoframe->pkt_pts += video_frame_size_out_encode;
				lastvideoframe->pkt_dts += video_frame_size_out_encode;
				av_log(NULL, AV_LOG_INFO, "--- Video proc, vcount:%d,vdurationc:%d, adurationc:%d, vdurationc/vcount:%f, adurationc/acount:%f\n",
						vcount, vdurationc, adurationc,  vdurationc/(double)vcount,adurationc/(double)acount);
		

			}
			if(acount <= adurationc+1 &&  vdurationc/vcount <= adurationc/acount){
				ac += 1;
				ret = filter_encode_write_frame(lastaudioframe, audioid);
				if (ret < 0)
					goto end;
				acount +=audio_frame_size_out_encode; 
				lastaudioframe->pts += audio_frame_size_out_encode; 
				lastaudioframe->pkt_pts += audio_frame_size_out_encode; 
				lastaudioframe->pkt_dts  += audio_frame_size_out_encode; 
				av_log(NULL, AV_LOG_INFO, "--- Audio proc, acount:%d,vdurationc:%d, adurationc:%d, vdurationc/vcount:%f,  adurationc/acount:%f\n",
						acount, vdurationc, adurationc, vdurationc/(double)vcount, adurationc/(double)acount);

			}

		
	}
	av_log(NULL, AV_LOG_INFO, "--- Audio Frame num:%d, Video Frame num:%d.\n", ac, vc);


    // flush filters and encoders //
    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        // flush filter //
        if (!filter_ctx[i].filter_graph)
            continue;
		av_log(NULL, AV_LOG_INFO, "--- flush filter stream:%d\n",i);
        ret = filter_encode_write_frame(NULL, i);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Flushing filter failed\n");
            goto end;
        }
		av_log(NULL, AV_LOG_INFO, "--- flush encoder stream:%d\n",i);
 

        // flush encoder 
        ret = flush_encoder(i);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Flushing encoder failed\n");
            goto end;
        }
    }

    av_write_trailer(ofmt_ctx);
end:
    av_free_packet(&packet);
    //av_frame_free(&frame);
    av_frame_free(&lastvideoframe);
    av_frame_free(&lastaudioframe);
    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        avcodec_close(ifmt_ctx->streams[i]->codec);
        if (ofmt_ctx && ofmt_ctx->nb_streams > i && ofmt_ctx->streams[i] && ofmt_ctx->streams[i]->codec)
            avcodec_close(ofmt_ctx->streams[i]->codec);
        if (filter_ctx && filter_ctx[i].filter_graph)
            avfilter_graph_free(&filter_ctx[i].filter_graph);
    }
    av_free(filter_ctx);
    avformat_close_input(&ifmt_ctx);
    if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);

    if (ret < 0)
        av_log(NULL, AV_LOG_ERROR, "Error occurred: %s\n", av_err2str(ret));

    return ret ? 1 : 0;


}


int t_main(int argc, char **argv)
{
    if (argc != 4) {
        av_log(NULL, AV_LOG_ERROR, "Usage: %s <input file> <output file> <duration>\n", argv[0]);
        return 1;
    }

	double duration = atof(argv[3]);
	if (0 == duration){
		av_log(NULL, AV_LOG_ERROR, "Usage: %s <input file> <output file> <duration>\n duration error!\n", argv[0]);
		return 1;
	}
	char * input = argv[1];
	char * output = argv[2];
	while(100){
		int rst = last_frame2video(input, output, duration);
	}
	
}
