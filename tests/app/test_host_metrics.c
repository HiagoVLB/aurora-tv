/* Host metrics poller: only while the overlay asks, never faster than allowed,
 * stops after repeated failures, stop() never waits for the request. */
#define host_metrics_start t_host_metrics_start
#define host_metrics_stop t_host_metrics_stop
#define host_metrics_snapshot t_host_metrics_snapshot
#define gs_get_hardware_metrics t_get_metrics
#define app_gs_client_new t_client_new
#define gs_destroy t_destroy
#define gs_set_total_timeout t_set_timeout
#include "../../src/app/stream/host_metrics.c"
#include <assert.h>
#include <stdio.h>

static SDL_atomic_t requests, destroyed;
static int fail_mode;          /* 0 ok, 1 fail */
static int request_delay_ms;
static int host_interval_ms = 1000;
static char fake_client;

GS_CLIENT t_client_new(app_t *app) { (void) app; return (GS_CLIENT) &fake_client; }
void t_destroy(GS_CLIENT hnd) { (void) hnd; SDL_AtomicIncRef(&destroyed); }
void t_set_timeout(GS_CLIENT hnd, int secs) { (void) hnd; (void) secs; }
int t_get_metrics(GS_CLIENT hnd, const SERVER_DATA *server, GS_HOST_METRICS *out) {
    (void) hnd;
    assert(server->httpsPort == 47984 && strcmp(server->serverInfo.address, "pc") == 0);
    SDL_AtomicIncRef(&requests);
    if (request_delay_ms) SDL_Delay(request_delay_ms);
    if (fail_mode) return GS_FAILED;
    *out = (GS_HOST_METRICS) {.interval_ms = host_interval_ms, .cpu_usage_percent = 30, .gpu_usage_percent = 80,
                              .gpu_encoder_percent = 40, .gpu_temperature_c = 70, .gpu_vram_used_mb = 1000};
    return GS_OK;
}

static void keep_asking(Uint32 ms, GS_HOST_METRICS *last, bool *got) {
    Uint32 end = SDL_GetTicks() + ms;
    while (!SDL_TICKS_PASSED(SDL_GetTicks(), end)) {
        *got = t_host_metrics_snapshot(last) || *got;
        SDL_Delay(200);
    }
}

int main(void) {
    SDL_Init(SDL_INIT_TIMER);
    static app_t app;
    char address[] = "pc";
    SERVER_DATA server = {.httpsPort = 47984, .serverInfo.address = address};
    GS_HOST_METRICS m;
    bool got = false;

    /* Overlay hidden: no requests at all. */
    host_metrics_service_t *s = t_host_metrics_start(&app, &server);
    assert(s);
    SDL_Delay(1500);
    assert(SDL_AtomicGet(&requests) == 0);
    /* Overlay shown: first reading arrives; host asks 1 s but the floor is 2 s. */
    keep_asking(4500, &m, &got);
    assert(got && m.gpu_usage_percent == 80 && m.gpu_temperature_c == 70);
    int n = SDL_AtomicGet(&requests);
    assert(n >= 2 && n <= 3);
    /* Overlay closed: polling stops within the 3 s window. */
    SDL_Delay(3500);
    n = SDL_AtomicGet(&requests);
    SDL_Delay(2500);
    assert(SDL_AtomicGet(&requests) == n);
    /* stop() returns at once even with a slow request in flight; old data is gone. */
    request_delay_ms = 1500;
    t_host_metrics_snapshot(NULL);
    SDL_Delay(300);
    Uint32 t0 = SDL_GetTicks();
    t_host_metrics_stop(s);
    assert(SDL_GetTicks() - t0 < 50);
    assert(!t_host_metrics_snapshot(&m));
    SDL_Delay(2500);
    assert(SDL_AtomicGet(&destroyed) == 1); /* worker freed itself */
    request_delay_ms = 0;

    /* Host refuses (option off / no permission): three tries, then quiet. */
    SDL_AtomicSet(&requests, 0);
    fail_mode = 1;
    s = t_host_metrics_start(&app, &server);
    got = false;
    keep_asking(23000, &m, &got);
    assert(!got && SDL_AtomicGet(&requests) == 3);
    assert(SDL_AtomicGet(&destroyed) == 1); /* the session still owns it */
    t_host_metrics_stop(s);                 /* safe after the worker ended; frees it */
    assert(SDL_AtomicGet(&destroyed) == 2);
    puts("Host metrics poller: overlay-gated, 2 s floor, stops after 3 failures, non-blocking stop passed");
    return 0;
}
