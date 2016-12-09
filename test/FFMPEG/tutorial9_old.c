#ifdef __MINGW32__
#undef main /* Prevents SDL from overriding main() */
#endif

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavcodec/colorspace.h"

// #include <android/log.h>
#include <execinfo.h>
#include <signal.h>

#include <SDL.h>
#include <SDL_thread.h>

#define AV_SYNC_THRESHOLD 0.01
#define AV_NOSYNC_THRESHOLD 10.0
#define SDL_AUDIO_BUFFER_SIZE 1024
#define MAX_AUDIOQ_SIZE (10 * 16 * 1024)
#define MAX_VIDEOQ_SIZE (10 * 256 * 1024)
#define FF_ALLOC_EVENT (SDL_USEREVENT)
#define FF_REFRESH_EVENT (SDL_USEREVENT + 1)
#define FF_QUIT_EVENT (SDL_USEREVENT + 2)
#define VIDEO_PLAYPAUSE (SDL_USEREVENT + 3)
#define VIDEO_FF (SDL_USEREVENT + 4)
#define VIDEO_REW (SDL_USEREVENT + 5)
#define VIDEO_PICTURE_QUEUE_SIZE 1
#define SUBPICTURE_QUEUE_SIZE 1
#define AUDIO_DIFF_AVG_NB 20
#define SAMPLE_CORRECTION_PERCENT_MAX 10.0
#define DEFAULT_AV_SYNC_TYPE AV_SYNC_AUDIO_MASTER //AV_SYNC_VIDEO_MASTER
#define LOG_TAG "SDL_FFMPEG"



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

typedef struct SubPicture
{
    double pts; /* presentation time stamp for this picture */
    AVSubtitle sub;
} SubPicture;

