#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#define _DARWIN_C_SOURCE 1

#include "util/diagnostic_reset.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void require(int ok, const char *message) {
    if (!ok) { fprintf(stderr, "diagnostic_reset: %s\n", message); exit(1); }
}

static void write_file(const char *path, const char *text) {
    FILE *file = fopen(path, "w");
    require(file != NULL && fputs(text, file) >= 0, "write fixture");
    require(fclose(file) == 0, "close fixture");
}

static void assert_text(const char *path, const char *expected) {
    FILE *file = fopen(path, "r");
    char text[256] = {0};
    require(file != NULL, "retained file exists");
    size_t n = fread(text, 1, sizeof(text) - 1, file);
    require(!ferror(file), "read retained file");
    text[n] = '\0';
    fclose(file);
    require(strcmp(text, expected) == 0, "retained file unchanged");
}

int main(void) {
    char root[] = "/tmp/aurora-reset-XXXXXX";
    require(mkdtemp(root) != NULL, "temporary directory");
    char persistent[512], temporary[512], marker[512], settings[1024], pairing[1024], unrelated[1024];
    snprintf(persistent, sizeof(persistent), "%s/conf", root);
    snprintf(temporary, sizeof(temporary), "%s/fallback", root);
    snprintf(marker, sizeof(marker), "%s/aurora-logs-reset.pending", root);
    snprintf(settings, sizeof(settings), "%s/settings.ini", persistent);
    snprintf(pairing, sizeof(pairing), "%s/client.key", persistent);
    snprintf(unrelated, sizeof(unrelated), "%s/unrelated.log", persistent);
    require(mkdir(persistent, 0755) == 0 && mkdir(temporary, 0755) == 0, "fixture directories");
    write_file(settings, "settings retained");
    write_file(pairing, "pairing retained");
    write_file(unrelated, "unrelated retained");
    const char *names[] = {"aurora.log", "aurora-crash.log", "aurora-run.state"};
    char logs[2][3][1024];
    for (unsigned d = 0; d < 2; d++) {
        for (unsigned i = 0; i < 3; i++) {
            snprintf(logs[d][i], sizeof(logs[d][i]), "%s/%s", d ? temporary : persistent, names[i]);
            write_file(logs[d][i], "old diagnostic");
        }
    }
    require(diagnostic_reset_apply(marker, persistent, temporary) == DIAGNOSTIC_RESET_NOT_NEEDED,
            "ordinary reopen without installation marker preserves logs");
    assert_text(logs[0][0], "old diagnostic");

    for (unsigned install = 0; install < 2; install++) {
        /* Restore the exact same marker bytes, as reinstalling one IPK does.
         * No version/build identity or changed timestamp is needed. */
        write_file(marker, "aurora-log-reset-v1\n");
        require(diagnostic_reset_apply(marker, persistent, temporary) == DIAGNOSTIC_RESET_DONE,
                "installation and identical-package reinstall clear diagnostics");
        require(access(marker, F_OK) != 0, "installation marker consumed");
        for (unsigned d = 0; d < 2; d++) {
            for (unsigned i = 0; i < 3; i++) {
                require(access(logs[d][i], F_OK) != 0, "clear each exact diagnostic in both directories");
                write_file(logs[d][i], "new diagnostic");
            }
        }
        assert_text(settings, "settings retained");
        assert_text(pairing, "pairing retained");
        assert_text(unrelated, "unrelated retained");
        require(diagnostic_reset_apply(marker, persistent, temporary) == DIAGNOSTIC_RESET_NOT_NEEDED,
                "reopen after reset preserves new logs including a crash report");
        assert_text(logs[0][1], "new diagnostic");
        assert_text(logs[1][0], "new diagnostic");
    }

    write_file(marker, "invalid marker\n");
    require(diagnostic_reset_apply(marker, persistent, temporary) == DIAGNOSTIC_RESET_FAILED,
            "refuse an invalid marker without deleting logs");
    assert_text(logs[0][0], "new diagnostic");
    unlink(marker);
    require(symlink(settings, marker) == 0, "marker symlink fixture");
    require(diagnostic_reset_apply(marker, persistent, temporary) == DIAGNOSTIC_RESET_FAILED,
            "refuse marker symlink");
    assert_text(settings, "settings retained");
    unlink(marker);

    require(unlink(logs[0][0]) == 0 && symlink(settings, logs[0][0]) == 0,
            "diagnostic symlink fixture");
    require(unlink(logs[0][1]) == 0 && link(pairing, logs[0][1]) == 0,
            "diagnostic hard-link fixture");
    write_file(marker, "aurora-log-reset-v1\n");
    require(diagnostic_reset_apply(marker, persistent, temporary) == DIAGNOSTIC_RESET_DONE,
            "unlink diagnostic links without touching targets");
    assert_text(settings, "settings retained");
    assert_text(pairing, "pairing retained");

    require(mkdir(logs[0][0], 0755) == 0, "unremovable diagnostic directory fixture");
    write_file(marker, "aurora-log-reset-v1\n");
    require(diagnostic_reset_apply(marker, persistent, temporary) == DIAGNOSTIC_RESET_FAILED && access(marker, F_OK) == 0,
            "failed cleanup retains marker for retry");
    require(rmdir(logs[0][0]) == 0, "remove cleanup obstruction");
    require(rmdir(temporary) == 0, "remove empty fallback");
    require(diagnostic_reset_apply(marker, persistent, temporary) == DIAGNOSTIC_RESET_DONE,
            "retry succeeds with missing fallback and missing diagnostic files");

    unlink(settings);
    unlink(pairing);
    unlink(unrelated);
    rmdir(persistent);
    rmdir(root);
    puts("diagnostic_reset: install/reinstall, reopen retention, exact-file cleanup, fallback, links and retry checks passed");
    return 0;
}
