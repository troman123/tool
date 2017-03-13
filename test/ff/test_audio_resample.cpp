<pre name="code" class="cpp">/*
*最简单的音频转码器（只处理音频）
*缪国凯 Mickel
*821486004@qq.com
*本程序实现从一个视频格式转码到另一个视频格式，只处理音频，视频忽略，若有多个音频流，只处理第一个
*2015-5-8
*/


#include "stdafx.h"

#ifdef __cplusplus
extern"C"
{
#endif
#include <libavformat/avformat.h>
#include "libavcodec/avcodec.h"
#include "libavfilter/avfiltergraph.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/avutil.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"

// #pragma comment(lib, "avcodec.lib")
// #pragma comment(lib, "avformat.lib")
// #pragma comment(lib, "avutil.lib")
//#pragma comment(lib, "avdevice.lib")
// #pragma comment(lib, "avfilter.lib")
//#pragma comment(lib, "postproc.lib")
//#pragma comment(lib, "swresample.lib")
//#pragma comment(lib, "swscale.lib")
#ifdef __cplusplus
};
#endif

typedef struct FilteringContext 
{
    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc_ctx;
    AVFilterGraph *filter_graph;
} FilteringContext;

static FilteringContext *filter_ctx;
AVStream *out_stream = NULL;
AVFormatContext *in_fmt_ctx = NULL, *out_fmt_ctx = NULL;

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

    if (!outputs || !inputs || !filter_graph) 
    {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    if (dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) 
    {
        buffersrc = avfilter_get_by_name("abuffer");
        buffersink = avfilter_get_by_name("abuffersink");
        if (!buffersrc || !buffersink)
        {
            av_log(NULL, AV_LOG_ERROR, "filtering source or sink element not found\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }
        if (!dec_ctx->channel_layout)
        {
            dec_ctx->channel_layout = av_get_default_channel_layout(dec_ctx->channels);
        }

        //format args
        _snprintf(args, sizeof(args),
            "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%I64x",
            dec_ctx->time_base.num, dec_ctx->time_base.den, dec_ctx->sample_rate,
            av_get_sample_fmt_name(dec_ctx->sample_fmt),
            dec_ctx->channel_layout);

        ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, NULL, filter_graph);
        if (ret < 0) 
        {
            av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer source\n");
            goto end;
        }

//      char args1[512];
//      _snprintf(args1, sizeof(args1),
//          "sample_rates=%d:sample_fmts=%s:channel_layouts=0x%I64x",
//          (uint8_t)enc_ctx->sample_rate, (uint8_t)enc_ctx->sample_fmt,
//          (uint8_t)enc_ctx->channel_layout);

        ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, NULL, filter_graph);
        if (ret < 0) 
        {
            av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer sink\n");
            goto end;
        }

        ret = av_opt_set_bin(buffersink_ctx, "sample_fmts", (uint8_t*)&enc_ctx->sample_fmt, 
            sizeof(enc_ctx->sample_fmt), AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) 
        {
            av_log(NULL, AV_LOG_ERROR, "Cannot set output sample format\n");
            goto end;
        }

        ret = av_opt_set_bin(buffersink_ctx, "channel_layouts", (uint8_t*)&enc_ctx->channel_layout,
            sizeof(enc_ctx->channel_layout), AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) 
        {
            av_log(NULL, AV_LOG_ERROR, "Cannot set output channel layout\n");
            goto end;
        }

        ret = av_opt_set_bin(buffersink_ctx, "sample_rates", (uint8_t*)&enc_ctx->sample_rate, 
            sizeof(enc_ctx->sample_rate), AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) 
        {
            av_log(NULL, AV_LOG_ERROR, "Cannot set output sample rate\n");
            goto end;
        }
    }
    else 
    {
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

    if (!outputs->name || !inputs->name) 
    {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    if ((ret = avfilter_graph_parse_ptr(filter_graph, filter_spec, &inputs, &outputs, NULL)) < 0)
    {
        goto end;
    }

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

    filter_ctx = (FilteringContext *)av_malloc_array(1, sizeof(*filter_ctx));

    if (!filter_ctx)
    {
        return AVERROR(ENOMEM);
    }

    for (i = 0; i < in_fmt_ctx->nb_streams; i++) 
    {
        filter_ctx[i].buffersrc_ctx  = NULL;
        filter_ctx[i].buffersink_ctx = NULL;
        filter_ctx[i].filter_graph   = NULL;

        if (in_fmt_ctx->streams[i]->codec->codec_type != AVMEDIA_TYPE_AUDIO)
        {
            continue;
        }

        filter_spec = "anull"; /* passthrough (dummy) filter for audio */

        ret = init_filter(&filter_ctx[0], in_fmt_ctx->streams[i]->codec,
            out_fmt_ctx->streams[out_stream->index]->codec, filter_spec);

        if (ret)
        {
            return ret;
        }
    }
    return 0;
}

int flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index)
{
    int ret;
    int got_frame;
    AVPacket enc_pkt;
    if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities &
        CODEC_CAP_DELAY))
    {
        return 0;
    }
    int i = 0;
    while(1)
    {
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        ret = avcodec_encode_audio2(out_fmt_ctx->streams[stream_index]->codec, &enc_pkt,
            NULL, &got_frame);
        
        if (ret < 0)
        {
            break;
        }
        if (!got_frame)
        {
            break;
        }
        /* prepare packet for muxing */
        enc_pkt.stream_index = stream_index;
        enc_pkt.dts = av_rescale_q_rnd(enc_pkt.dts,
            out_fmt_ctx->streams[stream_index]->codec->time_base,
            out_fmt_ctx->streams[stream_index]->time_base,
            (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        enc_pkt.pts = av_rescale_q_rnd(enc_pkt.pts,
            out_fmt_ctx->streams[stream_index]->codec->time_base,
            out_fmt_ctx->streams[stream_index]->time_base,
            (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        enc_pkt.duration = av_rescale_q(enc_pkt.duration,
            out_fmt_ctx->streams[stream_index]->codec->time_base,
            out_fmt_ctx->streams[stream_index]->time_base);

        /* mux encoded frame */
        ret = av_interleaved_write_frame(out_fmt_ctx, &enc_pkt);
        av_free_packet(&enc_pkt);
        if (ret < 0)
        {
            break;
        }
        i++;
    }
    printf("flusher write %d frame", i);
    return ret;
}

int _tmain(int argc, _TCHAR* argv[])
{
    AVFrame *frame;
    AVPacket pkt_in, pkt_out;
    int audio_index = -1;
    int ret;

    if (argc < 3)
    {
        printf("error in input param");
        getchar();
        return -1;
    }

    av_register_all();
    avfilter_register_all();
    //input
    if (avformat_open_input(&in_fmt_ctx, argv[1], NULL, NULL) < 0)
    {
        printf("can not open input file context");
        goto end;
    }
    if (avformat_find_stream_info(in_fmt_ctx, NULL) < 0)
    {
        printf("can not find input stream info!\n");
        goto end;
    }

    //output
    avformat_alloc_output_context2(&out_fmt_ctx, NULL, NULL, argv[2]);
    if (!out_fmt_ctx)
    {
        printf("can not alloc output context!\n");
        goto end;
    }
    //open decoder & new out stream & open encoder
    for (int i = 0; i < in_fmt_ctx->nb_streams; i++)
    {
        if (in_fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            //open decoder
            if(0 > avcodec_open2(in_fmt_ctx->streams[i]->codec, avcodec_find_decoder(in_fmt_ctx->streams[i]->codec->codec_id), NULL))
            {
                printf("can not find or open decoder!\n");
                goto end;
            }

            audio_index = i;

            //new stream
            out_stream = avformat_new_stream(out_fmt_ctx, NULL);
            if (!out_stream)
            {
                printf("can not new stream for output!\n");
                goto end;
            }

            //set codec context param           
            //use default audio encoder
            out_stream->codec->codec = avcodec_find_encoder(out_fmt_ctx->oformat->audio_codec);
            //use the input audio encoder
            //out_stream->codec->codec = avcodec_find_encoder(ifmt_ctx->streams[i]->codec->codec_id);

            out_stream->codec->sample_rate = in_fmt_ctx->streams[i]->codec->sample_rate;
            out_stream->codec->channel_layout = in_fmt_ctx->streams[i]->codec->channel_layout;
            out_stream->codec->channels = av_get_channel_layout_nb_channels(out_stream->codec->channel_layout);
            // take first format from list of supported formats
            out_stream->codec->sample_fmt = out_stream->codec->codec->sample_fmts[0];
            AVRational time_base={1, out_stream->codec->sample_rate};
            out_stream->codec->time_base = time_base;

            //open encoder
            if (!out_stream->codec->codec)
            {
                printf("can not find the encoder!\n");
                goto end;
            }
            if ((avcodec_open2(out_stream->codec, out_stream->codec->codec, NULL)) < 0)
            {
                printf("can not open the encoder\n");
                goto end;
            }

            if (out_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            {
                out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
            }

            break;
        }
    }

    //dump input info
    av_dump_format(in_fmt_ctx, 0, argv[1], 0);
    //dump output info
    av_dump_format(out_fmt_ctx, 0, argv[2], 1);

    if (-1 == audio_index)
    {
        printf("found no audio stream in input file!\n");
        goto end;
    }

    if (!(out_fmt_ctx->oformat->flags & AVFMT_NOFILE))
    {
        if(avio_open(&out_fmt_ctx->pb, argv[2], AVIO_FLAG_WRITE) < 0)
        {
            printf("can not open output file handle!\n");
            goto end;
        }
    }

    if(avformat_write_header(out_fmt_ctx, NULL) < 0)
    {
        printf("can not write the header of the output file!\n");
        goto end;
    }

    //init filter
    if ((ret = init_filters()) < 0)
    {
        goto end;
    }
    
    av_init_packet(&pkt_out);
    
    int got_frame, got_picture;
    int frame_index = 0;
    int i = 0;
    for(int i = 0; ;i++)
    {
        av_init_packet(&pkt_out);
        pkt_in.data = NULL;
        pkt_in.size = 0;

        got_frame = -1;
        got_picture = -1;
        
        if (av_read_frame(in_fmt_ctx, &pkt_in) < 0)
        {
            break;
        }
        if (pkt_in.stream_index != audio_index)
        {
            continue;
        }
        frame = av_frame_alloc();
        if ((ret = avcodec_decode_audio4(in_fmt_ctx->streams[audio_index]->codec, frame, &got_frame, &pkt_in)) < 0)
        {
            av_frame_free(&frame);
            printf("can not decoder a frame");
            break;
        }
        av_free_packet(&pkt_in);

        if (got_frame)
        {
            frame->pts = av_frame_get_best_effort_timestamp(frame);
            AVFrame *filt_frame;
            ret = av_buffersrc_add_frame_flags(filter_ctx[0].buffersrc_ctx, frame, 0);
            if (ret < 0) 
            {
                av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
                break;
            }

            /* pull filtered frames from the filtergraph */
            while (1) 
            {
                filt_frame = av_frame_alloc();
                if (!filt_frame) 
                {
                    ret = AVERROR(ENOMEM);
                    break;
                }

                ret = av_buffersink_get_frame(filter_ctx[0].buffersink_ctx, filt_frame);
                if (ret < 0) 
                {
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
                
                pkt_out.data = NULL;
                pkt_out.size = 0;
                av_init_packet(&pkt_out);

                ret = avcodec_encode_audio2(out_fmt_ctx->streams[out_stream->index]->codec, &pkt_out, 
                    filt_frame, &got_picture);
                av_frame_free(&filt_frame);

                if (ret < 0)
                {
                    printf("encode a frame failed!\n");
                    break;
                }
                if (got_picture)
                {
                    /* prepare packet for muxing */
                    pkt_out.stream_index = out_stream->index;
                    pkt_out.dts = av_rescale_q_rnd(pkt_out.dts,
                        out_fmt_ctx->streams[out_stream->index]->codec->time_base,
                        out_fmt_ctx->streams[out_stream->index]->time_base,
                        (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));

                    pkt_out.pts = av_rescale_q_rnd(pkt_out.pts,
                        out_fmt_ctx->streams[out_stream->index]->codec->time_base,
                        out_fmt_ctx->streams[out_stream->index]->time_base,
                        (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));

                    pkt_out.duration = av_rescale_q(pkt_out.duration,
                        out_fmt_ctx->streams[out_stream->index]->codec->time_base,
                        out_fmt_ctx->streams[out_stream->index]->time_base);

                    av_log(NULL, AV_LOG_DEBUG, "Muxing frame\n");
                    /* mux encoded frame */
                    ret = av_interleaved_write_frame(out_fmt_ctx, &pkt_out);

                    if (ret < 0)
                    {
                        printf("write a frame failed!\n");
                        break;
                    }
                    printf("success write a frame ,index:%d\n", frame_index);
                    frame_index++;
                }
            }
            av_frame_free(&frame);
            av_free_packet(&pkt_out);
        }
    }
    ret = flush_encoder(out_fmt_ctx, out_stream->index);

    if (ret < 0)
    {
        printf("Flushing encoder failed");
        return -1;
    }

    //write file trailer
    av_write_trailer(out_fmt_ctx);

    //clean
    avcodec_close(out_stream->codec);
    avcodec_close(in_fmt_ctx->streams[audio_index]->codec);

end:
    avformat_close_input(&in_fmt_ctx);

    if (out_fmt_ctx && !(out_fmt_ctx->oformat->flags & AVFMT_NOFILE))
    {
        avio_close(out_fmt_ctx->pb);
    }
    avformat_free_context(out_fmt_ctx);
    getchar();
    return 0;
}


