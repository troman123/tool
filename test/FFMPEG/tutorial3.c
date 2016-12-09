#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

void saveFrame(const AVFrame *pFrame, int width, int height, int iFrame);
static const char *ret_str(int v);

int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
    {
        fprintf(stderr, "Could not initialize SDL %s\n", SDL_GetError());
        return -1;
    }

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
        fprintf(stderr, "av_open_input_file ");

        return -1;
    }
    if (av_find_stream_info(pFormatCtx) < 0)
    {
        fprintf(stderr, "av_find_stream_info ");

        return -1;
    }
    dump_format(pFormatCtx, 0, argv[1], 0);

    fprintf(stdout, "pFormatCtx->nb_streams: %d\n", pFormatCtx->nb_streams);

    int i;
    for (i = 0; i < pFormatCtx->nb_streams; ++i)
    {
        fprintf(stdout, "pFormatCtx->nb_streams index: %d\n", i);

        enum AVMediaType codec_type = pFormatCtx->streams[i]->codec->codec_type;
        switch(codec_type)
        {
            case AVMEDIA_TYPE_VIDEO:
                fprintf(stdout, "AVMEDIA_TYPE_VIDEO.\n");

                break ;
            case AVMEDIA_TYPE_AUDIO:
                fprintf(stdout, "AVMEDIA_TYPE_AUDIO.\n");

                break ;
            case AVMEDIA_TYPE_DATA:
                fprintf(stdout, "AVMEDIA_TYPE_DATA.\n");

                break ;
            case AVMEDIA_TYPE_SUBTITLE:
                fprintf(stdout, "AVMEDIA_TYPE_SUBTITLE\n");

                break ;
            default:
                fprintf(stdout, "AVMEDIA_TYPE_UNKNOWN.\n");

                break ;
        }

        if (codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStream = i;

            //break ;
        }
    }

    if (videoStream == -1)
    {
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

    pFrame = avcodec_alloc_frame();
    pFrameRGB = avcodec_alloc_frame();
    if (pFrameRGB == NULL)
    {
        fprintf(stderr, "pFrameRGB == NULL\n");

        return -1;
    }

    numBytes = avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
    buffer = (uint8_t *)av_malloc(numBytes * (sizeof (uint8_t)));
    avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24, pCodecCtx->width,
        pCodecCtx->height);

    int frameFinished;



    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
      pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
      PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    if (img_convert_ctx == NULL)
    {
         fprintf(stderr, "Cannot initialize the conversion context/n");

         goto end;
    }

    screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 0, 0);
    if (!screen)
    {
        fprintf(stderr, "Could not set SDL video mode , exiting");

        goto end;
    }

    bmp = SDL_CreateYUVOverlay(pCodecCtx->width, pCodecCtx->height, SDL_YV12_OVERLAY, screen);
    pict.data[0] = bmp->pixels[0];
    pict.data[1] = bmp->pixels[1];
    pict.data[2] = bmp->pixels[2];
    pict.linesize[0] = bmp->pitches[0];
    pict.linesize[1] = bmp->pitches[1];
    pict.linesize[2] = bmp->pitches[2];
    rect.x = 0;
    rect.y = 0;
    rect.w = pCodecCtx->width;
    rect.h = pCodecCtx->height;

    int index;
    int ret = 0;
    AVPacket packet;
    av_init_packet(&packet);

    for (index = 0; ; index++)
    {
        //memset(&packet, 0, sizeof (packet));
        ret = av_read_frame(pFormatCtx, &packet);
        if (ret < 0)
        {
            fprintf(stderr, "read frame ret:%s\n", ret_str(ret));

            break ;
        }

        //fprintf(stdout, "stream_index:%d\n", packet.stream_index);

        if (packet.stream_index == videoStream)
        {
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
            if (ret < 0)
            {
                fprintf(stderr, "decode ret:%s\n", ret_str(ret));

                continue ;
            }

            //fprintf(stdout, "frameFinished %d\n", frameFinished);

            if (frameFinished)
            {
                // img_convert((AVPicture *)pFrameRGB, PIX_FMT_RGB24,
                //  (AVPicture *)pFrame, pCodecCtx->pix_fmt,
                //  pCodecCtx->width, pCodecCtx->height);
                SDL_LockYUVOverlay(bmp);
                sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0,
                    pCodecCtx->height, pict.data, pict.linesize);
                SDL_UnlockYUVOverlay(bmp);
                SDL_DisplayYUVOverlay(bmp, &rect);
            }
        }
        av_free_packet(&packet);
    }

end:
    sws_freeContext(img_convert_ctx);
    av_free(buffer);
    av_free(pFrameRGB);
    av_free(pFrame);
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