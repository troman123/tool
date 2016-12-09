#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include <stdio.h>

static void print_fps(double d, const char *postfix, char *outBuf, int outSize)
{
  uint64_t v= lrintf(d*100);
  if (v % 100      )
  {
    snprintf(outBuf, outSize, ", %3.2f %s", d, postfix);
  }
  else if (v%(100*1000))
  {
    av_log(outBuf, outSize, ", %1.0f %s", d, postfix);
  }
  else
  {
    av_log(outBuf, outSize, ", %1.0fk %s", d/1000, postfix);
  }
}

static int get_bit_rate(AVCodecContext *ctx)
{
    int bit_rate;
    int bits_per_sample;

    switch (ctx->codec_type) {
    case AVMEDIA_TYPE_VIDEO:
    case AVMEDIA_TYPE_DATA:
    case AVMEDIA_TYPE_SUBTITLE:
    case AVMEDIA_TYPE_ATTACHMENT:
        bit_rate = ctx->bit_rate;
        break;
    case AVMEDIA_TYPE_AUDIO:
        bits_per_sample = av_get_bits_per_sample(ctx->codec_id);
        bit_rate = bits_per_sample ? ctx->sample_rate * ctx->channels * bits_per_sample : ctx->bit_rate;
        break;
    default:
        bit_rate = 0;
        break;
    }
    return bit_rate;
}

void avcodec_string2(char *buf, int buf_size, AVCodecContext *enc, int encode)
{
  const char *codec_type;
  const char *codec_name;
  const char *profile = NULL;
  const AVCodec *p;
  int bitrate;
  AVRational display_aspect_ratio;

  if (!buf || buf_size <= 0)
  {
    return;
  }

  codec_type = av_get_media_type_string(enc->codec_type);
  codec_name = avcodec_get_name(enc->codec_id);
  if (enc->profile != FF_PROFILE_UNKNOWN)
  {
    if (enc->codec)
    {
      p = enc->codec;
    }
    else
    {
      p = encode ? avcodec_find_encoder(enc->codec_id) :
                    avcodec_find_decoder(enc->codec_id);
    }
    if (p)
    {
      profile = av_get_profile_name(p, enc->profile);
    }
  }

  snprintf(buf, buf_size, "%s: %s", codec_type ? codec_type : "unknown", codec_name);

  buf[0] ^= 'a' ^ 'A'; /* first letter in uppercase */

  if (enc->codec && strcmp(enc->codec->name, codec_name))
  {
    snprintf(buf + strlen(buf), buf_size - strlen(buf), " (%s)", enc->codec->name);
  }

  if (profile)
  {
    snprintf(buf + strlen(buf), buf_size - strlen(buf), " (%s)", profile);
  }

  // if (enc->codec_tag)
  // {
  //     char tag_buf[32];
  //     av_get_codec_tag_string(tag_buf, sizeof(tag_buf), enc->codec_tag);
  //     snprintf(buf + strlen(buf), buf_size - strlen(buf),
  //              " (%s / 0x%04X)", tag_buf, enc->codec_tag);
  // }

  switch (enc->codec_type)
  {
  case AVMEDIA_TYPE_VIDEO:
      if (enc->pix_fmt != AV_PIX_FMT_NONE)
      {
        char detail[256] = "(";
        const char *colorspace_name;

        snprintf(buf + strlen(buf), buf_size - strlen(buf), ", %s",
                 av_get_pix_fmt_name(enc->pix_fmt));

        if (enc->bits_per_raw_sample &&
            enc->bits_per_raw_sample <= av_pix_fmt_desc_get(enc->pix_fmt)->comp[0].depth_minus1)
        {
          av_strlcatf(detail, sizeof(detail), "%d bpc, ", enc->bits_per_raw_sample);
        }
        if (enc->color_range != AVCOL_RANGE_UNSPECIFIED)
        {
          av_strlcatf(detail, sizeof(detail),
                        enc->color_range == AVCOL_RANGE_MPEG ? "tv, ": "pc, ");
        }

        colorspace_name = av_get_colorspace_name(enc->colorspace);
        if (colorspace_name)
        {
          av_strlcatf(detail, sizeof(detail), "%s, ", colorspace_name);
        }

        if (strlen(detail) > 1)
        {
          detail[strlen(detail) - 2] = 0;
          av_strlcatf(buf, buf_size, "%s)", detail);
        }
      }

      if (enc->width)
      {
        snprintf(buf + strlen(buf), buf_size - strlen(buf),
                 ", %dx%d",
                 enc->width, enc->height);

        if (enc->sample_aspect_ratio.num)
        {
          av_reduce(&display_aspect_ratio.num, &display_aspect_ratio.den,
                    enc->width * enc->sample_aspect_ratio.num,
                    enc->height * enc->sample_aspect_ratio.den,
                    1024 * 1024);

          snprintf(buf + strlen(buf), buf_size - strlen(buf),
                   " [SAR %d:%d DAR %d:%d]",
                   enc->sample_aspect_ratio.num, enc->sample_aspect_ratio.den,
                   display_aspect_ratio.num, display_aspect_ratio.den);
        }
        //if (av_log_get_level() >= AV_LOG_DEBUG)
        // {
        //   int g = av_gcd(enc->time_base.num, enc->time_base.den);
        //   snprintf(buf + strlen(buf), buf_size - strlen(buf),
        //            ", %d/%d",
        //            enc->time_base.num / g, enc->time_base.den / g);
        // }
      }
      if (encode)
      {
        snprintf(buf + strlen(buf), buf_size - strlen(buf),
                 ", q=%d-%d", enc->qmin, enc->qmax);
      }

      break;
  case AVMEDIA_TYPE_AUDIO:
      if (enc->sample_rate)
      {
        snprintf(buf + strlen(buf), buf_size - strlen(buf), ", %d Hz", enc->sample_rate);
      }

      av_strlcat(buf, ", ", buf_size);
      av_get_channel_layout_string(buf + strlen(buf), buf_size - strlen(buf),
                                  enc->channels, enc->channel_layout);

      if (enc->sample_fmt != AV_SAMPLE_FMT_NONE)
      {
        snprintf(buf + strlen(buf), buf_size - strlen(buf),
                 ", %s", av_get_sample_fmt_name(enc->sample_fmt));
      }

      break;
  case AVMEDIA_TYPE_DATA:
      if (av_log_get_level() >= AV_LOG_DEBUG)
      {
        int g = av_gcd(enc->time_base.num, enc->time_base.den);
        if (g)
        {
          snprintf(buf + strlen(buf), buf_size - strlen(buf), ", %d/%d",
                  enc->time_base.num / g, enc->time_base.den / g);
        }
      }

      break;
  case AVMEDIA_TYPE_SUBTITLE:
      if (enc->width)
      {
        snprintf(buf + strlen(buf), buf_size - strlen(buf), ", %dx%d", enc->width, enc->height);
      }

      break;
  default:
      return;
  }

  // if (encode)
  // {
  //     if (enc->flags & CODEC_FLAG_PASS1)
  //     {
  //       snprintf(buf + strlen(buf), buf_size - strlen(buf), ", pass 1");
  //     }
  //     if (enc->flags & CODEC_FLAG_PASS2)
  //     {
  //       snprintf(buf + strlen(buf), buf_size - strlen(buf), ", pass 2");
  //     }
  // }

  bitrate = get_bit_rate(enc);
  if (bitrate != 0)
  {
    snprintf(buf + strlen(buf), buf_size - strlen(buf),
            ", %d kb/s",
            bitrate / 1000);
  }
  else if (enc->rc_max_rate > 0)
  {
    snprintf(buf + strlen(buf), buf_size - strlen(buf),
            ", max. %d kb/s",
            enc->rc_max_rate / 1000);
  }
}

