#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
#define _DARWIN_C_SOURCE 1
#endif

#include "diagnostic_reset.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static bool clear_directory(const char *directory) {
    int fd = open(directory, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return errno == ENOENT;
    static const char *const diagnostics[] = {"aurora.log", "aurora-crash.log", "aurora-run.state"};
    bool ok = true;
    for (unsigned i = 0; i < sizeof(diagnostics) / sizeof(diagnostics[0]); i++) {
        /* Unlink exact names, without following symlinks or truncating a shared
         * inode. Settings, pairing files, and unrelated logs are never visited. */
        if (unlinkat(fd, diagnostics[i], 0) != 0 && errno != ENOENT) ok = false;
    }
    close(fd);
    return ok;
}

diagnostic_reset_result_t diagnostic_reset_apply(const char *marker_path,
                                                const char *persistent_directory,
                                                const char *temporary_directory) {
    if (!marker_path || !persistent_directory || !temporary_directory) return DIAGNOSTIC_RESET_FAILED;
    int fd = open(marker_path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return errno == ENOENT ? DIAGNOSTIC_RESET_NOT_NEEDED : DIAGNOSTIC_RESET_FAILED;
    struct stat st;
    char marker[32];
    static const char expected[] = "aurora-log-reset-v1\n";
    bool valid = fstat(fd, &st) == 0 && S_ISREG(st.st_mode) && st.st_nlink == 1 &&
                 st.st_size == sizeof(expected) - 1 &&
                 read(fd, marker, sizeof(marker)) == sizeof(expected) - 1 &&
                 memcmp(marker, expected, sizeof(expected) - 1) == 0;
    close(fd);
    if (!valid) return DIAGNOSTIC_RESET_FAILED;

    /* A failed cleanup keeps the marker so the next launch can retry. Consume
     * it before starting either writer, so a later crash is preserved on reopen. */
    bool cleared = clear_directory(persistent_directory);
    if (strcmp(persistent_directory, temporary_directory) != 0 && !clear_directory(temporary_directory)) {
        cleared = false;
    }
    if (!cleared || unlink(marker_path) != 0) return DIAGNOSTIC_RESET_FAILED;
    return DIAGNOSTIC_RESET_DONE;
}
