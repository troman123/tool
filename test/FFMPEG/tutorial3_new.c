#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

#include <string.h>
#include <stdint.h>

typedef uint32_t UINT32;

void saveFrame(const AVFrame *pFrame, int width, int height, int iFrame);
static const char *ret_str(int v);

void callTrack(void)
{
  void* array[10] = {0};
  UINT32 size = 0;
  char **strframe = NULL;
  UINT32 i = 0, j = 0;

  size = backtrace(array, 10);
  strframe = (char **)backtrace_symbols(array, size);

  printf("print call frame now:\n");
  for(i = 0; i < size; i++){
    printf("frame %d -- %s\n", i, strframe[i]);
  }

  if(strframe)
  {
    free(strframe);
    strframe = NULL;
  }
}


int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
    {
        fprintf(stderr, "Could not initialize SDL %s\n", SDL_GetError());
        return -1;
    }

    av_register_all();
    avformat_network_init();


    av_log_set_level(AV_LOG_DEBUG);
  
    AVFormatContext *pFormatCtx = NULL;
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

    fprintf(stdout, "......................\n");
    fprintf(stdout, "%s\n", argv[1]);
    fprintf(stdout, "......................\n");

    if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0)
    {
        fprintf(stderr, "avformat_open_input\n ");

        return -1;
    }

    fprintf(stdout, "......................\n");
    
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        fprintf(stderr, "av_find_stream_info\n ");

        return -1;
    }
    fprintf(stdout, ":............................\n");
    av_dump_format(pFormatCtx, 0, argv[1], 0);

    fprintf(stdout, "..............\n");
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

    fprintf(stdout, "bebin to work, yes?");
    char enter; 
    scanf("%c", &enter);
    getchar();
    if (enter != 'y')
    {
      fprintf(stdout, "you enter is wrong.");

      goto end; 
    }

    for (index = 0; ; index++)
    {
        //memset(&packet, 0, sizeof (packet));
        fprintf(stdout, "index:%i", i);

        ret = av_read_frame(pFormatCtx, &packet);
        if (ret < 0)
        {
            callTrack(); 
            fprintf(stderr, "read frame ret:%d  ret_str:%s\n", ret, ret_str(ret));
            if (ret == AVERROR_EOF)
            {
              break ;
            }
            else
            {
              continue ;
            }
            
        }

        fprintf(stdout, "stream_index:%d packet.pts:%lld, packet.dts:%lld\n", packet.stream_index, packet.pts, packet.dts);

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
            //(AVPicture *)pFrame, pCodecCtx->pix_fmt,
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
