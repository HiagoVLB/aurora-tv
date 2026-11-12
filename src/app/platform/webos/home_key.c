/* Remote Home during a stream: disconnect first, then open the LG home.
 * The TV sends Aurora no background event when its home screen opens over the
 * app, so a stream left running behind it came back with a corrupted picture. */
#include "app.h"
#include "app_webos.h"
#include "stream/session.h"
#include "logging.h"
#include "lvgl.h"

/* Longest wait for the stream to close before going home anyway. Teardown
 * normally takes well under a second. */
#define HOME_AFTER_DISCONNECT_TIMEOUT_MS 3000

static bool home_after_disconnect = false;
static lv_timer_t *home_fallback_timer = NULL;

static void home_now(const char *why) {
    home_after_disconnect = false;
    if (home_fallback_timer != NULL) {
        lv_timer_del(home_fallback_timer);
        home_fallback_timer = NULL;
    }
    commons_log_info("APP", "Home: opening LG home (%s)", why);
    app_webos_open_ribbon();
}

static void home_fallback_cb(lv_timer_t *timer) {
    LV_UNUSED(timer);
    /* One-shot: LVGL deletes the timer after this call. */
    home_fallback_timer = NULL;
    home_now("stream close timed out");
}

static void home_interrupt_async(void *data) {
    LV_UNUSED(data);
    if (!home_after_disconnect) {
        return;
    }
    if (global != NULL && global->session != NULL) {
        session_interrupt(global->session, false, STREAMING_INTERRUPT_BACKGROUND);
    } else {
        home_now("stream already closed");
    }
}

void app_webos_go_home(session_t *session) {
    if (session == NULL) {
        app_webos_open_ribbon();
        return;
    }
    if (home_after_disconnect) {
        return;
    }
    commons_log_info("APP", "Home: disconnecting stream before LG home");
    home_after_disconnect = true;
    home_fallback_timer = lv_timer_create(home_fallback_cb, HOME_AFTER_DISCONNECT_TIMEOUT_MS, NULL);
    lv_timer_set_repeat_count(home_fallback_timer, 1);
    /* Interrupt on the next UI tick, outside the key handler (like the other
     * remote buttons, which go through the event bus). */
    lv_async_call(home_interrupt_async, NULL);
}

void app_webos_session_finished(void) {
    if (home_after_disconnect) {
        home_now("stream closed");
    }
}
