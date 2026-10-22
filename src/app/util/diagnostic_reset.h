#pragma once

typedef enum {
    DIAGNOSTIC_RESET_FAILED = -1,
    DIAGNOSTIC_RESET_NOT_NEEDED = 0,
    DIAGNOSTIC_RESET_DONE = 1,
} diagnostic_reset_result_t;

/* Before opening any diagnostic writer. The IPK restores the pending marker on
 * every install, including a reinstall of the identical package/version. */
diagnostic_reset_result_t diagnostic_reset_apply(const char *marker_path,
                                                const char *persistent_directory,
                                                const char *temporary_directory);
