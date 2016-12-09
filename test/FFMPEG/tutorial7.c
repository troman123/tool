#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

#define AV_SYNC_THRESHOLD 0.01
#define AV_NOSYNC_THRESHOLD 10.0
#define SDL_AUDIO_BUFFER_SIZE 1024
#define MAX_AUDIOQ_SIZE (10 * 16 * 1024)
#define MAX_VIDEOQ_SIZE (10 * 256 * 1024)
#define FF_ALLOC_EVENT (SDL_USEREVENT)
#define FF_REFRESH_EVENT (SDL_USEREVENT + 1)
#define FF_QUIT_EVENT (SDL_USEREVENT + 2)
#define VIDEO_PICTURE_QUEUE_SIZE 1
#define AUDIO_DIFF_AVG_NB 20
#define SAMPLE_CORRECTION_PERCENT_MAX 10.0
#define DEFAULT_AV_SYNC_TYPE AV_SYNC_VIDEO_MASTER

typedef struct PacketQueue
{
    AVPacketList *firstPkt, *lastPkt;
    int nbPackets;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;

typedef struct VideoPicture
{
    SDL_Overlay *bmp;
    int width, height;
    int allocated;
    double pts;
} VideoPicture;

typedef struct VideoState
{
    AVFormatContext *pFormatCtx;
    int videoStream, audioStream;
    int avSyncType;

    AVStream *audioSt;
    PacketQueue audioQ;
    DECLARE_ALIGNED(16, uint8_t, audioBuf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2]);
    unsigned int audioBufSize;
    unsigned int audioBufIndex;
    AVPacket audioPkt;
    uint8_t *audioPktData;
    int audioPktSize;
    double audioClock;
    double audioDiffCum;
    double audioDiffAvgCoef;
    int audioDiffAvgCount;
    double audioDiffThreshold;

    struct SwsContext *imgConvertCtx;
    AVStream *videoSt;
    PacketQueue videoQ;
    VideoPicture pictQ[VIDEO_PICTURE_QUEUE_SIZE];
    int pictqSize, pictqRindex, pictqWindex;
    SDL_mutex *pictqMutex;
    SDL_cond *pictqCond;
    SDL_Thread *videoTid;
    SDL_Thread *parseTid;
    double videoClock;
    double frameTimer;
    double frameLastPts;
    double frameLastDelay;
    double videoCurrentPts;
    ///<current displayed pts (different from video_clock if frame fifos are used)
    int64_t videoCurrentPtsTime;
    ///<time (av_gettime) at which we updated video_current_pts - used to have running video pts

    char fileName[1024];
    int quit;
} VideoState;

enum
{
    AV_SYNC_AUDIO_MASTER,
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_MASTER,
};

SDL_Surface *screen;
VideoState *gLobalVS;
uint64_t gLocalVideoPktPts = AV_NOPTS_VALUE;

static const char *retStr(int v);
static void scheduleRefresh(VideoState *is, int delay);
static Uint32 sdlRefreshTimerCb(Uint32 interval, void *opaque);

void allocPicture(void *userdata);
void audioCallBack(void *userdata, Uint8 *stream, int len);
int audioDecodeFrame(VideoState *is, uint8_t *audioBuf, int bufSize, double *ptsPtr);
int decodeThread(void *arg);
double getAudioClock(VideoState *is);
int ourGetBuffer(struct AVCodecContext *c, AVFrame *pic);
void ourReleaseBuffer(struct AVCodecContext *c, AVFrame *pic);
void packetQueueInit(PacketQueue* q);
int packetQueuePut(PacketQueue *q, AVPacket *pkt);
int packetQueueGet(PacketQueue *q, AVPacket *pkt, int block);
int queuePicture(VideoState *is, AVFrame *pFrame, double pts);
int streamComponentOpen(VideoState *is, int streamIndex);
double synchronizeVideo(VideoState *is, AVFrame *srcFrame, double pts);
void videoDisplay(VideoState *is);
void videoRefreshTimer(void *userdata);
int videoThread(void *arg);

