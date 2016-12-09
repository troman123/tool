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
    fileFormat = av_find_input_format("ass");
    if (avformat_open_input(&pFormatCtx, argv[1], fileFormat, NULL) != 0)
    {
        fprintf(stderr, "av_open_input_file \n");

        return -1;
    }
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        fprintf(stderr, "av_find_stream_info \n");

        return -1;
    }

    av_dump_format(pFormatCtx, 0, argv[1], 0);

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

    char *charenc = av_malloc(10);//"GBK";
    memset(charenc, 0, 10);
    memcpy(charenc, "GBK", strlen("GBK"));

    printf("charenc :%p\n", charenc);

    pCodecCtx->sub_charenc = charenc;//"UTF-16LE";
    printf("pCodecCtx->sub_charenc :%p\n", pCodecCtx->sub_charenc);
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
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
    fprintf(stderr, "end return.\n");
    avcodec_close(pCodecCtx);
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