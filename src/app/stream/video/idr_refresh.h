#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t reference_ticks;
    bool initialized;
} idr_refresh_t;

/* Called only after an accepted HEVC frame. Requests use elapsed monotonic
 * milliseconds, including when the host temporarily sends fewer frames. */
bool idr_refresh_due(idr_refresh_t *state, uint32_t now, bool is_idr, int interval_ms);
