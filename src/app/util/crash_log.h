#pragma once

#include <stdbool.h>

/* Process lifetime, initialized once on the main thread. No SDL dependency.
 * Keeps the last fatal report across clean restarts. No full memory dumps. */
bool crash_log_init(const char *directory, const char *version);
const char *crash_log_previous_run(void);
bool crash_log_mark_clean(void);
bool crash_log_backtrace_available(void);
