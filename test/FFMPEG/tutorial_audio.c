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

    int audioStream = -1;
    int i;
    for (i = 0; i < pFormatCtx->nb_streams; ++i)
    {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            audioStream = i;

            break ;
        }
    }

    if (audioStream == -1)
    {
        fprintf(stderr, "main audioStream -1\n");
        return -1;
    }
    pCodecCtx = pFormatCtx->streams[audioStream]->codec;
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
    
    AVPacket packet;
    AVFrame *frame = av_frame_alloc();
    int got_frame = 0;
    while (av_read_frame(pFormatCtx, &packet) >= 0)
    {
        if (packet.streams_index == audioStream)
        {
            int ret = avcodec_decode_audio4(pCodecCtx, frame, &got_frame, &packet);
            if (ret > 0)
            {

            }
        }
    }

end:
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