static const char *retStr(int v)
{
    char buffer[20];
    switch (v) {
    case AVERROR_EOF:     return "-EOF";
    case AVERROR(EIO):    return "-EIO";
    case AVERROR(ENOMEM): return "-ENOMEM";
    case AVERROR(EINVAL): return "-EINVAL";
    default:
        snprintf(buffer, sizeof (buffer), "%2d", v);
        return buffer;
    }
}

static void scheduleRefresh(VideoState *is, int delay)
{
    SDL_AddTimer(delay, sdlRefreshTimerCb, is);
}

static Uint32 sdlRefreshTimerCb(Uint32 interval, void *opaque)
{
    SDL_Event event;
    event.type = FF_REFRESH_EVENT;
    event.user.data1 = opaque;
    SDL_PushEvent(&event);

    return 0;
}

int main(int argc, char* argv[])
{
    av_register_all();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
    {
        fprintf(stderr, "Could not initialize SDL %s\n", SDL_GetError());
        return -1;
    }

#ifndef __DARWIN__
    screen = SDL_SetVideoMode(1270, 468, 32, SDL_SWSURFACE|SDL_ANYFORMAT|SDL_NOFRAME);
#else
    screen = SDL_SetVideoMode(1270, 468, 32, SDL_SWSURFACE|SDL_ANYFORMAT|SDL_NOFRAME);
#endif
//screen = SDL_SetVideoMode(100, 100, 0, 0);
    if (!screen)
    {
        fprintf(stderr, "Could not set SDL video mode , exiting\n");

        return -1;
    }

    fprintf(stdout, "%s\n", argv[1]);

    VideoState *is;
    is = av_malloc(sizeof (VideoState));
    gLobalVS = is;
    //int a = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    //memset(&(is->audioBuf), 0, (AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2);
    //pstrcpy(is->fileName, sizeof (is->fileName), argv[1]);
    is->avSyncType = DEFAULT_AV_SYNC_TYPE;
    av_strlcpy(is->fileName, argv[1], sizeof(is->fileName));
    is->pictqMutex = SDL_CreateMutex();
    is->pictqCond = SDL_CreateCond();
    scheduleRefresh(is, 40);
    is->parseTid = SDL_CreateThread(decodeThread, is);
    if (!is->parseTid)
    {
        av_free(is);

        return -1;
    }

    SDL_Event event;
    for(;;)
    {
        SDL_WaitEvent(&event);
        switch (event.type)
        {
        case FF_ALLOC_EVENT:
            allocPicture(event.user.data1);

            break ;
        case FF_REFRESH_EVENT:
            videoRefreshTimer(event.user.data1);

            break ;
        case SDL_QUIT:
            is->quit = 1;
            SDL_Quit();
            exit(0);

            break ;
        default:

            break ;
        }
    }
// end:
    // sws_freeContext(imgConvertCtx);
    // av_free(buffer);
    // av_free(pFrameRGB);
    // av_free(pFrame);
    // avcodec_close(pVCodecCtx);
    // av_close_input_file(pFormatCtx);

    // fprintf(stderr, "end return.\n");

    return 0;
}

