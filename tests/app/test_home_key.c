/* Remote Home during a stream: disconnect first, open the LG home once after the
 * stream finished, with a 3 s fallback. Real LVGL timers, stubbed luna/session. */
#define app_webos_open_ribbon test_open_ribbon
#define session_interrupt test_session_interrupt
#include "../../src/app/platform/webos/home_key.c"
#include <assert.h>
#include <stdio.h>

static int ribbon_count, interrupt_count;
static streaming_interrupt_reason_t last_reason;
static bool last_quitapp = true;

void test_open_ribbon(void) { ribbon_count++; }

void test_session_interrupt(session_t *session, bool quitapp, streaming_interrupt_reason_t reason) {
    (void) session;
    interrupt_count++;
    last_quitapp = quitapp;
    last_reason = reason;
}

static void pump(Uint32 ms) {
    Uint32 end = SDL_GetTicks() + ms;
    do {
        lv_timer_handler();
        SDL_Delay(5);
    } while (!SDL_TICKS_PASSED(SDL_GetTicks(), end));
}

static void reset(void) { ribbon_count = 0; interrupt_count = 0; last_quitapp = true; }

int main(void) {
    SDL_Init(SDL_INIT_TIMER);
    lv_init();
    static app_t app = {0};
    static char session_storage[64];
    session_t *session = (session_t *) session_storage; /* opaque; only compared */
    global = &app;

    /* No stream: LG home at once, nothing interrupted. */
    reset();
    app_webos_go_home(NULL);
    assert(ribbon_count == 1 && interrupt_count == 0);

    /* Stream: nothing synchronous inside the key handler. */
    reset();
    app.session = session;
    app_webos_go_home(session);
    assert(ribbon_count == 0 && interrupt_count == 0);
    pump(50);
    /* Next UI tick: disconnect (game keeps running), LG home not yet. */
    assert(interrupt_count == 1 && !last_quitapp && last_reason == STREAMING_INTERRUPT_BACKGROUND);
    assert(ribbon_count == 0);
    /* A second Home while waiting changes nothing. */
    app_webos_go_home(session);
    pump(50);
    assert(interrupt_count == 1 && ribbon_count == 0);
    /* Stream finished: LG home exactly once, fallback cancelled. */
    app.session = NULL;
    app_webos_session_finished();
    assert(ribbon_count == 1);
    pump(3300);
    assert(ribbon_count == 1);
    /* A later, unrelated stream end does not open the LG home. */
    app_webos_session_finished();
    assert(ribbon_count == 1);

    /* Teardown never finishes: fallback opens the LG home after ~3 s, once. */
    reset();
    app.session = session;
    app_webos_go_home(session);
    pump(2700);
    assert(interrupt_count == 1 && ribbon_count == 0);
    pump(600);
    assert(ribbon_count == 1);
    app_webos_session_finished();
    pump(100);
    assert(ribbon_count == 1);

    /* Stream gone before the deferred interrupt ran: LG home without waiting. */
    reset();
    app.session = session;
    app_webos_go_home(session);
    app.session = NULL;
    pump(50);
    assert(interrupt_count == 0 && ribbon_count == 1);
    pump(3300);
    assert(ribbon_count == 1);

    puts("Home key: disconnect first, LG home once after stream end or 3 s fallback passed");
    return 0;
}
