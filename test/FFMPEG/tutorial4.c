#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

typedef struct PacketQueue
{
    AVPacketList *firstPkt, *lastPkt;
    int nbPackets;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;

PacketQueue audioQ;
#define SDL_AUDIO_BUFFER_SIZE 1024

void saveFrame(const AVFrame *pFrame, int width, int height, int iFrame);
static const char *ret_str(int v);
void packetQueueInit(PacketQueue* q);
int packetQueuePut(PacketQueue *q, AVPacket *pkt);
int packetQueueGet(PacketQueue *q, AVPacket *pkt, int block);
void audioCallBack(void *userdata, Uint8 *stream, int len);
int audioDecodeFrame(AVCodecContext *pACodecCtx, uint8_t *audioBuf, int bufSize);

int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
    {
        fprintf(stderr, "Could not initialize SDL %s\n", SDL_GetError());
        return -1;
    }

    av_register_all();

    AVFormatContext *pFormatCtx;
    AVCodecContext *pVCodecCtx, *pACodecCtx;
    AVCodec *pVCodec, *pACodec;
    AVFrame *pFrame, *pFrameRGB;
    AVPicture pict;
    SDL_Rect rect;
    struct SwsContext *imgConvertCtx;
    SDL_Surface *screen;
    SDL_Overlay *bmp;
    SDL_AudioSpec wantedSpec, spec;
    uint8_t *buffer;
    int videoStream = -1;
    int audioStream = -1;
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
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO 
            && videoStream < 0)
        {
            videoStream = i;
        }
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO
            && audioStream < 0)
        {
            audioStream = i;
        }
    }

    if (videoStream == -1)
    {
        return -1;
    }

    if (audioStream == -1)
    {
        return -1;
    }

    fprintf(stdout, "videoStream: %d\n", videoStream);

    pVCodecCtx = pFormatCtx->streams[videoStream]->codec;
    pVCodec = avcodec_find_decoder(pVCodecCtx->codec_id);
    if (pVCodec == NULL)
    {
        fprintf(stderr, "Unsupported video codec.\n");

        return -1;
    }
    if (avcodec_open(pVCodecCtx, pVCodec) < 0)
    {
        fprintf(stderr, "Could not open video codec.\n");

        return -1;
    }
    pACodecCtx = pFormatCtx->streams[audioStream]->codec;
    pACodec = avcodec_find_decoder(pACodecCtx->codec_id);
    if (pACodec == NULL)
    {
        fprintf(stderr, "Unsupported audio codec.\n");

        return -1;
    }
    if (avcodec_open(pACodecCtx, pACodec) < 0)
    {
        fprintf(stderr, "Could not open audio codec.\n");

        return -1;
    }

    pFrame = avcodec_alloc_frame();
    pFrameRGB = avcodec_alloc_frame();
    if (pFrameRGB == NULL)
    {
        fprintf(stderr, "pFrameRGB == NULL\n");

        return -1;
    }

    numBytes = avpicture_get_size(PIX_FMT_RGB24, pVCodecCtx->width, pVCodecCtx->height);
    buffer = (uint8_t *)av_malloc(numBytes * (sizeof (uint8_t)));
    avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24, pVCodecCtx->width,
        pVCodecCtx->height);

    imgConvertCtx = sws_getContext(pVCodecCtx->width, pVCodecCtx->height,
      pVCodecCtx->pix_fmt, pVCodecCtx->width, pVCodecCtx->height,
      PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    if (imgConvertCtx == NULL)
    {
         fprintf(stderr, "Cannot initialize the conversion context\n");

         goto end;
    }

    screen = SDL_SetVideoMode(pVCodecCtx->width, pVCodecCtx->height, 0, 0);
    if (!screen)
    {
        fprintf(stderr, "Could not set SDL video mode , exiting\n");

        goto end;
    }

    wantedSpec.freq = pACodecCtx->sample_rate;
    wantedSpec.format = AUDIO_S16SYS;
    wantedSpec.channels = pACodecCtx->channels;
    wantedSpec.silence = 0;
    wantedSpec.samples = SDL_AUDIO_BUFFER_SIZE;
    wantedSpec.callback = audioCallBack;
    wantedSpec.userdata = pACodecCtx;

    if (SDL_OpenAudio(&wantedSpec, &spec) < 0)
    {
        fprintf(stderr, "SDL_OpenAudio : %s\n", SDL_GetError());
    }

    bmp = SDL_CreateYUVOverlay(pVCodecCtx->width, pVCodecCtx->height, SDL_YV12_OVERLAY, screen);
    pict.data[0] = bmp->pixels[0];
    pict.data[1] = bmp->pixels[1];
    pict.data[2] = bmp->pixels[2];
    pict.linesize[0] = bmp->pitches[0];
    pict.linesize[1] = bmp->pitches[1];
    pict.linesize[2] = bmp->pitches[2];
    rect.x = 0;
    rect.y = 0;
    rect.w = pVCodecCtx->width;
    rect.h = pVCodecCtx->height;

    int index;
    int ret = 0;
    int frameFinished;
    AVPacket packet;

    av_init_packet(&packet);
    packetQueueInit(&audioQ);
    SDL_PauseAudio(0);
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
            ret = avcodec_decode_video2(pVCodecCtx, pFrame, &frameFinished, &packet);
            if (ret < 0)
            {
                fprintf(stderr, "decode ret:%s\n", ret_str(ret));

                continue ;
            }
            if (frameFinished)
            {
                // img_convert((AVPicture *)pFrameRGB, PIX_FMT_RGB24,
                //  (AVPicture *)pFrame, pVCodecCtx->pix_fmt,
                //  pVCodecCtx->width, pVCodecCtx->height);
                SDL_LockYUVOverlay(bmp);
                sws_scale(imgConvertCtx, pFrame->data, pFrame->linesize, 0,
                    pVCodecCtx->height, pict.data, pict.linesize);
                SDL_UnlockYUVOverlay(bmp);
                SDL_DisplayYUVOverlay(bmp, &rect);
            }
            av_free_packet(&packet);
        }
        else if(packet.stream_index == audioStream)
        {
            packetQueuePut(&audioQ, &packet);
        }
        else
        {
            av_free_packet(&packet);
        }
    }