int decodeThread(void *arg)
{
    VideoState *is = (VideoState *)arg;
    AVPacket pkt, *packet = &pkt;
    av_init_packet(packet);

    if (av_open_input_file(&(is->pFormatCtx), is->fileName, NULL, 0, NULL) != 0)
    {
        fprintf(stderr, "av_open_input_file \n");

        return -1;
    }
    if (av_find_stream_info(is->pFormatCtx) < 0)
    {
        fprintf(stderr, "av_find_stream_info \n");

        return -1;
    }
    dump_format(is->pFormatCtx, 0, is->fileName, 0);

    int videoIndex = -1;
    int audioIndex = -1;
    int i;
    for (i = 0; i < is->pFormatCtx->nb_streams; ++i)
    {
        if (is->pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO 
            && videoIndex < 0)
        {
            videoIndex = i;
        }
        if (is->pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO
            && audioIndex < 0)
        {
            audioIndex = i;
        }
    }

    if (videoIndex == -1 || audioIndex == -1)
    {
        fprintf(stdout, " Cannot find the video/audio Index");

        return -1;
    }

    streamComponentOpen(is, videoIndex);
    streamComponentOpen(is, audioIndex);

    for (;;)
    {
        if (is->quit)
        {
            break;
        }
        if (is->audioQ.size > MAX_AUDIOQ_SIZE ||
            is->videoQ.size > MAX_VIDEOQ_SIZE)
        {
            SDL_Delay(10);

            continue ;
        }
        if (av_read_frame(is->pFormatCtx, packet) < 0)
        {
            if (url_ferror(&is->pFormatCtx->pb) == 0)
            {
                SDL_Delay(100);

                continue ;
            }
            else
            {
                break ;
            }
        }

        if (packet->stream_index == is->videoStream)
        {
            packetQueuePut(&is->videoQ, packet);
        }
        else if (packet->stream_index == is->audioStream)
        {
            packetQueuePut(&is->audioQ, packet);
        }
        else
        {
            av_free_packet(packet);
        }
    }
    while (!is->quit)
    {
        SDL_Delay(100);
    }

fail:
    if (1)
    {
        SDL_Event event;
        event.type = FF_QUIT_EVENT;
        event.user.data1 = is;
        SDL_PushEvent(&event);
    }
return 0;
}

int streamComponentOpen(VideoState *is, int streamIndex)
{
    AVFormatContext *pFormatCtx = is->pFormatCtx;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    SDL_AudioSpec wantedSpec, actualSpec;

    fprintf(stdout, "streamIndex %d\n", streamIndex);

    if (streamIndex < 0 || streamIndex > pFormatCtx->nb_streams)
    {
        return -1;
    }

    pCodecCtx = pFormatCtx->streams[streamIndex]->codec;

    if (pCodecCtx->codec_type == AVMEDIA_TYPE_AUDIO)
    {
        wantedSpec.freq = pCodecCtx->sample_rate;
        wantedSpec.format = AUDIO_S16SYS;
        wantedSpec.channels = pCodecCtx->channels;
        wantedSpec.silence = 0;
        wantedSpec.samples = SDL_AUDIO_BUFFER_SIZE;
        wantedSpec.callback = audioCallBack;
        wantedSpec.userdata = is;

        if (SDL_OpenAudio(&wantedSpec, &actualSpec) < 0)
        {
            fprintf(stderr, "SDL_OpenAudio : %s\n", SDL_GetError());

            return -1;
        }
    }

    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (!pCodec || (avcodec_open(pCodecCtx, pCodec)) < 0)
    {
        fprintf(stderr, "Could not open video codec.\n");

        return -1;
    }
    switch (pCodecCtx->codec_type)
    {
    case AVMEDIA_TYPE_AUDIO:
        is->audioStream = streamIndex;
        is->audioSt = pFormatCtx->streams[streamIndex];
        is->audioBufSize = 0;
        is->audioBufIndex = 0;
        is->audioDiffAvgCoef = exp(log(0.01 / AUDIO_DIFF_AVG_NB));
        is->audioDiffAvgCount = 0;
        /* Correct audio only if larger error than this */
        is->audioDiffThreshold = 2.0 * SDL_AUDIO_BUFFER_SIZE / pCodecCtx->sample_rate;

        memset(&(is->audioPkt), 0, sizeof (is->audioPkt));
        packetQueueInit(&(is->audioQ));
        SDL_PauseAudio(0);

        break;
    case AVMEDIA_TYPE_VIDEO:
        is->videoStream = streamIndex;
        is->videoSt = pFormatCtx->streams[streamIndex];
        packetQueueInit(&(is->videoQ));
        is->videoTid = SDL_CreateThread(videoThread, is);
        is->frameTimer = (double)av_gettime() / 1000000.0;
        is->frameLastPts = 40e-3;
        is->videoCurrentPtsTime = av_gettime();
        pCodecCtx->get_buffer = ourGetBuffer;
        pCodecCtx->release_buffer = ourReleaseBuffer;

        break;
    default :
        break;
    }
}

