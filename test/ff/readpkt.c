#include <stdio.h>
#include <stdarg.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

typedef struct context_sys_t
{
    AVFormatContext *pFormatCtx;
    AVCodecContext *pVideoContext;
    AVCodecContext *pAudioContext;

    int videoStreamIndex;
    int audioStreamIndex;

    FILE *fp;
} context_sys_t;

int init(void **ptr)
{
    context_sys_t* p_ctx = (context_sys_t*)malloc(sizeof(context_sys_t));
    *ptr = p_ctx;

    av_register_all();
    avcodec_register_all();

    av_log_set_level(AV_LOG_DEBUG);

    AVFormatContext *pFormatctx = avformat_alloc_context();
    if (avformat_open_input(&pFormatctx, "../../../test/youkuYY.mp4", NULL, NULL) != 0)
    {
        return -1;
    }
    p_ctx->pFormatCtx = pFormatctx;

    if (avformat_find_stream_info(pFormatctx, NULL) < 0)
    {
        return -2;
    }

    av_dump_format(pFormatctx, 0, NULL, 0);

    int i;
    for (i = 0; i < pFormatctx->nb_streams; ++i)
    {
        if (pFormatctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            p_ctx->pVideoContext = pFormatctx->streams[i]->codec;
            p_ctx->videoStreamIndex = i;
            fprintf(stdout, "format video stream index =%d\n", i);
        }
        else if (pFormatctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            p_ctx->pAudioContext = pFormatctx->streams[i]->codec;
            p_ctx->audioStreamIndex = i;
            fprintf(stdout, "format audio stream index =%d\n", i);
        }
    }

    AVCodec *pCodec = NULL;
    pCodec = avcodec_find_decoder(p_ctx->pVideoContext->codec_id);
    if (pCodec == NULL)
    {
        fprintf(stdout, "Unsupported video codec.");
    }

    int ret = avcodec_open2(p_ctx->pVideoContext, pCodec, 0);
    if (ret != 0)
    {
        fprintf(stderr, "can not open video codec.");
    }

    pCodec = avcodec_find_decoder(p_ctx->pAudioContext->codec_id);
    if (pCodec == NULL)
    {
        fprintf(stderr, "Unsupported audio codec.");
    }

    ret = avcodec_open2(p_ctx->pAudioContext, pCodec, 0);
    if (ret != 0)
    {
        fprintf(stderr, "can not open audio codec.\n");
    }

    p_ctx->fp = fopen("io.log", "a+");
    if (p_ctx->fp == NULL)
    {
        fprintf(stdout, "%s", strerror(errno));

        return -3;
    }

    return 0;
}

void dumpMemory(context_sys_t* p_ctx, uint8_t *buf, int len, char *fmt, ...)
{
    FILE *fp = p_ctx->fp;

    va_list va;
    va_start(va, fmt);
    vfprintf(stdout, fmt, va);
    va_end(va);

    if (buf != NULL && len != 0)
    {
        //char buf[10] = {0};
        int index = 0;
        while (index < len)
        {
            fprintf(stdout, "%02x", *(buf + (index++)));
        }
    }

    fprintf(stdout, " \n");
}

void readPkt(void *ptr)
{
    context_sys_t* p_ctx = (context_sys_t*)ptr;
    AVPacket pkt;
    av_init_packet(&pkt);
    int got_picture = 0;
    AVFrame *pFrame = av_frame_alloc();
    int count = 0;
    while (av_read_frame(p_ctx->pFormatCtx, &pkt) == 0 && count++ < 100)
    {
        dumpMemory(p_ctx, pkt.data, pkt.size, "avpacket buf=%p data=%p len=%d stream_index=%d  ", pkt.buf, pkt.data, pkt.size, pkt.stream_index);
        // if (pkt.stream_index == p_ctx->videoStreamIndex)
        // {
        //     dumpMemory(p_ctx, pkt.data, pkt.size, "avpacket buf=%p data=%p len=%d stream_index=%d  ", pkt.buf, pkt.data, pkt.size, pkt.stream_index);
        //     //avcodec_decode_video2(p_ctx->pVideoContext, pFrame, &got_picture, &pkt);
        //     fprintf(stdout, "avpacket len=%d pts=%lld dts=%lld  stream_index=%d\n",
        //         pkt.size, pkt.pts, pkt.dts, pkt.stream_index);
        //     //dumpMemory(p_ctx, pkt.data, pkt.size, "avpacket buf=%p data=%p len=%d stream_index=%d  ", pkt.buf, pkt.data, pkt.size, pkt.stream_index);
        // }
        if (pkt.stream_index == p_ctx->audioStreamIndex)
        {
            AVPacket tmpPkt;
            tmpPkt = pkt;
            int size = 0;
            do
            {
                //avcodec_get_frame_defaults(pFrame);
                size = avcodec_decode_audio4(p_ctx->pAudioContext, pFrame, &got_picture, &tmpPkt);
                if (size >= 0)
                {
                    tmpPkt.size -= size;
                    tmpPkt.data += size;
                }

                fprintf(stdout, "got_picture=%d size=%d size=%d\n", got_picture, size, tmpPkt.size);
                if (got_picture)
                {

                }
                else
                {
                    break;
                }
                
                
            } while (tmpPkt.size > 0);
            

            //avcodec_decode_video2(p_ctx->pVideoContext, pFrame, &got_picture, &pkt);
        }

        av_free_packet(&pkt);
        
    }

    fclose(p_ctx->fp);
}