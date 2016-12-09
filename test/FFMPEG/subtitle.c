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
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
    {
        fprintf(stderr, "Could not initialize SDL %s\n", SDL_GetError());

        return -1;
    }

    avcodec_register_all();
    av_register_all();


    AVFormatContext *pFormatCtx;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVFrame *pFrame, *pFrameRGB;
    AVPicture pict;
    SDL_Rect rect;
    struct SwsContext *img_convert_ctx;
    SDL_Surface *screen;
    SDL_Overlay *bmp;
    uint8_t *buffer;
    int videoStream = -1;
    int numBytes;

    fprintf(stdout, "%s\n", argv[1]);

    if (av_open_input_file(&pFormatCtx, argv[1], NULL, 0, NULL) != 0)
    {
        fprintf(stderr, "av_open_input_file \n");

        return -1;
    }
    if (av_find_stream_info(pFormatCtx) < 0)
    {
        fprintf(stderr, "av_find_stream_info \n");

        return -1;
    }

    dump_format(pFormatCtx, 0, argv[1], 0);

    int i;
    for (i = 0; i < pFormatCtx->nb_streams; ++i)
    {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_SUBTITLE)
        {
            videoStream = i;

            break ;
        }
    }

    if (videoStream == -1)
    {
        fprintf(stderr, "Unsupported videoStream.\n");

        return -1;
    }

    fprintf(stdout, "videoStream: %d\n", videoStream);

    pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL)
    {
        fprintf(stderr, "Unsupported codec.\n");

        return -1;
    }
    if (avcodec_open(pCodecCtx, pCodec) < 0)
    {
        fprintf(stderr, "Could not open codec.\n");

        return -1;
    }

    int index;
    int ret = 0;
    AVPacket packet;
    av_init_packet(&packet);
    AVSubtitle sub;
    int len1, gotSubtitle;

    for (index = 0; ; index++)
    {
        //memset(&packet, 0, sizeof (packet));
        ret = av_read_frame(pFormatCtx, &packet);
        if (ret < 0)
        {
            fprintf(stderr, "read frame ret:%s\n", ret_str(ret));

            break ;
        }

        fprintf(stdout, "stream_index:%d\n", packet.stream_index);

        if (packet.stream_index == videoStream)
        {
            ret = avcodec_decode_subtitle2(pCodecCtx, &sub, &gotSubtitle, &packet);

            if (index > 30)
            {
                break ;
            }

            fprintf(stdout, "gotSubtitle:%d\n", gotSubtitle);
            dumpSubtitle(&sub);
        }
        av_free_packet(&packet);
    }

end:
    avcodec_close(pCodecCtx);
    av_close_input_file(pFormatCtx);

    fprintf(stderr, "end return.\n");

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