#include "host_metrics.h"

#include "app.h"
#include "errors.h"
#include "logging.h"

#include <SDL.h>
#include <stdlib.h>
#include <string.h>

#define HOST_METRICS_MIN_INTERVAL_MS 2000
#define HOST_METRICS_RETRY_MS 10000
#define HOST_METRICS_MAX_FAILURES 3
#define HOST_METRICS_WANTED_WINDOW_MS 3000
#define HOST_METRICS_STALE_MS 10000

struct host_metrics_service_t {
    GS_CLIENT client;
    SERVER_DATA server;
    SDL_atomic_t stop;
    /* Owned by the session and by the worker; the last one to let go frees it. */
    SDL_atomic_t refs;
};

/* Shared with the UI thread. Only the newest service writes it. */
static SDL_SpinLock snapshot_lock;
static GS_HOST_METRICS snapshot;
static Uint32 snapshot_ticks;
static bool snapshot_valid;
static SDL_atomic_t wanted_ticks;
static SDL_atomic_t generation;

static void free_service(host_metrics_service_t *service) {
    if (service->client) gs_destroy(service->client);
    free((void *) service->server.serverInfo.address);
    free(service);
}

static void release_service(host_metrics_service_t *service) {
    if (SDL_AtomicDecRef(&service->refs)) {
        free_service(service);
    }
}

static bool wait_ms(host_metrics_service_t *service, Uint32 ms) {
    Uint32 end = SDL_GetTicks() + ms;
    while (!SDL_AtomicGet(&service->stop)) {
        if (SDL_TICKS_PASSED(SDL_GetTicks(), end)) return true;
        SDL_Delay(100);
    }
    return false;
}

static int host_metrics_thread(void *arg) {
    host_metrics_service_t *service = arg;
    int my_generation = SDL_AtomicGet(&generation);
    int failures = 0;
    int interval = HOST_METRICS_MIN_INTERVAL_MS;
    while (!SDL_AtomicGet(&service->stop)) {
        Uint32 wanted = (Uint32) SDL_AtomicGet(&wanted_ticks);
        if (wanted == 0 || SDL_TICKS_PASSED(SDL_GetTicks(), wanted + HOST_METRICS_WANTED_WINDOW_MS)) {
            if (!wait_ms(service, 250)) break;
            continue;
        }
        GS_HOST_METRICS metrics;
        int ret = gs_get_hardware_metrics(service->client, &service->server, &metrics);
        if (SDL_AtomicGet(&service->stop) || SDL_AtomicGet(&generation) != my_generation) break;
        if (ret == GS_OK) {
            failures = 0;
            interval = metrics.interval_ms > HOST_METRICS_MIN_INTERVAL_MS ? metrics.interval_ms
                                                                         : HOST_METRICS_MIN_INTERVAL_MS;
            SDL_AtomicLock(&snapshot_lock);
            snapshot = metrics;
            snapshot_ticks = SDL_GetTicks();
            snapshot_valid = true;
            SDL_AtomicUnlock(&snapshot_lock);
            if (!wait_ms(service, (Uint32) interval)) break;
        } else {
            if (++failures >= HOST_METRICS_MAX_FAILURES) {
                commons_log_info("HostMetrics", "Host metrics unavailable (ret=%d); stopped for this session", ret);
                break;
            }
            if (!wait_ms(service, HOST_METRICS_RETRY_MS)) break;
        }
    }
    release_service(service);
    return 0;
}

host_metrics_service_t *host_metrics_start(app_t *app, const SERVER_DATA *server) {
    if (!app || !server || !server->serverInfo.address || server->httpsPort == 0) return NULL;
    host_metrics_service_t *service = calloc(1, sizeof(*service));
    if (!service) return NULL;
    service->client = app_gs_client_new(app);
    service->server = *server;
    /* Only the address and HTTPS port are used; drop borrowed pointers. */
    service->server.uuid = NULL;
    service->server.mac = NULL;
    service->server.hostname = NULL;
    service->server.modes = NULL;
    service->server.serverInfo.address = strdup(server->serverInfo.address);
    if (!service->client || !service->server.serverInfo.address) {
        free_service(service);
        return NULL;
    }
    /* Host waits up to 2 s for a sample; never let a stuck request live long. */
    gs_set_total_timeout(service->client, 4);
    SDL_AtomicIncRef(&generation);
    SDL_AtomicLock(&snapshot_lock);
    snapshot_valid = false;
    SDL_AtomicUnlock(&snapshot_lock);
    SDL_AtomicSet(&wanted_ticks, 0);
    SDL_AtomicSet(&service->refs, 2);
    SDL_Thread *thread = SDL_CreateThread(host_metrics_thread, "host_metrics", service);
    if (!thread) {
        free_service(service);
        return NULL;
    }
    SDL_DetachThread(thread);
    return service;
}

void host_metrics_stop(host_metrics_service_t *service) {
    if (!service) return;
    SDL_AtomicSet(&service->stop, 1);
    SDL_AtomicLock(&snapshot_lock);
    snapshot_valid = false;
    SDL_AtomicUnlock(&snapshot_lock);
    release_service(service);
}

bool host_metrics_snapshot(GS_HOST_METRICS *out) {
    Uint32 now = SDL_GetTicks();
    SDL_AtomicSet(&wanted_ticks, (int) (now ? now : 1));
    SDL_AtomicLock(&snapshot_lock);
    bool ok = snapshot_valid && !SDL_TICKS_PASSED(now, snapshot_ticks + HOST_METRICS_STALE_MS);
    if (ok && out) *out = snapshot;
    SDL_AtomicUnlock(&snapshot_lock);
    return ok;
}
