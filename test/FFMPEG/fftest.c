#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"

char buffer[20];
static const char *ret_str(int v)
{
  memset(buffer, 0, 20);
    switch (v) {
  case AVERROR(EAGAIN): return "AVERROR(EAGAIN)";
    case AVERROR_EOF:     return "-EOF";
    case AVERROR(EIO):    return "-EIO";
    case AVERROR(ENOMEM): return "-ENOMEM";
    case AVERROR(EINVAL): return "-EINVAL";
    default:
        snprintf(buffer, sizeof(buffer), "%2d", v);
        return buffer;
    }
}

int main(int argc, char const *argv[])
{
  avcodec_register_all();
  av_register_all();

  av_log_set_level(AV_LOG_DEBUG);

  AVFormatContext *pFormatCtx = avformat_alloc_context();
  AVCodecContext *pCodecCtx;
  AVCodec *pCodec;
  AVInputFormat *fileFormat;
  int ret = -1;
  int videoStream = -1;

  //"/media/xhms/测试播放文件/flv/4002_G(Flash Video)_V(Baseline@L1.3,LD)_A(LC).flv"
  if ((ret = avformat_open_input(&pFormatCtx, argv[1] == NULL ? "/media/xhms/测试播放文件/avi/2011_G(AVI)_V(AVC,SD)_A(55).avi" : argv[1], NULL, NULL)) != 0)
  {
      fprintf(stderr, "av_open_input_file \n");

      return -1;
  }


  fprintf(stdout, "pFormatCtx %p\n", pFormatCtx);

  if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
  {
      fprintf(stderr, "av_find_stream_info \n");

      return -1;
  }

  av_dump_format(pFormatCtx, 0, argv[1], 0);


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
      fprintf(stderr, "Unsupported videoStream.\n");

      return -1;
  }

  fprintf(stdout, "videoStream: %d\n", videoStream);

  pCodecCtx = pFormatCtx->streams[videoStream]->codec;
  //pCodecCtx->debug = 1;
  pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
  if (pCodec == NULL)
  {
      fprintf(stderr, "Unsupported codec.\n");

      return -1;
  }

  
  //pCodecCtx->sub_charenc = charenc;//"UTF-16LE";
    
  if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
  {
      fprintf(stderr, "Could not open codec.\n");

      return -1;
  }

  ret = avformat_seek_file(pFormatCtx, -1, INT64_MIN, 100000, INT64_MAX, 0);

  fprintf(stdout, "ret=%d\n", ret);

  int index;
    
  AVPacket packet;
  av_init_packet(&packet);
  AVFrame *pFrame = av_frame_alloc();
  int got_picture;
  for (index = 0; index < 10000; index++)
  {
      //memset(&packet, 0, sizeof (packet));
      ret = av_read_frame(pFormatCtx, &packet);
      if (ret < 0)
      {
          fprintf(stderr, "read frame ret:%d\n", ret);

          break ;
      }

      fprintf(stdout, "stream_index:%d\n", packet.stream_index);

      if (packet.stream_index == videoStream)
      {
          ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, &packet);

          fprintf(stdout, "got_picture:%d\n", got_picture);
      }
      av_free_packet(&packet);
  }

  fprintf(stdout, "index=%d\n", index);

  return 0;
}