#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

void saveFrame(const AVFrame *pFrame, int width, int height, int iFrame);
static const char *ret_str(int v);

static void dumpSubtitle(AVSubtitle *sub)
{
    if (sub == NULL)
    {
        return ;
    }

    fprintf(stdout, "%p\n", sub);
    fprintf(stdout, "format:%u \n", sub->format);
    fprintf(stdout, "start_display_time:%u \n", sub->start_display_time);
    fprintf(stdout, "end_display_time:%u \n", sub->end_display_time);
    fprintf(stdout, "num_rects:%u \n", sub->num_rects);
    fprintf(stdout, "pts:%lld \n", sub->pts);

    int i;
    for (i = 0; i < sub->num_rects; ++i)
    {
        AVSubtitleRect *rect = sub->rects[i];
        fprintf(stdout, "x:%d\n", rect->x);
        fprintf(stdout, "y:%d\n", rect->y);
        fprintf(stdout, "w:%d\n", rect->w);
        fprintf(stdout, "h:%d\n", rect->h);
        fprintf(stdout, "h:%06x\n", rect->nb_colors);

        switch(rect->type)
        {
        case SUBTITLE_NONE:
            fprintf(stdout, "SUBTITLE_NONE\n");

            break ;
        case SUBTITLE_TEXT:
            fprintf(stdout, "SUBTITLE_TEXT\n");

            break ;
        case SUBTITLE_BITMAP:
            fprintf(stdout, "SUBTITLE_BITMAP\n");

            break ;
        case SUBTITLE_ASS:
            fprintf(stdout, "SUBTITLE_ASS\n");

            break ;
        default:
            break;
        }

        fprintf(stdout, "text:%s\n", rect->text);
        fprintf(stdout, "ass:%s\n", rect->ass);
    }
}

int main(int argc, char* argv[])
{
    avcodec_register_all();
    av_register_all();


    AVFormatContext *pFormatCtx = NULL;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVFrame *pFrame, *pFrameRGB;
    AVPicture pict;
    struct SwsContext *img_convert_ctx;
    uint8_t *buffer;
    int videoStream = -1;
    int numBytes;

    fprintf(stdout, "%s\n", argv[1]);
    AVInputFormat *fileFormat;
    //fileFormat = av_find_input_format("ass");
    if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0)
    {
        fprintf(stderr, "av_open_input_file \n");

        return -1;
    }
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        fprintf(stderr,"av_find_stream_info \n");

        return -1;
    }

    av_dump_format(pFormatCtx, 0, argv[1], 0);

    int i;
    for (i = 0; i < pFormatCtx->nb_streams; ++i)
    {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            fprintf(stdout, ".audioindex: %d\n", i);
        }
    }

    avformat_close_input(&pFormatCtx);


    return 0;
}

static const char *ret_str(int v)
{
    char buffer[20];
    switch (v) {
    case AVERROR_EOF:     return "-EOF";
    case AVERROR(EIO):    return "-EIO";
    case AVERROR(ENOMEM): return "-ENOMEM";
    case AVERROR(EINVAL): return "-EINVAL";
    default:
        snprintf(buffer, sizeof(buffer), "%2d", v);
        return buffer;
    }
}

void saveFrame(const AVFrame *pFrame, int width, int height, int iFrame)
{
    FILE *pFile;
    char szFileName[32];

    sprintf(szFileName, "saveframe/frame%d.ppm", iFrame);
    pFile = fopen(szFileName, "wb");
    if (pFile == NULL)
    {
        fprintf(stderr, "pFile NULL\n");

        return ;
    }
    fprintf(stdout, "pFrame:%0X, szFileName:%s", (int)pFrame, szFileName);

    fprintf(pFile, "P6\n%d %d\n255\n", width, height);
    int i;
    for (i = 0; i < height; ++i)
    {
        fwrite(pFrame->data[0] + i * pFrame->linesize[0], 1, width * 3, pFile);
    }

    fclose(pFile);
}