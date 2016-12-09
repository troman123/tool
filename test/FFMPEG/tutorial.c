#include "libavformat/avformat.h"
#include <libavcodec/avcodec.h>
#include "libswscale/swscale.h"
#include <libavutil/time.h>

#define status_t int
#define NO_ERROR 0
#define BAD_VALUE -1
#define THUMBNAIL_WIDTH       256
#define THUMBNAIL_HEIGHT      197
#define THUMBNAIL_PIX_FORMAT  PIX_FMT_YUVJ420P
//#define AVPixelFormat int

void saveFrame(AVFrame *pFrame, int width, int height, int iFrame);
int main(int argc, char* argv[])
{
    av_register_all();

    AVFormatContext *pFormatCtx;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVFrame *pFrame, *pFrameYUV, *pFrameRGB;
    uint8_t *yuvBuffer, *buffer;

    fprintf(stderr, "avgv[]%s\n", argv[1]);

    pFormatCtx = avformat_alloc_context();
    if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0)
    {
        fprintf(stderr, "avformat_open_input fail\n");
        return -1;
    }
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        fprintf(stderr, "avformat_find_stream_info fail\n");
        return -1;
    }
    av_dump_format(pFormatCtx, 0, argv[1], 0);

    int videoStream = -1;
    int i;
    for (i = 0; i < pFormatCtx->nb_streams; ++i)
    {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStream = i;

            break ;
        }
    }

    if (videoStream == -1)
    {
        fprintf(stderr, "main videoStream -1\n");
        return -1;
    }
    pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL)
    {
        fprintf(stderr, "Unsupported codec.\n");

        return -1;
    }

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
    {
        fprintf(stderr, "Could not open codec.\n");

        return -1;
    }

    fprintf(stdout, "open avcodec. name=%s [%s]\n", pCodec->name, avcodec_get_name(pCodec->id));
    pFrame = avcodec_alloc_frame();
    pFrameYUV = avcodec_alloc_frame();
    pFrameRGB = avcodec_alloc_frame();
    if (pFrameRGB == NULL)
    {
        fprintf(stderr, "pFrameRGB fail.\n");
        return -1;
    }

    
    //src ==> yuv ==> rgb
    struct SwsContext *convertCtx = NULL; //src ==> yuv
    struct SwsContext *convertYGCtx = NULL;// yuv ==> rgb

    int yuvCtxWidth =  pCodecCtx->width;
    int yuvCtxHeight = pCodecCtx->height;
    int rgbCtxWidth = pCodecCtx->width;
    int rgbCtxHeight = pCodecCtx->height;

    int yuvSrcSliceY = 0;
    int yuvSrcSliceH = 100;//pCodecCtx->height;

    int yuvPicWidth =  100;
    int yuvPicHeight = 100;
    int rgbPicWidth = pCodecCtx->width;
    int rgbPicHeight = pCodecCtx->height;

    fprintf(stdout, "context width=%d height=%d\n", pCodecCtx->width, pCodecCtx->height);
    int numBytes;

    numBytes = avpicture_get_size(PIX_FMT_YUV420P, yuvPicWidth, yuvPicHeight);
    yuvBuffer = (uint8_t *)av_malloc(numBytes * (sizeof (uint8_t)));
    avpicture_fill((AVPicture *)pFrameYUV, yuvBuffer, PIX_FMT_YUV420P, yuvPicWidth, yuvPicHeight);

    numBytes = avpicture_get_size(PIX_FMT_RGB24, rgbPicWidth, rgbPicHeight);
    buffer = (uint8_t *)av_malloc(numBytes * (sizeof (uint8_t)));
    avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24, rgbPicWidth, rgbPicHeight);

    int frameFinished = 0;
    AVPacket packet;
    int index = 0;

    fprintf(stdout, "sws_getContext           srcSliceY         srcSliceH         [yuv]srcPic       [yuv]srcStride        [rgb]dstPic        [rgb]dstStride\n");
    while (av_read_frame(pFormatCtx, &packet) >= 0)
    {
        if (packet.stream_index == videoStream)
        {
            int ret = avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

            //fprintf(stdout, "avcodec decode video. ret=%d frameFinished=%d\n", ret, frameFinished);
            if (frameFinished)
            {
                // img_convert((AVPicture *)pFrameRGB, PIX_FMT_RGB24,
                //  (AVPicture *)pFrame, pCodecCtx->pix_fmt,
                //  pCodecCtx->width, pCodecCtx->height);
                if (convertCtx == NULL && convertYGCtx == NULL)
                {    

                    // src ==> yuv
                    convertCtx = sws_getContext(
                                        pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                                        yuvPicWidth, yuvPicHeight, PIX_FMT_YUV420P,
                                        2, NULL, NULL, NULL);
                    if (convertCtx == NULL)
                    {
                         fprintf(stderr, "Cannot initialize the conversion context/n");

                         goto end;
                    }

                    // yuv ==> src
                    convertYGCtx = sws_getContext(
                                        yuvCtxWidth, yuvCtxHeight, PIX_FMT_YUV420P,
                                        rgbCtxWidth, rgbCtxHeight, PIX_FMT_RGB24,
                                        2, NULL, NULL, NULL);
                    if (convertYGCtx == NULL)
                    {
                         fprintf(stderr, "Cannot initialize the conversion context/n");

                         goto end;
                    }
                }

                sws_scale(convertCtx,
                            pFrame->data,
                            pFrame->linesize,
                            0,
                            pCodecCtx->height,
                            pFrameYUV->data,
                            pFrameYUV->linesize);


                fprintf(stdout, ".                                                   \n");
                

                fprintf(stdout, "[%dx%d]==>[%dx%d]          %d          %d              [%dx%d]          [%d %d %d]             [%dx%d]             [%d %d %d]\n",
                    yuvCtxWidth, yuvCtxHeight, rgbCtxWidth, rgbCtxHeight,
                    yuvSrcSliceY, yuvSrcSliceH,
                    yuvPicWidth, yuvPicHeight,
                    pFrameYUV->linesize[0], pFrameYUV->linesize[1], pFrameYUV->linesize[2],
                    rgbPicWidth, rgbPicHeight,
                    pFrameRGB->linesize[0], pFrameRGB->linesize[1], pFrameRGB->linesize[2]
                    );
                sws_scale(convertYGCtx,
                            pFrameYUV->data,
                            pFrameYUV->linesize,
                            yuvSrcSliceY,
                            yuvSrcSliceH,
                            pFrameRGB->data,
                            pFrameRGB->linesize);
                // if (++index <= 5)
                // {
                //     //saveFrame(pFrameRGB, dstWidth, dstHeight, index);
                // }
                // else
                // {
                //     break;
                // }
            }
        }
        av_free_packet(&packet);
    }

    sws_freeContext(convertCtx);
    sws_freeContext(convertYGCtx);


