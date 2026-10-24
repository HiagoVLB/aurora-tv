#include "idr_refresh.h"

bool idr_refresh_due(idr_refresh_t *state, uint32_t now, bool is_idr, int interval_ms) {
    if (!state->initialized || is_idr || interval_ms < 500) {
        state->reference_ticks = now;
        state->initialized = true;
        return false;
    }
    if ((uint32_t) (now - state->reference_ticks) < (uint32_t) interval_ms) {
        return false;
    }
    /* Bound repeated requests even when the host does not answer with an IDR. */
    state->reference_ticks = now;
    return true;
}