void dump_stream_format(AVFormatContext *ic, int i, int index, char *outBuf, int outSize)
{
  if (outBuf == NULL || outSize <= 0)
  {
    return;
  }

  char buf[256] = {0};
  int flags = ic->iformat->flags;
  AVStream *st = ic->streams[i];
  int g = av_gcd(st->time_base.num, st->time_base.den);
  AVDictionaryEntry *lang = av_dict_get(st->metadata, "language", NULL, 0);

  avcodec_string2(buf, sizeof(buf), st->codec, 0);

  //av_log(NULL, AV_LOG_INFO, "    Stream #%d:%d", index, i);
  snprintf(outBuf, outSize, "    Stream #%d:%d", index, i);

  /* the pid is an important information, so we display it */
  /* XXX: add a generic system */
  if (flags & AVFMT_SHOW_IDS)
  {
    //av_log(NULL, AV_LOG_INFO, "[0x%x]", st->id);
    snprintf(outBuf + strlen(outBuf), outSize - strlen(outBuf), "[0x%x]", st->id);
  }

  if (lang)
  {
    //av_log(NULL, AV_LOG_INFO, "(%s)", lang->value);
    snprintf(outBuf + strlen(outBuf), outSize - strlen(outBuf), "(%s)", lang->value);
  }

  //av_log(NULL, AV_LOG_DEBUG,
  //      ", %d, %d/%d",
  //      st->codec_info_nb_frames, st->time_base.num/g, st->time_base.den/g);

  //av_log(NULL, AV_LOG_INFO, ": %s", buf);
  snprintf(outBuf + strlen(outBuf), outSize - strlen(outBuf), ": %s", buf);

  if(st->codec->codec_type == AVMEDIA_TYPE_VIDEO)
  {
    if(st->avg_frame_rate.den && st->avg_frame_rate.num)
    {
      print_fps(av_q2d(st->avg_frame_rate), "fps", outBuf + strlen(outBuf), outSize - strlen(outBuf));
    }
  }

  outBuf = outBuf + strlen(outBuf);
  *outBuf = '\n';
}

char *getAVFormatString(AVFormatContext *formatCtx)
{
  // av_dump_format(pFormatctx, 0, argv[1], 0);
  fprintf(stdout, "iformat:%s\n", formatCtx->iformat->name);
  //fprintf(stdout, "%d\n", pFormatctx->nb_streams);
  //fprintf(stdout, "%d\n", pFormatctx->nb_programs);

  int i;
  for(i = 0; i < formatCtx->nb_streams; i++)
  {
    char buf[512] = {0};
    dump_stream_format(formatCtx, i, 0, buf, sizeof (buf));

    fprintf(stdout, "%s", buf);
  }
}

int main(int argc, char* argv[])
{
  av_register_all();
  avcodec_register_all();
  AVFormatContext *pFormatctx= NULL;
  AVCodecContext *pCodecCtx;

  fprintf(stdout, "%s\n", argv[1]);

  if (avformat_open_input(&pFormatctx, argv[1], NULL, NULL) != 0)
  {
    fprintf(stderr, "avformat_open_input faile\n");

    return -1;
  }
  if (avformat_find_stream_info(pFormatctx, NULL) < 0)
  {
    fprintf(stderr, "avformat_find_stream_info");

    return -1;
  }

  getAVFormatString(pFormatctx);
}
