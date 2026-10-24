#include "stream/video/idr_refresh.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>

static void test_variable_frame_rate(void) {
    idr_refresh_t state = {0};
    assert(!idr_refresh_due(&state, 100, true, 2000));
    /* A 90 FPS session whose host becomes idle and sends only 10 FPS must
     * still request at two seconds, rather than waiting for 180 frames. */
    for (uint32_t now = 200; now < 2100; now += 100) {
        assert(!idr_refresh_due(&state, now, false, 2000));
    }
    assert(idr_refresh_due(&state, 2100, false, 2000));
    assert(!idr_refresh_due(&state, 2101, false, 2000));
    assert(!idr_refresh_due(&state, 4099, false, 2000));
    assert(idr_refresh_due(&state, 4100, false, 2000));
}

static void test_idr_restart_and_pause(void) {
    idr_refresh_t state = {0};
    assert(!idr_refresh_due(&state, 500, false, 2000));
    assert(!idr_refresh_due(&state, 2000, true, 2000));
    assert(!idr_refresh_due(&state, 3999, false, 2000));
    assert(idr_refresh_due(&state, 4000, false, 2000));
    /* A long gap causes one request on the next accepted frame, never a burst. */
    assert(idr_refresh_due(&state, 30000, false, 2000));
    assert(!idr_refresh_due(&state, 30001, false, 2000));
    state = (idr_refresh_t) {0};
    assert(!idr_refresh_due(&state, 30002, false, 2000));
}

static void test_disabled_and_wrap(void) {
    idr_refresh_t state = {0};
    assert(!idr_refresh_due(&state, 0, true, 0));
    assert(!idr_refresh_due(&state, 10000, false, 0));
    assert(!idr_refresh_due(&state, 20000, false, -1));
    assert(!idr_refresh_due(&state, 30000, false, 499));
    state = (idr_refresh_t) {0};
    assert(!idr_refresh_due(&state, UINT32_MAX - 999, true, 2000));
    assert(!idr_refresh_due(&state, 999, false, 2000));
    assert(idr_refresh_due(&state, 1000, false, 2000));
    /* Corrupt/large INI values do not multiply FPS or overflow signed int. */
    state = (idr_refresh_t) {0};
    assert(!idr_refresh_due(&state, 0, true, INT_MAX));
    assert(!idr_refresh_due(&state, INT_MAX - 1, false, INT_MAX));
    assert(idr_refresh_due(&state, INT_MAX, false, INT_MAX));
}

int main(void) {
    test_variable_frame_rate();
    test_idr_restart_and_pause();
    test_disabled_and_wrap();
    puts("HEVC periodic refresh elapsed-time, pause, reset and clock-wrap checks passed");
    return 0;
}
