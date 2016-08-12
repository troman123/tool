#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000

#define MAX_SAMPLE_RATE     48000
#define DEFAULT_SAMPLE_RATE 44100
#define CAST_SAMPLE_RATE(x) (((x) > MAX_SAMPLE_RATE) ? DEFAULT_SAMPLE_RATE : (x))
#define CAST_CHANNELS(x)    (((x) >= 2) ? 2 : 1)

DECLARE_ALIGNED(16, uint8_t, mTargetAudioBuffer)[AVCODEC_MAX_AUDIO_FRAME_SIZE * 4];

typedef struct AudioPkt {
  int size;
  uint8_t *data;
} AudioPkt;

int audioDecoder(char *url)
{
  av_register_all();
  AVFormatContext *mFormatContext;
  mFormatContext = avformat_alloc_context();
  if (avformat_open_input(&mFormatContext, url, NULL, NULL) != 0)
  {
    fprintf(stderr, "avformat open input fail.\n");

    return -1;
  }

  int mTrackCount = 0;
  AVStream *mAudioStream;
  int mAudioStreamIndex = -1;
  if (avformat_find_stream_info(mFormatContext, NULL) >= 0)
  {
    mTrackCount = mFormatContext->nb_streams;
    if (mTrackCount > 0)
    {
      int i = 0;
      for (i = 0; i < mTrackCount; ++i)
      {
        if (mFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
          mAudioStream = mFormatContext->streams[i];
          mAudioStreamIndex = i;
          break;
        }
      }
    }
    else
    {
      //WARN("can't find any stream.");
    }
  }

  SwrContext *mSwrContext;
  AVFrame *mFrame;
  int mSampleBytes;
  double mAudioSampleRate;
  double mAudioTimeBase;

  mFrame = avcodec_alloc_frame();

  AVCodecContext *mCodecContext = mAudioStream->codec;
  AVCodec *codec = avcodec_find_decoder(mCodecContext->codec_id);
  if (codec != NULL && avcodec_open2(mCodecContext, codec, NULL) != 0)
  {
    fprintf(stderr, "avcodec open fail.\n");
  }

  uint64_t outChannelLayout = mAudioStream->codec->channels == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
  mSampleBytes = av_get_channel_layout_nb_channels(outChannelLayout) * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);

  int channels = mCodecContext->channels;
  uint64_t channelLayout = mCodecContext->channel_layout;
  if (channelLayout == 0 || av_get_channel_layout_nb_channels(channelLayout) != channels)
  {
    channelLayout = av_get_default_channel_layout(channels);
  }

  mSwrContext = swr_alloc_set_opts(NULL,
                                   outChannelLayout,
                                   AV_SAMPLE_FMT_S16,
                                   CAST_SAMPLE_RATE(mCodecContext->sample_rate),
                                   channelLayout,
                                   mCodecContext->sample_fmt,
                                   mCodecContext->sample_rate,
                                   0,
                                   NULL);
  if (mSwrContext != NULL)
  {
    if (swr_init(mSwrContext) >= 0)
    {
      mAudioSampleRate = 1.0 / mAudioStream->codec->sample_rate;
      mAudioTimeBase = av_q2d(mAudioStream->time_base);
    }

    AVPacket avpacket;
    av_init_packet( &avpacket );
    AVPacket *pkt = &avpacket;

    uint8_t audioBuf[4096] = {0};
    int bufIndex = 0;

    int copyCount = 0;
    while (av_read_frame(mFormatContext, pkt) >= 0)
    {
      fprintf(stdout, "av_read_frame avpacket AVBufferRef *buf=%p pts=%lld dts=%lld size=%d flags=%d, side_data=%p side_data_elems=%d pos=%lld convergence_duration=%lld index=%d mAudioStreamIndex=%d\n",
       pkt->buf, pkt->pts, pkt->dts, pkt->size, pkt->flags, pkt->side_data, pkt->side_data_elems, pkt->pos, pkt->convergence_duration, pkt->stream_index, mAudioStreamIndex);

      if (pkt->stream_index == mAudioStreamIndex)
      {
        if (copyCount < 2)
        {
          memcpy(audioBuf + bufIndex, pkt->data, pkt->size);
          bufIndex += pkt->size;
          copyCount++;
        }
        else
        {
            decodeAudioPkt(mCodecContext, mSwrContext, mFrame, mAudioStreamIndex, mSampleBytes, audioBuf, bufIndex);
            bufIndex = 0;
            copyCount = 0;
        }
      }
    }

      av_free_packet(pkt);
  }


  swr_free(&mSwrContext);
  av_free(mFrame);
  avcodec_close(mCodecContext);
  avformat_close_input(&mFormatContext);

  return 0;
}

void decodeAudioPkt(AVCodecContext *mCodecContext, SwrContext *mSwrContext, AVFrame *mFrame, int mAudioStreamIndex, int mSampleBytes, uint8_t *buf, int bufSize)
{
  AVPacket avpacket;
  av_init_packet( &avpacket );

  avpacket.size = bufSize;
  avpacket.data = buf;

  int gotFrame = 0;
  dumpMem(avpacket.data, avpacket.size);
  avcodec_get_frame_defaults(mFrame);
  AVPacket tmpPkt = avpacket;
  int size;
  while (tmpPkt.size > 0)
  {
    size = avcodec_decode_audio4(mCodecContext, mFrame, &gotFrame, &tmpPkt);

    fprintf(stdout, "avcodec decode audio size=%d gotFrame=%d\n", size, gotFrame);

    if (size > 0)
    {
      tmpPkt.size -= size;
      tmpPkt.data += size;

      if (gotFrame)
      {
        int pts = av_frame_get_best_effort_timestamp(mFrame);
        if (pts != -1)
        {
          fprintf(stdout, "frame completed. pts=%d", pts);

          uint8_t *outBuffer[] = { (uint8_t *) mTargetAudioBuffer };
          int outCount = sizeof (mTargetAudioBuffer) / mSampleBytes;

          const uint8_t **inBuffer = (const uint8_t **) mFrame->extended_data;
          int inCount = mFrame->nb_samples;

          int convRet = swr_convert(mSwrContext, outBuffer, outCount, inBuffer, inCount);

          fprintf(stdout, "swr_convert outCount=%d inCount=%d convRet=%d \n", outCount, inCount, convRet);
          //render(mTargetAudioBuffer, convRet * mSampleBytes, pts);
        }
        else
        {
          //WARN("invalid pts. ignore it.");
        }
      }
    }
    else
    {
      //WARN("packet error. skip it.");

      /* if error, we skip the frame */
      tmpPkt.size = 0;
    }
  }
}

void dumpMem(uint8_t *ptr, int size)
{
  char buf[4097] = {0};
  int printLen = 4096 > size * 2 ? size * 2 : 4096;
  int index = 0;
  char *tmp = buf;
  while (index < printLen)
  {
    sprintf(tmp + index, "%02x", *(ptr++));
    index += 2;
  }

  fprintf(stdout, "dumpMem %s\n", buf);
}

int main(int argc, char const *argv[])
{
  /* code */
  audioDecoder("/mnt/hgfs/D/test/video/youkuYY.mp4");
  return 0;
}