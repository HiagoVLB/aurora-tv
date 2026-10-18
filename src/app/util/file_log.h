#pragma once

#include <stdbool.h>
#include "logging.h"

/* One persistent file, trimmed in place; producers never perform disk I/O. */
bool file_log_init(const char *path);
void file_log_deinit(void);
void file_log_on_log(commons_log_level level, const char *tag, const char *message);
