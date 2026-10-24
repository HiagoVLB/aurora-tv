/* Deterministic host/clock fixtures around the actual ABR worker and policy. */
#include "../../src/app/stream/adaptive_bitrate.c"
#include <assert.h>
#include <stdio.h>

static Uint32 ticks;
static int sets, applied_override, set_result, delays, stop_after;
static bool repeat_sample, empty_sample;
static adaptive_bitrate_service_t *running;
Uint32 SDL_GetTicks(void) { return ticks; }
void SDL_Delay(Uint32 ms) {
    ticks += ms;
    if (++delays == stop_after && running) SDL_AtomicSet(&running->stop, 1);
}
void vdec_stats_snapshot(struct VIDEO_STATS *out) {
    memset(out, 0, sizeof(*out));
    if (empty_sample) return;
    out->totalFrames = 120;
    out->measurementEndTimestamp = repeat_sample ? 1 : ticks;
}
bool LiGetEstimatedRttInfo(uint32_t *rtt, uint32_t *variance) {
    (void)variance; *rtt = 10; return true;
}
int gs_set_bitrate_ex(GS_CLIENT c, const SERVER_DATA *s, int kbps, int *applied) {
    (void)c; (void)s; sets++;
    *applied = applied_override ? applied_override : kbps;
    return set_result;
}
int gs_get_abr_capabilities(GS_CLIENT c, const SERVER_DATA *s, GS_ABR_CAPABILITIES *caps) {
    (void)c; (void)s; memset(caps, 0, sizeof(*caps)); return GS_OK;
}
int gs_set_abr_mode(GS_CLIENT c, const SERVER_DATA *s, const GS_ABR_CONFIG *config) {
    (void)c; (void)s; (void)config; return GS_OK;
}
int gs_report_abr_feedback(GS_CLIENT c, const SERVER_DATA *s, const GS_ABR_FEEDBACK *f, GS_ABR_ACTION *a) {
    (void)c; (void)s; (void)f; memset(a, 0, sizeof(*a)); return GS_OK;
}
void gs_set_total_timeout(GS_CLIENT c, int seconds) { (void)c; (void)seconds; }
void commons_log_printf(commons_log_level level, const char *tag, const char *fmt, ...) {
    (void)level; (void)tag; (void)fmt;
}
static adaptive_bitrate_service_t service(void) {
    adaptive_bitrate_service_t s = { .initial_bitrate = 50000, .current_bitrate = 50000 };
    abr_apply_mode_preset(&s);
    return s;
}
static void run_samples(bool repeat, bool empty) {
    adaptive_bitrate_service_t s = service();
    running = &s; ticks = 0; delays = 0; sets = 0; stop_after = 16;
    repeat_sample = repeat; empty_sample = empty;
    abr_thread(&s);
    if (repeat || empty) assert(sets == 0 && s.current_bitrate == 50000);
    else assert(sets > 0 && s.current_bitrate > 50000);
    running = NULL;
}
int main(void) {
    assert(adaptive_bitrate_start(NULL) == NULL);
    adaptive_bitrate_service_t s = service();
    ticks = 10000;
    abr_tick_local(&s, 10.0f);
    assert(s.current_bitrate == 35000);
    applied_override = 20000;
    assert(abr_set_bitrate(&s, 40000, "test"));
    assert(s.current_bitrate == 20000 && s.max_bitrate == 20000 && s.min_bitrate <= 20000);
    applied_override = 0;
    set_result = GS_IO_ERROR;
    ticks = UINT32_MAX - 1000;
    assert(!abr_set_bitrate(&s, 15000, "failure"));
    int before = sets;
    ticks = UINT32_MAX - 500;
    assert(!abr_set_bitrate(&s, 15000, "backoff") && sets == before);
    ticks = 1001;
    set_result = GS_OK;
    assert(abr_set_bitrate(&s, 15000, "after wrap"));
    s = service();
    ticks = 10000;
    adaptive_bitrate_request_drop(&s, 75, 15000, "test");
    abr_apply_pending_drop(&s);
    assert(s.current_bitrate == 37500);
    before = sets;
    abr_tick_local(&s, 10.0f);
    assert(sets == before);
    run_samples(true, false);
    run_samples(false, true);
    run_samples(false, false);
    assert(abr_scale(INT_MAX, 105, 100) == INT_MAX);
    s = service(); s.initial_bitrate = 1000; s.mode = ABR_MODE_QUALITY;
    abr_apply_mode_preset(&s);
    assert(s.min_bitrate <= s.max_bitrate);
    puts("ABR loss, host clamp, clock wrap, recovery hold and stale sample checks passed");
    return 0;
}
