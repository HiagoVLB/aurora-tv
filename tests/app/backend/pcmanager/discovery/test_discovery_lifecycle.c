/* Real lifecycle with deterministic workers: failed mDNS and an active callback. */
#include "backend/pcmanager/discovery/discovery.h"
#include "backend/pcmanager/discovery/impl/impl.h"
#include "logging.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static SDL_atomic_t mode, alive, throttle_alive, callbacks;
static SDL_sem *exit_gate;
void commons_log_printf(commons_log_level level, const char *tag, const char *fmt, ...) {
    (void) level; (void) tag; (void) fmt;
}
void discovery_throttle_init(discovery_throttle_t *throttle, discovery_callback callback, void *data) {
    memset(throttle, 0, sizeof(*throttle));
    throttle->callback = callback; throttle->user_data = data;
    SDL_AtomicSet(&throttle_alive, 1);
}
void discovery_throttle_deinit(discovery_throttle_t *throttle) {
    (void) throttle;
    assert(SDL_AtomicGet(&alive) == 0);
    SDL_AtomicSet(&throttle_alive, 0);
}
void discovery_throttle_on_discovered(discovery_throttle_t *throttle, const sockaddr_t *addr, Uint32 ttl) {
    (void) throttle; (void) addr; (void) ttl;
    assert(SDL_AtomicGet(&throttle_alive));
    SDL_AtomicAdd(&callbacks, 1);
}
void discovery_worker_stop(discovery_task_t *task) {
    SDL_LockMutex(task->lock);
    task->stop = true;
    SDL_UnlockMutex(task->lock);
}
int discovery_worker(discovery_task_t *task) {
    SDL_AtomicAdd(&alive, 1);
    if (SDL_AtomicGet(&mode) == 0) {
        SDL_AtomicAdd(&alive, -1);
        return -1; /* e.g. no multicast interface, as in Docker --network none. */
    }
    for (;;) {
        SDL_LockMutex(task->lock);
        bool stopped = task->stop;
        SDL_UnlockMutex(task->lock);
        if (stopped) break;
        SDL_Delay(1);
    }
    SDL_SemWait(exit_gate);
    /* An already-dispatched callback must finish before throttle destruction. */
    discovery_discovered(task->discovery, NULL);
    SDL_AtomicAdd(&alive, -1);
    return 0;
}
static void wait_atomic(SDL_atomic_t *value, int expected) {
    Uint32 deadline = SDL_GetTicks() + 3000;
    while (SDL_AtomicGet(value) != expected && !SDL_TICKS_PASSED(SDL_GetTicks(), deadline)) SDL_Delay(1);
    assert(SDL_AtomicGet(value) == expected);
}
int main(void) {
    assert(SDL_Init(0) == 0);
    discovery_t discovery;
    discovery_init(&discovery, NULL, NULL);
    /* Stopping after spontaneous worker exit used to lock a freed mutex. */
    for (unsigned i = 0; i < 100; i++) {
        discovery_start(&discovery);
        assert(discovery.task != NULL);
        wait_atomic(&discovery.task->completed, 1);
        discovery_stop(&discovery);
        discovery_stop(&discovery);
    }
    discovery_deinit(&discovery);
    SDL_AtomicSet(&mode, 1);
    discovery_init(&discovery, NULL, NULL);
    exit_gate = SDL_CreateSemaphore(0);
    discovery_start(&discovery);
    wait_atomic(&alive, 1);
    discovery_task_t *first = discovery.task;
    discovery_start(&discovery);
    assert(discovery.task == first); /* Start is idempotent while running. */
    discovery_stop(&discovery); /* Must return before gate opens, never join here. */
    assert(SDL_AtomicGet(&alive) == 1);
    SDL_SemPost(exit_gate);
    discovery_deinit(&discovery); /* Joins before releasing callback dependencies. */
    assert(SDL_AtomicGet(&callbacks) == 1 && SDL_AtomicGet(&alive) == 0);
    SDL_DestroySemaphore(exit_gate);
    discovery_init(&discovery, NULL, NULL);
    discovery_deinit(&discovery); /* No thread was ever started. */
    SDL_Quit();
    puts("discovery lifecycle tests passed");
}