int videoThread(void *arg)
{
    VideoState *is = (VideoState *)arg;
    AVPacket pkt, *packet = &pkt;
    int len, frameFinished;
    AVFrame *pFrame;
    double pts;

    av_init_packet(packet);
    pFrame = avcodec_alloc_frame();
    for (;;)
    {
        if (packetQueueGet(&is->videoQ, packet, 1) < 0)
        {
            break ;
        }

        pts = 0;
        gLocalVideoPktPts = packet->pts;
        len = avcodec_decode_video2(is->videoSt->codec, pFrame, &frameFinished, packet);

        if (packet->dts == AV_NOPTS_VALUE && pFrame->opaque 
            && (uint64_t *)pFrame->opaque != AV_NOPTS_VALUE)
        {
            pts = *(uint64_t *)(pFrame->opaque);
        }
        if (packet->dts != AV_NOPTS_VALUE)
        {
            pts = packet->dts;
        }
        else
        {
            pts = 0;
        }

        pts *= av_q2d(is->videoSt->time_base);

        if (frameFinished)
        {
            pts = synchronizeVideo(is, pFrame, pts);

            if (queuePicture(is, pFrame, pts) < 0)
            {
                break ;
            }
        }
        av_free_packet(packet);
    }
    av_free(pFrame);

    return 0;
}

int queuePicture(VideoState *is, AVFrame *pFrame, double pts)
{
    VideoPicture *vp;
    int dstPixFmt;
    AVPicture pict;

    SDL_LockMutex(is->pictqMutex);
    while (is->pictqSize >= VIDEO_PICTURE_QUEUE_SIZE && !is->quit) 
    {
        SDL_CondWait(is->pictqCond, is->pictqMutex);
    }
    SDL_UnlockMutex(is->pictqMutex);

    if (is->quit)
    {
        return -1;
    }

    vp = &is->pictQ[is->pictqWindex];

    if (!vp->bmp ||
     vp->width != is->videoSt->codec->width ||
     vp->height != is->videoSt->codec->height)
    {
        SDL_Event event;
        vp->allocated = 0;
        event.type = FF_ALLOC_EVENT;
        event.user.data1 = is;
        SDL_PushEvent(&event);

        SDL_LockMutex(is->pictqMutex);
        while (!vp->allocated && !is->quit)
        {
            SDL_CondWait(is->pictqCond, is->pictqMutex);
        }
        SDL_UnlockMutex(is->pictqMutex);

        if (is->quit)
        {
            return -1;
        }
    }
    if (vp->bmp)
    {
        SDL_LockYUVOverlay(vp->bmp);
        dstPixFmt = PIX_FMT_YUV420P;
        pict.data[0] = vp->bmp->pixels[0];
        pict.data[1] = vp->bmp->pixels[1];
        pict.data[2] = vp->bmp->pixels[2];
        pict.linesize[0] = vp->bmp->pitches[0];
        pict.linesize[1] = vp->bmp->pitches[1];
        pict.linesize[2] = vp->bmp->pitches[2];

        if (is->imgConvertCtx == NULL)
        {
            int w = is->videoSt->codec->width;
            int h = is->videoSt->codec->height;
            is->imgConvertCtx = sws_getContext(w, h, is->videoSt->codec->pix_fmt,
                w, h, dstPixFmt, SWS_BICUBIC, NULL, NULL, NULL);
            if (is->imgConvertCtx == NULL)
            {
                 fprintf(stderr, "Cannot initialize the conversion context\n");

                 exit(1);
            }
        }

        sws_scale(is->imgConvertCtx, pFrame->data, pFrame->linesize, 0,
            is->videoSt->codec->height, pict.data, pict.linesize);
        SDL_UnlockYUVOverlay(vp->bmp);

        vp->pts = pts;

        if (++(is->pictqWindex) == VIDEO_PICTURE_QUEUE_SIZE)
        {
            is->pictqWindex = 0;
        }

        SDL_LockMutex(is->pictqMutex);
        is->pictqSize++;
        SDL_UnlockMutex(is->pictqMutex);
    }

    return 0;
}

