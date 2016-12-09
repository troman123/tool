#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

void saveFrame(const AVFrame *pFrame, int width, int height, int iFrame);
static const char *ret_str(int v);

int main(int argc, char* argv[])
{
    av_register_all();

    AVFormatContext *pFormatCtx;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVFrame *pFrame, *pFrameRGB;
    struct SwsContext *img_convert_ctx;
    uint8_t *buffer;

    fprintf(stdout, "%s\n", argv[1]);

    pFormatCtx = avformat_alloc_context();

    fprintf(stdput, "......................");

    if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0)
    {
        fprintf(stderr, "avformat_open_input ");

        return -1;
    }
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        fprintf(stderr, "av_find_stream_info ");

        return -1;
    }
    fprintf(stdput, "......................");

    av_dump_format(pFormatCtx, 0, argv[1], 0);

    int videoStream = -1;
    // int i;
    // for (i = 0; i < pFormatCtx->nb_streams; ++i)
    // {
    //     if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
    //     {
    //         videoStream = i;

    //         break ;
    //     }
    // }
    videoStream = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &pCodec, 0);

    if (videoStream == -1)
    {
        fprintf(stderr, "videoStream < 0 ");

        return -1;
    }

    fprintf(stdout, "videoStream: %d\n", videoStream);

    pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    // pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    // if (pCodec == NULL)
    // {
    //     fprintf(stderr, "Unsupported codec.\n");

    //     return -1;
    // }
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
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

    int numBytes;
    numBytes = avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
    buffer = (uint8_t *)av_malloc(numBytes * (sizeof (uint8_t)));
    avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24, pCodecCtx->width,
        pCodecCtx->height);

    int frameFinished;
    int index;

    AVPacket packet;
    int ret = 0;

    av_init_packet(&packet);
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
      pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
      PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
    if (img_convert_ctx == NULL)
    {
         fprintf(stderr, "Cannot initialize the conversion context/n");

         goto end;
    }
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

            fprintf(stdout, "frameFinished %d\n", frameFinished);

            if (frameFinished)
            {
                // img_convert((AVPicture *)pFrameRGB, PIX_FMT_RGB24,
                //  (AVPicture *)pFrame, pCodecCtx->pix_fmt,
                //  pCodecCtx->width, pCodecCtx->height);

                sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0,
                    pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
                if (index % 100 == 0)
                {
                    saveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, index);
                    if (index > 10000)
                    {
                        break ;
                    }
                }
            }
        }
        av_free_packet(&packet);//注中途退出的没有释放
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