end:
    av_free(yuvBuffer);
    av_free(buffer);
    av_free(pFrameRGB);
    av_free(pFrame);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    return 0;
}
//测试结果记录：sws_getContext youkuYY.mp4(960x540)
//   sws_getContext             srcSliceY   srcSliceH  sws_scale srcStride         dstStride                    dstPic
//1> 960x540 ==> 960x540           0          960       [992 496 496]              [2880 0 0]                   960x540
//1> 960x540 ==> 960x540           0          960       [992 496 496]              [1440 0 0]                  [480x320]
void saveFrame(AVFrame *pFrame, int width, int height, int iFrame)
{
    FILE *pFile;
    char szFileName[32];

    sprintf(szFileName, "frame%d.ppm", iFrame);
    pFile = fopen(szFileName, "wb");
    if (pFile == NULL)
    {
        fprintf(stderr, "saveFrame file open fail.\n");
        return ;
    }

    fprintf(pFile, "P6\n%d %d\n255\n", width, height);
    int i;
    for (i = 0; i < height; ++i)
    {
        fwrite(pFrame->data[0] + i * pFrame->linesize[0], 1, width * 3, pFile);
    }

    fclose(pFile);
}

// AVFrame *resizeFrame(AVFrame *frame, AVPixelFormat fmt, int width, int height)
// {
//   fprintf(stdout, "resize frame. %dx%d => %dx%d", frame->width, frame->height, width, height);

//   AVFrame *dst = allocFrame(PIX_FMT_YUV420P, width, height);

//   SwsContext *ctx = sws_getContext(frame->width,
//                                    frame->height,
//                                    fmt,
//                                    width,
//                                    height,
//                                    PIX_FMT_YUV420P,
//                                    SWS_BICUBIC,
//                                    NULL,
//                                    NULL,
//                                    NULL);
//   if (ctx != NULL)
//   {
//     sws_scale(ctx, frame->data, frame->linesize, 0, frame->height, dst->data, dst->linesize);
//     sws_freeContext(ctx);
//   }
//   else
//   {
//     fprintf(stdout, "get sws context failed.");

//     avcodec_free_frame(&dst);
//   }

//   return dst;
// }

// AVFrame *centerCropFrame(AVCodecContext *ctx, AVFrame *frame, AVPixelFormat fmt, int width, int height)
// {
//   fprintf(stdout, "center crop frame. %dx%d => %dx%d width=%d height=%d", frame->width, frame->height, width, height, ctx->width, ctx->height);