void allocPicture(void *userdata)
{
    VideoState *is = (VideoState *)userdata;
    VideoPicture *vp;
    vp = &is->pictQ[is->pictqWindex];

    if (vp->bmp)
    {
        SDL_FreeYUVOverlay(vp->bmp);
    }

    vp->bmp = SDL_CreateYUVOverlay(is->videoSt->codec->width, is->videoSt->codec->height,
        SDL_YV12_OVERLAY, screen);
    vp->width = is->videoSt->codec->width;
    vp->height = is->videoSt->codec->height;
    SDL_LockMutex(is->pictqMutex);
    vp->allocated = 1;
    SDL_CondSignal(is->pictqCond);
    SDL_UnlockMutex(is->pictqMutex);
}

void videoRefreshTimer(void *userdata)
{
    VideoState *is = (VideoState *)userdata;
    VideoPicture *vp;
    double actualDelay, delay, syncThreshold, refClock, diff;

    if (is->videoSt)
    {
        if (is->pictqSize == 0)
        {
            scheduleRefresh(is, 1);
        }
        else
        {
            vp = &(is->pictQ[is->pictqRindex]);
            is->videoCurrentPts = vp->pts;
            is->videoCurrentPtsTime = av_gettime();
            delay = vp->pts - is->frameLastPts;

            if (delay <= 0 || delay >= 1.0)
            {
                delay = is->frameLastDelay;
            }

            is->frameLastDelay = delay;
            is->frameLastPts = vp->pts;

            if (is->avSyncType != AV_SYNC_VIDEO_MASTER)
            {
                syncThreshold = (delay > AV_SYNC_THRESHOLD) ? delay : AV_SYNC_THRESHOLD;
                refClock = getAudioClock(is);
                diff = vp->pts - refClock;

                // if (fabs(diff) < AV_SYNC_THRESHOLD)
                // {
                //     if (diff <= -syncThreshold)
                //     {
                //         delay = 0;
                //     }
                //     else if (diff >= syncThreshold)
                //     {
                //         delay = 2 * delay;
                //     }
                // }
                if (diff <= -syncThreshold)
                {
                    delay = 0;
                }
                else if (diff >= syncThreshold)
                {
                    delay = 2 * delay;
                }
            }

            is->frameTimer += delay;
            actualDelay = is->frameTimer - (av_gettime() / 1000000.0);

            if (actualDelay < 0.010)
            {
                actualDelay = 0.010;
            }

            scheduleRefresh(is, (int )(actualDelay * 1000 + 0.5));
            videoDisplay(is);

            if (++(is->pictqRindex) == VIDEO_PICTURE_QUEUE_SIZE)
            {
                is->pictqRindex = 0;
            }

            SDL_LockMutex(is->pictqMutex);
            is->pictqSize--;
            SDL_CondSignal(is->pictqCond);
            SDL_UnlockMutex(is->pictqMutex);
        }
    }
    else
    {
        scheduleRefresh(is, 100);
    }
}

void videoDisplay(VideoState *is)
{
    SDL_Rect rect;
    VideoPicture *vp;
    AVPicture pict;
    float aspectRadio;
    int w, h, x, y;
    int i;

    vp = &is->pictQ[is->pictqRindex];
    if (vp->bmp)
    {
        if (is->videoSt->codec->sample_aspect_ratio.num == 0)
        {
            aspectRadio = 0;
        }
        else
        {
            aspectRadio = av_q2d(is->videoSt->codec->sample_aspect_ratio) *
                is->videoSt->codec->width / is->videoSt->codec->height;
        }
        if (aspectRadio <= 0.0)
        {
            aspectRadio = (float)is->videoSt->codec->width / is->videoSt->codec->height;
        }
        h = screen->h;
        w = ((int)rint(h * aspectRadio)) & (-3);
        if (w > screen->w)
        {
            w = screen->w;
            h = ((int)rint(w * aspectRadio)) & (-3);
        }
        x = (screen->w - w) / 2;
        y = (screen->h - h) / 2;
        rect.x = x;
        rect.y = y;
        rect.w = w;
        rect.h = h;

        SDL_DisplayYUVOverlay(vp->bmp, &rect);
    }
}

