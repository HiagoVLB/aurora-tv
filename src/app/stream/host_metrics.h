#pragma once

#include <stdbool.h>
#include "client.h"

typedef struct app_t app_t;

/* Polls VibeShine hardware metrics (CPU/GPU/encoder) for the stats overlay.
 * Requests run only while the overlay keeps asking (host_metrics_snapshot is
 * called about once a second), never faster than the host interval, one at a
 * time, on a separate GS_CLIENT so ABR requests never wait behind them. The
 * host option is opt-in; after repeated failures (disabled, no permission,
 * busy) polling stops for the rest of the session. */
typedef struct host_metrics_service_t host_metrics_service_t;

host_metrics_service_t *host_metrics_start(app_t *app, const SERVER_DATA *server);

/* Returns immediately; the worker finishes its current request. Call exactly once,
 * also when polling already stopped on its own. */
void host_metrics_stop(host_metrics_service_t *service);

/* Latest reading. Also tells the poller the overlay is visible. Returns false when
 * no fresh reading exists (host option off, unsupported, or not yet sampled). */
bool host_metrics_snapshot(GS_HOST_METRICS *out);