//   AVFrame *dst = NULL;
//   // if (frame->width == 0)
//   // {
//   //   frame->width = ctx->width;
//   // }
//   // if (frame->height == 0)
//   // {
//   //   frame->height = ctx->height;
//   // }
//   double scale_x = (double) frame->width / (width * 4 / 3);
//   double scale_y = (double) frame->height / (height * 4 / 3);
//   double scale = FFMIN(scale_x, scale_y);
//   int w = frame->width / scale;
//   int h = frame->height / scale;
//   frame = resizeFrame(frame, fmt, w, h);
//   if (frame != NULL)
//   {
//     dst = allocFrame(PIX_FMT_YUV420P, width, height);
//     av_picture_crop((AVPicture *) dst, (AVPicture *) frame, PIX_FMT_YUV420P, (h - height) / 2, (w - width) / 2);
//     avcodec_free_frame(&frame);
//   }

//   return dst;
// }

// status_t writeJPEG(AVCodecContext *ctx, AVFrame *frame, const char *path, int width, int height)
// {
//   fprintf(stdout, "write jpeg to %s", path);

//   status_t status = BAD_VALUE;

//   AVFrame *dst = centerCropFrame(ctx, frame, ctx->pix_fmt, width, height);
//   if (dst != NULL)
//   {
//     AVCodecContext *output_ctx = avcodec_alloc_context3(NULL);
//     output_ctx->bit_rate      = ctx->bit_rate;
//     output_ctx->width         = dst->width;
//     output_ctx->height        = dst->height;
//     output_ctx->pix_fmt       = THUMBNAIL_PIX_FORMAT;
//     output_ctx->codec_id      = AV_CODEC_ID_MJPEG;
//     output_ctx->codec_type    = AVMEDIA_TYPE_VIDEO;
//     output_ctx->time_base.num = ctx->time_base.num;
//     output_ctx->time_base.den = ctx->time_base.den;

//     if (output_ctx->bit_rate == 0)
//     {
//       output_ctx->bit_rate = 64000;
//     }
//     if (output_ctx->time_base.num == 0)
//     {
//       output_ctx->time_base.den = 5;
//       output_ctx->time_base.num = 1;
//     }
//     if (output_ctx->bit_rate_tolerance == 0
//         || output_ctx->bit_rate_tolerance < output_ctx->bit_rate * av_q2d(output_ctx->time_base))
//     {
//        output_ctx->bit_rate_tolerance = FFMAX(output_ctx->bit_rate / 4, (int64_t)output_ctx->bit_rate * av_q2d(output_ctx->time_base));
//     }

//     fprintf(stdout, "bit_rate_tolerance=%d, bit_rate=%d, width=%d, height=%d, num=%d, den=%d codec_id=%d",
//      output_ctx->bit_rate_tolerance, output_ctx->bit_rate, dst->width, dst->height, output_ctx->time_base.num, output_ctx->time_base.den, output_ctx->codec_id);

//     AVCodec *output_codec = avcodec_find_encoder(output_ctx->codec_id);
//     if (output_codec != NULL)
//     {
//       int ret;
//       if ((ret = avcodec_open2(output_ctx, output_codec, NULL)) >= 0)
//       {
//         output_ctx->mb_lmin = output_ctx->lmin = output_ctx->qmin * FF_QP2LAMBDA;
//         output_ctx->mb_lmax = output_ctx->lmax = output_ctx->qmax * FF_QP2LAMBDA;
//         output_ctx->flags = CODEC_FLAG_QSCALE;
//         output_ctx->global_quality = output_ctx->qmin * FF_QP2LAMBDA;

//         dst->pts = 1;
//         dst->quality = output_ctx->global_quality;

//         AVPacket pkt;
//         av_init_packet(&pkt);
//         pkt.data = NULL;
//         pkt.size = 0;
//         int got_packet = 0;
//         if (avcodec_encode_video2(output_ctx, &pkt, dst, &got_packet) == 0 && got_packet)
//         {
//           FILE *fp = fopen(path, "wb");
//           if (fp != NULL)
//           {
//             fwrite(pkt.data, pkt.size, 1, fp);
//             fclose(fp);

//             status = NO_ERROR;
//           }
//           else
//           {
//             fprintf(stdout, "create file failed.");
//           }

//           av_free_packet(&pkt);
//         }
//         avcodec_close(output_ctx);
//       }
//       else
//       {
//         fprintf(stdout, "write jpeg. open codec failed.ret=%d", ret);
//       }
//     }
//     else
//     {
//       fprintf(stdout, "can't find mjpeg encoder.");
//     }

//     avcodec_free_frame(&dst);
//   }

//   return status;
// }