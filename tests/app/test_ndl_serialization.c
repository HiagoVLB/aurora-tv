/* Real NDL driver, simulated firmware. No TV, SDL or network required. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "ndl_common.h"
#include "ss4s/modapi.h"
#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include <unistd.h>

extern bool SS4S_ModuleOpen_NDL_WEBOS5(SS4S_Module *, const SS4S_LibraryContext *);

static NDLMediaLoadCallback callback;
static atomic_int active_calls, overlaps, video_entered, audio_calls, unload_calls;
static atomic_bool hold_video, release_video, contender_started;
static atomic_bool delayed_load, reject_video;
static atomic_bool load_entered;
static unsigned delayed_feed_us = 350000;
/* SS4S holds its player mutex while invoking OpenAudio. Model that lock in
 * ReportFrame too, to catch inversions when the prime releases the media lock. */
static pthread_mutex_t player_lock = PTHREAD_MUTEX_INITIALIZER;
static SS4S_PlayerContext *ctx;
static unsigned char payload[16];

static void enter_ndl(void) {
    if (atomic_fetch_add(&active_calls, 1) != 0) atomic_fetch_add(&overlaps, 1);
}
static void leave_ndl(void) { atomic_fetch_sub(&active_calls, 1); }
static void log_noop(SS4S_LogLevel level, const char *tag, const char *fmt, ...) {
    (void) level; (void) tag; (void) fmt;
}
static void report_noop(SS4S_Player *player, uint32_t latency) {
    (void) player; (void) latency;
    pthread_mutex_lock(&player_lock); pthread_mutex_unlock(&player_lock);
}
bool SS4S_webOS_Is_HighEnd_SoC(void) { return true; }
int SS4S_webOS_GetMaxVideoResolution(SS4S_VideoCodec codec, unsigned *w, unsigned *h, unsigned *fps) {
    (void) codec; (void) w; (void) h; (void) fps; return -1;
}
int NDL_DirectMediaInit(const char *app_id, ResourceReleased cb) { (void) app_id; (void) cb; return 0; }
int NDL_DirectMediaQuit(void) { return 0; }
const char *NDL_DirectMediaGetError(void) { return "mock error"; }
int NDL_DirectMediaLoad(NDL_DIRECTMEDIA_DATA_INFO_T *info, NDLMediaLoadCallback cb) {
    (void) info; enter_ndl(); callback = cb;
    atomic_store(&load_entered, true);
    if (!atomic_load(&delayed_load)) {
        callback(0x16, 0, "true"); callback(0x1a, 0, "true");
    }
    leave_ndl(); return 0;
}
int NDL_DirectMediaUnload(void) {
    enter_ndl(); atomic_fetch_add(&unload_calls, 1);
    callback(0x17, 0, "true"); leave_ndl(); return 0;
}
int NDL_DirectVideoPlay(void *data, unsigned size, long long pts) {
    assert(data != NULL && size == sizeof(payload) && pts >= 0);
    enter_ndl(); atomic_fetch_add(&video_entered, 1);
    /* A synchronous callback must never wait for the calling thread's lock. */
    callback(0x31, 0, NULL);
    if (atomic_exchange(&delayed_load, false)) callback(0x16, 0, "true");
    while (atomic_load(&hold_video) && !atomic_load(&release_video)) usleep(100);
    usleep(100);
    leave_ndl(); return atomic_exchange(&reject_video, false) ? -1 : 0;
}
int NDL_DirectAudioPlay(void *data, unsigned size, long long pts) {
    assert(data != NULL && size > 0 && pts >= 0);
    enter_ndl(); atomic_fetch_add(&audio_calls, 1); usleep(100); leave_ndl(); return 0;
}
int NDL_DirectVideoGetRenderBufferLength(int *length) {
    enter_ndl(); *length = 1; leave_ndl(); return 0;
}
int NDL_DirectVideoSetFrameDropThreshold(int threshold) { (void) threshold; return 0; }
int NDL_DirectVideoSetHDRInfo(NDL_DIRECTVIDEO_HDR_INFO_T info) { (void) info; return 0; }