typedef struct VideoState
{
    AVFormatContext *pFormatCtx;
    AVFormatContext *pSubFormatCtx;
    int videoStream, audioStream, subtitleStream;
    int avSyncType;
    SDL_Thread *parseTid;
    char fileName[1024];
    char *subFileName;
    int quit;

    //AUDIO
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

    //VIDEO
    AVStream *videoSt;
    PacketQueue videoQ;
    VideoPicture pictQ[VIDEO_PICTURE_QUEUE_SIZE];
    SDL_mutex *pictqMutex;
    SDL_cond *pictqCond;
    SDL_Thread *videoTid;
    int pictqSize, pictqRindex, pictqWindex;
    struct SwsContext *imgConvertCtx;
    double videoClock;
    double frameTimer;
    double frameLastPts;
    double frameLastDelay;
    double videoCurrentPts;
    ///<current displayed pts (different from video_clock if frame fifos are used)
    int64_t videoCurrentPtsTime;
    ///<time (av_gettime) at which we updated video_current_pts - used to have running video pts
    int seekReq;
    int seekFlags;
    int64_t seekPos;
    int paused;
    int lastPaused;
    int readPauseReturn;

    //SUBTITLE
    AVStream *subtitleSt;
    PacketQueue subtitleQ;
    SubPicture subpictQ[SUBPICTURE_QUEUE_SIZE];
    SDL_mutex *subpqMutex;
    SDL_cond *subpqCond;
    SDL_Thread *subtitleTid;
    int subpqSize, subpqRindex, subpqWindex;

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
AVPacket flushPkt;

static const char *retStr(int v);
static void scheduleRefresh(VideoState *is, int delay);
static Uint32 sdlRefreshTimerCb(Uint32 interval, void *opaque);

void allocPicture(void *userdata);
void audioCallBack(void *userdata, Uint8 *stream, int len);
int audioDecodeFrame(VideoState *is, uint8_t *audioBuf, int bufSize, double *ptsPtr);
int decodeThread(void *arg);
void doSeek(VideoState *is, double incr);
int formatContextInit(VideoState *is);
double getAudioClock(VideoState *is);
double getMasterClock(VideoState *is);
int ourGetBuffer(struct AVCodecContext *c, AVFrame *pic);
void ourReleaseBuffer(struct AVCodecContext *c, AVFrame *pic);
void packetQueueInit(PacketQueue* q);
int packetQueuePut(PacketQueue *q, AVPacket *pkt);
int packetQueueGet(PacketQueue *q, AVPacket *pkt, int block);
void packetQueueFlush(PacketQueue *q);
int queuePicture(VideoState *is, AVFrame *pFrame, double pts);
int subtitleThread(void *arg);
void streamPause(VideoState *is);
void streamSeek(VideoState *is, int64_t pos, int rel);
int streamComponentOpen(VideoState *is, AVFormatContext *pFormatCtx, int streamIndex);
double synchronizeVideo(VideoState *is, AVFrame *srcFrame, double pts);
void togglePause(void);
void videoDisplay(VideoState *is);
void videoRefreshTimer(void *userdata);
int videoThread(void *arg);

//static const char *retStr(int v)
//{
//    char buffer[20];
//    switch (v)
//    {
//    case AVERROR_EOF:
//        return "-EOF";
//    case AVERROR(EIO):
//        return "-EIO";
//    case AVERROR(ENOMEM):
//        return "-ENOMEM";
//    case AVERROR(EINVAL):
//        return "-EINVAL";
//    default:
//        snprintf(buffer, sizeof (buffer), "%2d", v);
//
//    return buffer;
//    }
//}

#define ALPHA_BLEND(a, oldp, newp, s)\
((((oldp << s) * (255 - (a))) + (newp * (a))) / (255 << s))

#define RGBA_IN(r, g, b, a, s)\
{\
    unsigned int v = ((const uint32_t *)(s))[0];\
    a = (v >> 24) & 0xff;\
    r = (v >> 16) & 0xff;\
    g = (v >> 8) & 0xff;\
    b = v & 0xff;\
}

#define YUVA_IN(y, u, v, a, s, pal)\
{\
    unsigned int val = ((const uint32_t *)(pal))[*(const uint8_t*)(s)];\
    a = (val >> 24) & 0xff;\
    y = (val >> 16) & 0xff;\
    u = (val >> 8) & 0xff;\
    v = val & 0xff;\
}

#define YUVA_OUT(d, y, u, v, a)\
{\
    ((uint32_t *)(d))[0] = (a << 24) | (y << 16) | (u << 8) | v;\
}


#define BPP 1

static void blend_subrect(AVPicture *dst, const AVSubtitleRect *rect, int imgw, int imgh)
{
    int wrap, wrap3, width2, skip2;
    int y, u, v, a, u1, v1, a1, w, h;
    uint8_t *lum, *cb, *cr;
    const uint8_t *p;
    const uint32_t *pal;
    int dstx, dsty, dstw, dsth;

    dstw = av_clip(rect->w, 0, imgw);
    dsth = av_clip(rect->h, 0, imgh);
    dstx = av_clip(rect->x, 0, imgw - dstw);
    dsty = av_clip(rect->y, 0, imgh - dsth);
    lum = dst->data[0] + dsty * dst->linesize[0];
    cb = dst->data[1] + (dsty >> 1) * dst->linesize[1];
    cr = dst->data[2] + (dsty >> 1) * dst->linesize[2];

    width2 = ((dstw + 1) >> 1) + (dstx & ~dstw & 1);
    skip2 = dstx >> 1;
    wrap = dst->linesize[0];
    wrap3 = rect->pict.linesize[0];
    p = rect->pict.data[0];
    pal = (const uint32_t *)rect->pict.data[1];  /* Now in YCrCb! */

    if (dsty & 1) {
        lum += dstx;
        cb += skip2;
        cr += skip2;

        if (dstx & 1) {
            YUVA_IN(y, u, v, a, p, pal);
            lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
            cb[0] = ALPHA_BLEND(a >> 2, cb[0], u, 0);
            cr[0] = ALPHA_BLEND(a >> 2, cr[0], v, 0);
            cb++;
            cr++;
            lum++;
            p += BPP;
        }
        for(w = dstw - (dstx & 1); w >= 2; w -= 2) {
            YUVA_IN(y, u, v, a, p, pal);
            u1 = u;
            v1 = v;
            a1 = a;
            lum[0] = ALPHA_BLEND(a, lum[0], y, 0);

            YUVA_IN(y, u, v, a, p + BPP, pal);
            u1 += u;
            v1 += v;
            a1 += a;
            lum[1] = ALPHA_BLEND(a, lum[1], y, 0);
            cb[0] = ALPHA_BLEND(a1 >> 2, cb[0], u1, 1);
            cr[0] = ALPHA_BLEND(a1 >> 2, cr[0], v1, 1);
            cb++;
            cr++;
            p += 2 * BPP;
            lum += 2;
        }
        if (w) {
            YUVA_IN(y, u, v, a, p, pal);
            lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
            cb[0] = ALPHA_BLEND(a >> 2, cb[0], u, 0);
            cr[0] = ALPHA_BLEND(a >> 2, cr[0], v, 0);
            p++;
            lum++;
        }
        p += wrap3 - dstw * BPP;
        lum += wrap - dstw - dstx;
        cb += dst->linesize[1] - width2 - skip2;
        cr += dst->linesize[2] - width2 - skip2;
    }
    for(h = dsth - (dsty & 1); h >= 2; h -= 2) {
        lum += dstx;
        cb += skip2;
        cr += skip2;

        if (dstx & 1) {
            YUVA_IN(y, u, v, a, p, pal);
            u1 = u;
            v1 = v;
            a1 = a;
            lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
            p += wrap3;
            lum += wrap;
            YUVA_IN(y, u, v, a, p, pal);
            u1 += u;
            v1 += v;
            a1 += a;
            lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
            cb[0] = ALPHA_BLEND(a1 >> 2, cb[0], u1, 1);
            cr[0] = ALPHA_BLEND(a1 >> 2, cr[0], v1, 1);
            cb++;
            cr++;
            p += -wrap3 + BPP;
            lum += -wrap + 1;
        }
        for(w = dstw - (dstx & 1); w >= 2; w -= 2) {
            YUVA_IN(y, u, v, a, p, pal);
            u1 = u;
            v1 = v;
            a1 = a;
            lum[0] = ALPHA_BLEND(a, lum[0], y, 0);

            YUVA_IN(y, u, v, a, p + BPP, pal);
            u1 += u;
            v1 += v;
            a1 += a;
            lum[1] = ALPHA_BLEND(a, lum[1], y, 0);
            p += wrap3;
            lum += wrap;

            YUVA_IN(y, u, v, a, p, pal);
            u1 += u;
            v1 += v;
            a1 += a;
            lum[0] = ALPHA_BLEND(a, lum[0], y, 0);

            YUVA_IN(y, u, v, a, p + BPP, pal);
            u1 += u;
            v1 += v;
            a1 += a;
            lum[1] = ALPHA_BLEND(a, lum[1], y, 0);

            cb[0] = ALPHA_BLEND(a1 >> 2, cb[0], u1, 2);
            cr[0] = ALPHA_BLEND(a1 >> 2, cr[0], v1, 2);

            cb++;
            cr++;
            p += -wrap3 + 2 * BPP;
            lum += -wrap + 2;
        }
        if (w) {
            YUVA_IN(y, u, v, a, p, pal);
            u1 = u;
            v1 = v;
            a1 = a;
            lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
            p += wrap3;
            lum += wrap;
            YUVA_IN(y, u, v, a, p, pal);
            u1 += u;
            v1 += v;
            a1 += a;
            lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
            cb[0] = ALPHA_BLEND(a1 >> 2, cb[0], u1, 1);
            cr[0] = ALPHA_BLEND(a1 >> 2, cr[0], v1, 1);
            cb++;
            cr++;
            p += -wrap3 + BPP;
            lum += -wrap + 1;
        }
        p += wrap3 + (wrap3 - dstw * BPP);
        lum += wrap + (wrap - dstw - dstx);
        cb += dst->linesize[1] - width2 - skip2;
        cr += dst->linesize[2] - width2 - skip2;
    }
    /* handle odd height */
    if (h) {
        lum += dstx;
        cb += skip2;
        cr += skip2;

        if (dstx & 1) {
            YUVA_IN(y, u, v, a, p, pal);
            lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
            cb[0] = ALPHA_BLEND(a >> 2, cb[0], u, 0);
            cr[0] = ALPHA_BLEND(a >> 2, cr[0], v, 0);
            cb++;
            cr++;
            lum++;
            p += BPP;
        }
        for(w = dstw - (dstx & 1); w >= 2; w -= 2) {
            YUVA_IN(y, u, v, a, p, pal);
            u1 = u;
            v1 = v;
            a1 = a;
            lum[0] = ALPHA_BLEND(a, lum[0], y, 0);

            YUVA_IN(y, u, v, a, p + BPP, pal);
            u1 += u;
            v1 += v;
            a1 += a;
            lum[1] = ALPHA_BLEND(a, lum[1], y, 0);
            cb[0] = ALPHA_BLEND(a1 >> 2, cb[0], u, 1);
            cr[0] = ALPHA_BLEND(a1 >> 2, cr[0], v, 1);
            cb++;
            cr++;
            p += 2 * BPP;
            lum += 2;
        }
        if (w) {
            YUVA_IN(y, u, v, a, p, pal);
            lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
            cb[0] = ALPHA_BLEND(a >> 2, cb[0], u, 0);
            cr[0] = ALPHA_BLEND(a >> 2, cr[0], v, 0);
        }
    }
}

static void dumpSubtitle(AVSubtitle *sub)
{
    if (sub == NULL)
    {
        return ;
    }

    fprintf(stdout, "%p\n", sub);
    fprintf(stdout, "format:%u \n", sub->format);
    fprintf(stdout, "start_display_time:%u \n", sub->start_display_time);
    fprintf(stdout, "end_display_time:%u \n", sub->end_display_time);
    fprintf(stdout, "num_rects:%u \n", sub->num_rects);
    fprintf(stdout, "pts:%lld \n", sub->pts);

    int i;
    for (i = 0; i < sub->num_rects; ++i)
    {
        AVSubtitleRect *rect = sub->rects[i];
        fprintf(stdout, "x:%d\n", rect->x);
        fprintf(stdout, "y:%d\n", rect->y);
        fprintf(stdout, "w:%d\n", rect->w);
        fprintf(stdout, "h:%d\n", rect->h);
        fprintf(stdout, "h:%06x\n", rect->nb_colors);

        switch(rect->type)
        {
        case SUBTITLE_NONE:
            fprintf(stdout, "SUBTITLE_NONE\n");

            break ;
        case SUBTITLE_TEXT:
            fprintf(stdout, "SUBTITLE_TEXT\n");

            break ;
        case SUBTITLE_BITMAP:
            fprintf(stdout, "SUBTITLE_BITMAP\n");

            break ;
        case SUBTITLE_ASS:
            fprintf(stdout, "SUBTITLE_ASS\n");

            break ;
        default:
            break;
        }

        rect->text != NULL ? fprintf(stdout, "text:%s\n", rect->text) : 1;
        rect->text != NULL ? fprintf(stdout, "ass:%s\n", rect->ass) : 1;
    }
}

static void freeSubpicture(SubPicture *sp)
{
    int i;

    for (i = 0; i < sp->sub.num_rects; i++)
    {
        av_freep(&sp->sub.rects[i]->pict.data[0]);
        av_freep(&sp->sub.rects[i]->pict.data[1]);
        av_freep(&sp->sub.rects[i]);
    }

    av_free(sp->sub.rects);

    memset(&sp->sub, 0, sizeof(AVSubtitle));
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

static void widebrightSegvHandler(int signum)
{
    void *array[10];
    size_t size;
    char **strings;
    size_t i, j;

    signal(signum, SIG_DFL); /* 还原默认的信号处理handler */

    size = backtrace(array, 10);
    strings = (char **)backtrace_symbols(array, size);

    fprintf(stderr, "widebright received SIGSEGV! Stack trace:\n");

    for (i = 0; i < size; i++)
    {
        fprintf(stderr, "%d %s \n", i, strings[i]);
    }

    free (strings);

    exit(1);
}

int main(int argc, char* argv[])
{
    signal(SIGSEGV, widebrightSegvHandler);
    // SIGSEGV      11       Core    Invalid memory reference
    signal(SIGABRT, widebrightSegvHandler);
    // SIGABRT       6       Core    Abort signal from

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

    av_init_packet(&flushPkt);
    flushPkt.data = "FLUSH";

    VideoState *is;
    is = av_malloc(sizeof (VideoState));
    gLobalVS = is;
    //is->paused = 1;
    //int a = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    //memset(&(is->audioBuf), 0, (AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2);
    //pstrcpy(is->fileName, sizeof (is->fileName), argv[1]);
    is->avSyncType = DEFAULT_AV_SYNC_TYPE;
    av_strlcpy(is->fileName, argv[1], sizeof(is->fileName));

    if (argv[2])
    {
        char* fileName = (char *)av_malloc(sizeof (char) * 1024);
        av_strlcpy(fileName, argv[2], 1024);
        is->subFileName = fileName;

        fprintf(stdout, "%s\n", is->subFileName);
    }

    is->pictqMutex = SDL_CreateMutex();
    is->pictqCond = SDL_CreateCond();
    is->subpqMutex = SDL_CreateMutex();
    is->subpqCond = SDL_CreateCond();
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
        double incr, pos;

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
            fprintf(stdout, " quit .");

            is->quit = 1;
            SDL_Quit();
            exit(0);

            break ;
        case SDL_KEYDOWN:
            switch(event.key.keysym.sym)
            {
            case SDLK_LEFT:
                doSeek(is, -10);

                break ;
            case SDLK_RIGHT:
                doSeek(is, 10);

                break ;
            case SDLK_UP:
                doSeek(is, 60);

                break ;
            case SDLK_DOWN:
                doSeek(is, -60);

                break ;
            default:

                break;
            }
        case VIDEO_FF:
            doSeek(is, 10);

            break ;
        case VIDEO_REW:
            doSeek(is, -10);

            break ;
        case VIDEO_PLAYPAUSE:
            togglePause();

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
    AVPacket pkt, subPkt, *packet = &pkt, *subpacket = &subPkt;

    int ret = formatContextInit(is);
    if (ret < 0)
    {
        fprintf(stderr, "formatContextInit fail.\n");

        goto fail;
    }

    int videoIndex = -1;
    int audioIndex = -1;
    int subtitleIndex = -1;
    int i;
    for (i = 0; i < is->pFormatCtx->nb_streams; ++i)
    {
        fprintf(stdout, "pFormatCtx->nb_streams index: %d\n", i);

        enum AVMediaType codec_type = is->pFormatCtx->streams[i]->codec->codec_type;
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
    for (i = 0; i < is->pSubFormatCtx->nb_streams; ++i)
    {
        fprintf(stdout, "pSubFormatCtx->nb_streams index: %d\n", i);

        enum AVMediaType codec_type = is->pSubFormatCtx->streams[i]->codec->codec_type;
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

        if (is->pSubFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_SUBTITLE)
        {
            subtitleIndex = i;
        }
    }
    if (videoIndex == -1 || audioIndex == -1 || subtitleIndex == -1)
    {
        fprintf(stdout, " Cannot find the video/audio/subtitleIndex Index");

        return -1;
    }

    av_init_packet(packet);
    av_init_packet(subpacket);
    streamComponentOpen(is, is->pFormatCtx, videoIndex);
    streamComponentOpen(is, is->pFormatCtx, audioIndex);
    streamComponentOpen(is, is->pSubFormatCtx, subtitleIndex);

    for (;;)
    {
        if (is->quit)
        {
            break ;
        }
        if (is->paused != is->lastPaused)
        {
            fprintf(stdout, "is->paused:%d\n", is->paused);

            is->lastPaused = is->paused;
            if (is->paused)
            {
                is->readPauseReturn = av_read_pause(is->pFormatCtx);
            }
            else
            {
                av_read_play(is->pFormatCtx);
            }
        }
        if(is->seekReq)
        {
            int streamIndex= -1;
            int64_t seekTarget = is->seekPos;

            if (is->videoStream >= 0)
            {
                streamIndex = is->videoStream;
            }
            else if (is->audioStream >= 0)
            {
                streamIndex = is->audioStream;
            }
            if(streamIndex >= 0)
            {
                seekTarget = av_rescale_q(seekTarget, AV_TIME_BASE_Q,
                 is->pFormatCtx->streams[streamIndex]->time_base);
            }
            if (av_seek_frame(is->pFormatCtx, streamIndex, seekTarget, is->seekFlags) < 0)
            {
                if (is->pFormatCtx->iformat->read_seek2)
                {
                    fprintf(stdout, "format specific\n");
                }
                else if(is->pFormatCtx->iformat->read_timestamp)
                {
                    fprintf(stdout, "frame_binary\n");
                }
                else
                {
                    fprintf(stdout, "generic\n");
                }

                fprintf(stderr, "%s: error while seeking. target: %llx, streamIndex: %d\n",
                 is->fileName, seekTarget, streamIndex);
            }
            else
            {
                if(is->audioStream >= 0)
                {
                    packetQueueFlush(&is->audioQ);
                    packetQueuePut(&is->audioQ, &flushPkt);
                }
                if(is->videoStream >= 0)
                {
                    packetQueueFlush(&is->videoQ);
                    packetQueuePut(&is->videoQ, &flushPkt);
                }
            }
            is->seekReq = 0;
        }

        if (is->audioQ.size > MAX_AUDIOQ_SIZE ||
            is->videoQ.size > MAX_VIDEOQ_SIZE)
        {
            SDL_Delay(10);

            continue ;
        }
        if (av_read_frame(is->pFormatCtx, packet) < 0)
        {
            if (url_feof(is->pFormatCtx->pb) == 0)
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
        if (av_read_frame(is->pSubFormatCtx, subpacket)
         && subpacket->stream_index == is->subtitleStream)
        {
            packetQueuePut(&is->subtitleQ, subpacket);
        }
        else
        {
            av_free_packet(subpacket);
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

int formatContextInit(VideoState *is)
{
    // is->pFormatCtx = avformat_alloc_context();
    // is->pSubFormatCtx = avformat_alloc_context();

    // fprintf(stdout, "fileName:%s\n", is->fileName);
    // fprintf(stdout, "subFileName:%s\n", is->subFileName);

    // if (avformat_open_input(&(is->pFormatCtx), is->fileName, NULL, NULL) < 0
    //     || avformat_open_input(&(is->pSubFormatCtx), is->subFileName, NULL, NULL) < 0)
    // {
    //     fprintf(stderr, "avformat_open_input fail.\n");

    //     return -1;
    // }
    // if (avformat_find_stream_info(is->pFormatCtx, NULL) < 0
    //  || avformat_find_stream_info(is->pSubFormatCtx, NULL) < 0)
    // {
    //     fprintf(stderr, "av_find_stream_info fail.\n");

    //     return -1;
    // }

    // av_dump_format(is->pFormatCtx, 0, is->fileName, 0);
    // av_dump_format(is->pSubFormatCtx, 0, is->subFileName, 0);

    //is->pFormatCtx = avformat_alloc_context();
    //is->pSubFormatCtx = avformat_alloc_context();

    fprintf(stdout, "fileName:%s\n", is->fileName);
    fprintf(stdout, "subFileName:%s\n", is->subFileName);

    if (av_open_input_file(&(is->pFormatCtx), is->fileName, NULL, 0, NULL) < 0
        || av_open_input_file(&(is->pSubFormatCtx), is->subFileName, NULL, 0, NULL) < 0)
    {
        fprintf(stderr, "av_open_input_file fail.\n");

        return -1;
    }
    if (av_find_stream_info(is->pFormatCtx) < 0
     || av_find_stream_info(is->pSubFormatCtx) < 0)
    {
        fprintf(stderr, "av_find_stream_info fail.\n");

        return -1;
    }

    dump_format(is->pFormatCtx, 0, is->fileName, 0);
    dump_format(is->pSubFormatCtx, 0, is->subFileName, 0);

    return 0;
}

int streamComponentOpen(VideoState *is, AVFormatContext *pFormatCtx, int streamIndex)
{
    //AVFormatContext *pFormatCtx = is->pFormatCtx;
    AVStream *avStream;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    SDL_AudioSpec wantedSpec, actualSpec;

    fprintf(stdout, "streamIndex %d\n", streamIndex);

    if (streamIndex < 0 || streamIndex > is->pFormatCtx->nb_streams)
    {
        return -1;
    }

    avStream = pFormatCtx->streams[streamIndex];
    pCodecCtx = pFormatCtx->streams[streamIndex]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

    if ((avcodec_open(pCodecCtx, pCodec)) < 0)
    {
        fprintf(stderr, "Could not open video codec.\n");

        return -1;
    }

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
    case AVMEDIA_TYPE_SUBTITLE:
        is->subtitleStream = streamIndex;
        is->subtitleSt = pFormatCtx->streams[streamIndex];
        packetQueueInit(&is->subtitleQ);
        is->subtitleTid = SDL_CreateThread(subtitleThread, is);

        break ;
    default :
        break;
    }
}

int subtitleThread(void *arg)
{
    VideoState *is = arg;
    SubPicture *pict;
    AVPacket pkt1, *pkt = &pkt1;
    int len1, gotSubtitle;
    double pts;
    int i, j;
    int r, g, b, y, u, v, a;

    for(;;)
    {
        while (is->paused)
        {
            SDL_Delay(10);
        }
        if (packetQueueGet(&is->subtitleQ, pkt, 1) < 0)
        {
            break;
        }
        if (pkt->data == flushPkt.data)
        {
            avcodec_flush_buffers(is->subtitleSt->codec);

            continue;
        }

        SDL_LockMutex(is->subpqMutex);
        while (is->subpqSize >= SUBPICTURE_QUEUE_SIZE)
        {
            SDL_CondWait(is->subpqCond, is->subpqMutex);
        }
        SDL_UnlockMutex(is->subpqMutex);

        pict = &is->subpictQ[is->subpqWindex];

       /* NOTE: ipts is the PTS of the _first_ picture beginning in
           this packet, if any */
        pts = 0;
        if (pkt->pts != AV_NOPTS_VALUE)
        {
            pts = av_q2d(is->subtitleSt->time_base) * pkt->pts;
        }

        len1 = avcodec_decode_subtitle2(is->subtitleSt->codec,
         &(pict->sub), &gotSubtitle, pkt);

        fprintf(stdout, "gotSubtitle:%d\n", gotSubtitle);
        dumpSubtitle(&(pict->sub));

        if (gotSubtitle && pict->sub.format == 0)
        {
            pict->pts = pts;

            for (i = 0; i < pict->sub.num_rects; i++)
            {
                for (j = 0; j < pict->sub.rects[i]->nb_colors; j++)
                {
                    RGBA_IN(r, g, b, a, (uint32_t*)pict->sub.rects[i]->pict.data[1] + j);
                    y = RGB_TO_Y_CCIR(r, g, b);
                    u = RGB_TO_U_CCIR(r, g, b, 0);
                    v = RGB_TO_V_CCIR(r, g, b, 0);
                    YUVA_OUT((uint32_t*)pict->sub.rects[i]->pict.data[1] + j, y, u, v, a);
                }
            }

            /* now we can update the picture count */
            if (++is->subpqWindex == SUBPICTURE_QUEUE_SIZE)
            {
                is->subpqWindex = 0;
            }

            SDL_LockMutex(is->subpqMutex);
            is->subpqSize++;
            SDL_UnlockMutex(is->subpqMutex);
        }

        av_free_packet(pkt);
    }

    return 0;
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
        while(is->paused && !is->quit) {
            SDL_Delay(10);
        }
        if (is->quit)
        {
            break ;
        }
        if (packetQueueGet(&is->videoQ, packet, 1) < 0)
        {
            break ;
        }
        if(packet->data == flushPkt.data)
        {
            avcodec_flush_buffers(is->videoSt->codec);

            continue;
        }

        pts = 0;
        gLocalVideoPktPts = packet->pts;
        len = avcodec_decode_video2(is->videoSt->codec, pFrame, &frameFinished, packet);

        if (packet->dts == AV_NOPTS_VALUE && pFrame->opaque != NULL
            && *(uint64_t *)(pFrame->opaque) != AV_NOPTS_VALUE)
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
    SubPicture *subpict;
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

            if(is->subtitleSt)
            {
                if (is->subpqSize > 0)
                {
                    subpict = &is->subpictQ[is->subpqRindex];

                    if ((is->videoCurrentPts >
                         (subpict->pts + ((float )subpict->sub.end_display_time / 1000))))
                    {
                        freeSubpicture(subpict);

                        /* update queue size and signal for next picture */
                        if (++is->subpqRindex == SUBPICTURE_QUEUE_SIZE)
                        {
                            is->subpqRindex = 0;
                        }

                        SDL_LockMutex(is->subpqMutex);
                        is->subpqSize--;
                        SDL_CondSignal(is->subpqCond);
                        SDL_UnlockMutex(is->subpqMutex);
                    }
                }
            }

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
    SubPicture *subpict;
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
            aspectRadio = (float )is->videoSt->codec->width / is->videoSt->codec->height;
        }
        h = screen->h;
        w = ((int )rint(h * aspectRadio)) & (-3);
        if (w > screen->w)
        {
            w = screen->w;
            h = ((int )rint(w * aspectRadio)) & (-3);
        }
        x = (screen->w - w) / 2;
        y = (screen->h - h) / 2;
        rect.x = x;
        rect.y = y;
        rect.w = w;
        rect.h = h;

        if (is->subtitleSt)
        {
            if (is->subpqSize > 0)
            {
                subpict = &is->subpictQ[is->subpqRindex];

                if (vp->pts >= subpict->pts + ((float )subpict->sub.start_display_time / 1000))
                {
                    SDL_LockYUVOverlay(vp->bmp);

                    pict.data[0] = vp->bmp->pixels[0];
                    pict.data[1] = vp->bmp->pixels[2];
                    pict.data[2] = vp->bmp->pixels[1];
                    pict.linesize[0] = vp->bmp->pitches[0];
                    pict.linesize[1] = vp->bmp->pitches[2];
                    pict.linesize[2] = vp->bmp->pitches[1];

                    for (i = 0; i < subpict->sub.num_rects; i++)
                    {
                        blend_subrect(&pict, subpict->sub.rects[i],
                                      vp->bmp->w, vp->bmp->h);
                    }

                    SDL_UnlockYUVOverlay(vp->bmp);
                }
            }
        }

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

void togglePause(void)
{
    fprintf(stdout, "togglePause.");

    if (gLobalVS)
    {
        streamPause(gLobalVS);
    }
}

void streamPause(VideoState *is)
{
    is->paused = !is->paused;

    fprintf(stdout, "is->paused:%d\n", is->paused);
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
    //fprintf(stdout, "audioCallBack.\n");

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
        if (packet->data && packet->data != flushPkt.data)
        {
            av_free_packet(packet);
        }
        if (is->paused)
        {
            return -1;
        }
        if (packetQueueGet(&is->audioQ, packet, 1) < 0)
        {
            return -1;
        }
        if (packet->data == flushPkt.data)
        {
            avcodec_flush_buffers(is->audioSt->codec);

            continue;
        }
        if (packet->pts != AV_NOPTS_VALUE)
        {
            is->audioClock = av_q2d(is->audioSt->codec->time_base) * packet->pts;

            //fprintf(stdout, "is->audioClock : %lf\n", is->audioClock);
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

void doSeek(VideoState *is, double incr)
{
    if (is)
    {
        double pos = getMasterClock(is);

        fprintf(stdout, "pos:%f\n", pos);

        pos += incr;
        streamSeek(is, (int64_t)(pos * AV_TIME_BASE), incr);
    }
}

void streamSeek(VideoState *is, int64_t pos, int rel)
{
    if(!is->seekReq)
    {
        is->seekPos = pos;
        is->seekFlags = rel < 0 ? AVSEEK_FLAG_BACKWARD : 0;
        is->seekReq = 1;
    }
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

    if (av_dup_packet(pkt) < 0 && pkt != &flushPkt)
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

void packetQueueFlush(PacketQueue *q)
{
    AVPacketList *pkt, *pkt1;

    SDL_LockMutex(q->mutex);
    for(pkt = q->firstPkt; pkt != NULL; pkt = pkt1) 
    {
        pkt1 = pkt->next;
        av_free_packet(&pkt->pkt);
        av_freep(&pkt);
    }

    q->lastPkt = NULL;
    q->firstPkt = NULL;
    q->nbPackets = 0;
    q->size = 0;
    SDL_UnlockMutex(q->mutex);
}

////////////////////////////////////////////////////////////