end:
    sws_freeContext(imgConvertCtx);
    av_free(buffer);
    av_free(pFrameRGB);
    av_free(pFrame);
    avcodec_close(pVCodecCtx);
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
    fprintf(stdout, "pFrame:%0X, szFileName:%s \n", (int)pFrame, szFileName);

    fprintf(pFile, "P6\n%d %d\n255\n", width, height);
    int i;
    for (i = 0; i < height; ++i)
    {
        fwrite(pFrame->data[0] + i * pFrame->linesize[0], 1, width * 3, pFile);
    }

    fclose(pFile);
}

void packetQueueInit(PacketQueue* q)
{
    memset(q, 0, sizeof (PacketQueue));
    q->mutex = SDL_CreateMutex();
    q->cond = SDL_CreateCond();
}

int packetQueuePut(PacketQueue *q, AVPacket *pkt)
{
    AVPacketList *tmpPkts;
    if (av_dup_packet(pkt) < 0)
    {
        return -1;
    }
    tmpPkts = av_malloc(sizeof (AVPacketList));
    if (!tmpPkts)
    {
        return -1;
    }
    tmpPkts->pkt = *pkt;
    tmpPkts->next = NULL;

    SDL_LockMutex(q->mutex);
    if (!(q->lastPkt))
    {
        q->firstPkt = tmpPkts;
    }
    else
    {
        q->lastPkt->next = tmpPkts;
    }
    q->lastPkt = tmpPkts;
    q->nbPackets++;
    q->size += tmpPkts->pkt.size;
    SDL_CondSignal(q->cond);

    SDL_UnlockMutex(q->mutex);
    return 0;
}

int packetQueueGet(PacketQueue *q, AVPacket *pkt, int block)
{
    AVPacketList *tmpPkts;
    int ret;
    SDL_LockMutex(q->mutex);

    for (;;)
    {
        tmpPkts = q->firstPkt;
        if (tmpPkts)
        {
            q->firstPkt = tmpPkts->next;
            if (!q->firstPkt)
            {
                q->lastPkt = NULL;
            }
            *pkt = tmpPkts->pkt;
            q->nbPackets--;
            q->size -= pkt->size;
            av_free(tmpPkts);
            ret = 1;

            break ;
        }
        else if (!block)
        {
            ret = 0;

            break;
        }
        else
        {
            SDL_CondWait(q->cond, q->mutex);
        }

    }
    SDL_UnlockMutex(q->mutex);

    return ret;
}

void audioCallBack(void *userdata, Uint8 *stream, int len)
{
    fprintf(stdout, "audioCallBack.\n");

    AVCodecContext *pACodecCtx = (AVCodecContext *)userdata;
    int len1, audioSize;
    static uint8_t audioBuf[AVCODEC_MAX_AUDIO_FRAME_SIZE * 3 / 2];
    static unsigned int audioBufSize = 0;
    static unsigned int audioBufIndex = 0;

    while (len > 0) 
    {
        if (audioBufIndex >= audioBufSize)
        {
            audioSize = audioDecodeFrame(pACodecCtx, audioBuf, sizeof (audioBuf));

            if (audioSize < 0)
            {
                audioBufSize = 1024;
                memset(audioBuf, 0, audioBufSize);
            }
            else
            {
                audioBufSize = audioSize;
            }
            audioBufIndex = 0;
        }

        len1 = audioBufSize - audioBufIndex;

        if (len1 > len)
        {
            len1 = len;
        }

        memcpy(stream, (uint8_t *)audioBuf + audioBufIndex, len1);
        len -= len1;
        stream += len1;
        audioBufIndex += len1;
    }
}

int audioDecodeFrame(AVCodecContext *pACodecCtx, uint8_t *audioBuf, int bufSize)
{
    static AVPacket pkt = {0};
    static int audioPktSize = 0;
    static uint8_t *audioPktData = NULL;

    int len1, dataSize;

    for (;;)
    {
        while (audioPktSize > 0)
        {
            dataSize = bufSize;

            AVPacket tmpPkt;

            av_init_packet(&tmpPkt);
            tmpPkt.data = audioPktData;
            tmpPkt.size = audioPktSize;
            len1 = avcodec_decode_audio3(pACodecCtx, (int16_t *)audioBuf,
             &dataSize, &tmpPkt);

            if (len1 < 0)
            {
                audioPktSize = 0;

                break ;
            }
            if (dataSize <= 0)
            {
                continue ;
            }

            audioPktSize -= len1;
            audioPktData += len1;

            return dataSize;
        }
        if (pkt.data)
        {
            av_free_packet(&pkt);
        }
        if (packetQueueGet(&audioQ, &pkt, 1) < 0)
        {
            return -1;
        }

        audioPktSize = pkt.size;
        audioPktData = pkt.data;
    }
}