double synchronizeVideo(VideoState *is, AVFrame *srcFrame, double pts)
{
    double frameDelay;

    if (pts != 0)
    {
        is->videoClock = pts;
    }
    else
    {
        pts = is->videoClock;
    }

    frameDelay = av_q2d(is->videoSt->codec->time_base);
    frameDelay += srcFrame->repeat_pict * (frameDelay * 0.5);
    is->videoClock += frameDelay;

    return pts;
}

double getVideoClock(VideoState *is)
{
    double delay;

    delay = (av_gettime() - is->videoCurrentPtsTime) / 1000000.0;

    return is->videoCurrentPts + delay;
}

double getExternalClock(VideoState *is)
{
    return av_gettime() / 1000000.0;
}

double getMasterClock(VideoState *is)
{
    if (is->avSyncType == AV_SYNC_VIDEO_MASTER)
    {
        return getVideoClock(is);
    }
    else if (is->avSyncType == AV_SYNC_AUDIO_MASTER)
    {
        return getAudioClock(is);
    }
    else
    {
        return getExternalClock(is);
    }
}

int synchronizeAudio(VideoState *is, short *samples, int samplesSize, double pts)
{
    int n;
    double refClock;

    n = 2 * is->audioSt->codec->channels;

    if(is->avSyncType != AV_SYNC_AUDIO_MASTER)
    {
        double diff, avgDiff;
        int wantedSize, minSize, maxSize, nbSamples;

        refClock = getMasterClock(is);
        diff = getAudioClock(is) - refClock;

        if(diff < AV_NOSYNC_THRESHOLD)
        {
            // accumulate the diffs
            is->audioDiffCum = diff + is->audioDiffAvgCoef * is->audioDiffCum;

            if(is->audioDiffAvgCount < AUDIO_DIFF_AVG_NB)
            {
                is->audioDiffAvgCount++;
            }
            else
            {
                avgDiff = is->audioDiffCum * (1.0 - is->audioDiffAvgCoef);

                if(fabs(avgDiff) >= is->audioDiffThreshold)
                {
                    wantedSize = samplesSize +
                     ((int )(diff * is->audioSt->codec->sample_rate) * n);
                    minSize = samplesSize * ((100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100);
                    maxSize = samplesSize * ((100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100);

                    if(wantedSize < minSize)
                    {
                        wantedSize = minSize;
                    }
                    else if (wantedSize > maxSize)
                    {
                        wantedSize = maxSize;
                    }
                    if(wantedSize < samplesSize)
                    {
                        /* remove samples */
                        samplesSize = wantedSize;
                    }
                    else if(wantedSize > samplesSize)
                    {
                        uint8_t *samplesEnd, *q;
                        int nb;

                        /* add samples by copying final sample*/
                        //nb = (samplesSize - wantedSize);
                        nb = (wantedSize - samplesSize);
                        samplesEnd = (uint8_t *)samples + samplesSize - n;
                        q = samplesEnd + n;

                        while(nb > 0)
                        {
                            memcpy(q, samplesEnd, n);
                            q += n;
                            nb -= n;
                        }

                        samplesSize = wantedSize;
                    }
                }
            }
        }
        else
        {
            /* difference is TOO big; reset diff stuff */
            is->audioDiffAvgCount = 0;
            is->audioDiffCum = 0;
        }
    }
    return samplesSize;
}

void audioCallBack(void *userdata, Uint8 *stream, int len)
{
    fprintf(stdout, "audioCallBack.\n");

    VideoState *is = (VideoState *)userdata;
    int len1, audioSize;
    double pts;

    while (len > 0) 
    {
        if (is->audioBufIndex >= is->audioBufSize)
        {
            audioSize = audioDecodeFrame(is, is->audioBuf, sizeof (is->audioBuf), &pts);

            if (audioSize <= 0)
            {
                is->audioBufSize = 1024;
                memset(is->audioBuf, 0, is->audioBufSize);
            }
            else
            {
                audioSize = synchronizeAudio(is, (int16_t *)is->audioBuf, audioSize, pts);

                audioSize == 0 ? fprintf(stdout, "audioSize %d\n", audioSize) : 1;

                is->audioBufSize = audioSize;
            }

            is->audioBufIndex = 0;
        }

        len1 = is->audioBufSize - is->audioBufIndex;

        if (len1 > len)
        {
            len1 = len;
        }

        memcpy(stream, (uint8_t *)(is->audioBuf) + is->audioBufIndex, len1);
        len -= len1;
        stream += len1;
        is->audioBufIndex += len1;
    }
}

int audioDecodeFrame(VideoState *is, uint8_t *audioBuf, int bufSize, double *ptsPtr)
{
    AVPacket *packet = &(is->audioPkt);
    int len1, dataSize, n;

    for (;;)
    {
        while (is->audioPktSize > 0)
        {
            dataSize = bufSize;
            AVPacket tmpPkt;
            av_init_packet(&tmpPkt);
            tmpPkt.data = is->audioPktData;
            tmpPkt.size = is->audioPktSize;

            len1 = avcodec_decode_audio3(is->audioSt->codec, (int16_t *)audioBuf,
             &dataSize, &tmpPkt);

            if (len1 < 0)
            {
                is->audioPktSize = 0;

                break ;
            }
            if (dataSize <= 0)
            {
                continue ;
            }

            is->audioPktSize -= len1;
            is->audioPktData += len1;
            *ptsPtr = is->audioClock;
            n = 2 * is->audioSt->codec->channels;
            is->audioClock += (double )dataSize / (n * is->audioSt->codec->sample_rate);

            return dataSize;
        }
        if (packet->data)
        {
            av_free_packet(packet);
        }
        if (packetQueueGet(&is->audioQ, packet, 1) < 0)
        {
            return -1;
        }
        if (packet->pts != AV_NOPTS_VALUE)
        {
            is->audioClock = av_q2d(is->audioSt->codec->time_base) * packet->pts;
        }

        is->audioPktSize = packet->size;
        is->audioPktData = packet->data;

    }
}

int ourGetBuffer(struct AVCodecContext *c, AVFrame *pic)
{
    int ret = avcodec_default_get_buffer(c, pic);
    uint64_t *pts = av_malloc(sizeof (uint64_t));
    *pts = gLocalVideoPktPts;
    pic->opaque = pts;

    return ret;
}

void ourReleaseBuffer(struct AVCodecContext *c, AVFrame *pic)
{
    if (pic)
    {
        av_freep(&pic->opaque);
    }
    avcodec_default_release_buffer(c, pic);
}

double getAudioClock(VideoState *is)
{
    double pts;
    int hwBufSize, bytesPerSec = 0, n;

    pts = is->audioClock;
    hwBufSize = is->audioBufSize - is->audioBufIndex;
    n = is->audioSt->codec->channels * 2;
    bytesPerSec = is->audioSt->codec->sample_rate * n;

    if (bytesPerSec)
    {
        pts -= (double)hwBufSize / bytesPerSec;
    }

    return pts;
}

void packetQueueInit(PacketQueue *q)
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
        fprintf(stderr, "av_dup_packet error\n");

        return -1;
    }

    //fprintf(stdout, "stream_index:%d\n", pkt->stream_index);

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
        if (gLobalVS->quit)
        {
            ret = -1;

            break ;
        }

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