static void *feed_video(void *arg) {
    int count = *(int *) arg;
    for (int i = 0; i < count; i++)
        assert(SS4S_NDL_webOS5_VideoDriver.Feed((void *) ctx, payload, sizeof(payload), 0) == SS4S_VIDEO_FEED_OK);
    return NULL;
}
static void *feed_audio(void *arg) {
    atomic_store(&contender_started, true);
    int count = *(int *) arg;
    for (int i = 0; i < count; i++)
        assert(SS4S_NDL_webOS5_AudioDriver.Feed((void *) ctx, payload, sizeof(payload)) == SS4S_AUDIO_FEED_OK);
    return NULL;
}
static void *close_audio(void *arg) {
    (void) arg; atomic_store(&contender_started, true);
    SS4S_NDL_webOS5_AudioDriver.Close((void *) ctx); return NULL;
}
static void *open_audio(void *arg) {
    SS4S_AudioInstance **instance = arg;
    SS4S_AudioInfo audio = {.codec=SS4S_AUDIO_PCM_S16LE, .sampleRate=48000, .numOfChannels=2};
    pthread_mutex_lock(&player_lock);
    assert(SS4S_NDL_webOS5_AudioDriver.Open(&audio, instance, ctx) == SS4S_AUDIO_OPEN_OK);
    pthread_mutex_unlock(&player_lock);
    return NULL;
}
static void wait_for(atomic_int *value) {
    for (int i = 0; i < 10000 && !atomic_load(value); i++) usleep(100);
    assert(atomic_load(value));
}
static void run_codec(SS4S_VideoCodec codec) {
    fprintf(stderr, "NDL test codec=%d delayed_load=%d\n", codec, atomic_load(&delayed_load));
    ctx = SS4S_NDL_webOS5_PlayerDriver.Create(NULL);
    SS4S_NDL_webOS5_PlayerDriver.SetWaitAudioVideoReady(ctx, true);
    SS4S_VideoInfo video = {.codec=codec, .width=2560, .height=1440, .frameRateNumerator=90, .frameRateDenominator=1};
    SS4S_VideoInstance *vi;
    assert(SS4S_NDL_webOS5_VideoDriver.Open(&video, NULL, &vi, ctx) == SS4S_VIDEO_OPEN_OK);
    SS4S_AudioInstance *ai;
    if (atomic_load(&delayed_load)) {
        atomic_store(&load_entered, false);
        pthread_t opening;
        pthread_create(&opening, NULL, open_audio, &ai);
        while (!atomic_load(&load_entered)) usleep(100);
        usleep(delayed_feed_us); /* cover both budgeted prime and continuation */
        int count = 2;
        feed_video(&count);
        fprintf(stderr, "NDL test: early video fed\n");
        pthread_join(opening, NULL);
    } else {
        open_audio(&ai);
    }

    pthread_t vthread, athread;
    int count = 1000;
    pthread_create(&vthread, NULL, feed_video, &count);
    pthread_create(&athread, NULL, feed_audio, &count);
    pthread_join(vthread, NULL); pthread_join(athread, NULL);
    fprintf(stderr, "NDL test: concurrent feeds complete\n");
    assert(atomic_load(&overlaps) == 0);
    atomic_store(&reject_video, true);
    assert(SS4S_NDL_webOS5_VideoDriver.Feed(vi, payload, sizeof(payload), 0) == SS4S_VIDEO_FEED_ERROR);
    assert(SS4S_NDL_webOS5_VideoDriver.Feed(vi, payload, sizeof(payload), 0) == SS4S_VIDEO_FEED_OK);
    assert(SS4S_NDL_webOS5_AudioDriver.Feed(ai, payload, sizeof(payload)) == SS4S_AUDIO_FEED_OK);

    /* Force an audio close exactly while video is inside the firmware call. */
    count = 1;
    atomic_store(&hold_video, true); atomic_store(&release_video, false);
    atomic_store(&video_entered, 0); atomic_store(&contender_started, false);
    int previous_unloads = atomic_load(&unload_calls);
    pthread_create(&vthread, NULL, feed_video, &count);
    wait_for(&video_entered);
    pthread_create(&athread, NULL, close_audio, NULL);
    while (!atomic_load(&contender_started)) usleep(100);
    usleep(20000);
    bool unloaded_during_feed = atomic_load(&unload_calls) != previous_unloads;
    atomic_store(&release_video, true);
    pthread_join(vthread, NULL); pthread_join(athread, NULL);
    assert(!unloaded_during_feed);
    assert(atomic_load(&overlaps) == 0);
    assert(SS4S_NDL_webOS5_VideoDriver.Feed(vi, payload, sizeof(payload), 0) == SS4S_VIDEO_FEED_NOT_READY);
    SS4S_NDL_webOS5_VideoDriver.Close(vi);
    SS4S_NDL_webOS5_PlayerDriver.Destroy(ctx);
    atomic_store(&hold_video, false);
}
int main(void) {
    alarm(60);
    SS4S_Module module;
    const SS4S_LibraryContext lib = {.Log=log_noop, .VideoStats.ReportFrame=report_noop};
    assert(SS4S_ModuleOpen_NDL_WEBOS5(&module, &lib));
    run_codec(SS4S_VIDEO_AV1); run_codec(SS4S_VIDEO_H265); run_codec(SS4S_VIDEO_H264);
    atomic_store(&delayed_load, true);
    run_codec(SS4S_VIDEO_AV1);
    delayed_feed_us = 650000;
    atomic_store(&delayed_load, true);
    run_codec(SS4S_VIDEO_AV1);
    SS4S_NDL_webOS5_Driver_Quit();
    puts("NDL: AV1/HEVC/H264 concurrent A/V, close during feed, callbacks, delayed prime and error paths passed");
}
