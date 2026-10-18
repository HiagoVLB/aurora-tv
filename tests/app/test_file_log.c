#define _DARWIN_C_SOURCE 1

/* Standalone POSIX test: no SDL, TV or logging backend required. */
#include "util/file_log.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void require(bool ok, const char *message) {
    if (!ok) { fprintf(stderr, "%s\n", message); exit(1); }
}

static char *read_log(const char *path) {
    FILE *f = fopen(path, "rb");
    require(f != NULL, "open log");
    require(fseek(f, 0, SEEK_END) == 0, "seek");
    long len = ftell(f);
    require(len >= 0 && len <= 4096, "hard size limit");
    rewind(f);
    char *text = calloc((size_t) len + 1, 1);
    require(text != NULL, "allocate");
    require(fread(text, 1, (size_t) len, f) == (size_t) len, "read");
    fclose(f);
    if (len) require(text[len - 1] == '\n', "complete final line");
    return text;
}

static void *producer(void *unused) {
    (void) unused;
    for (int i = 0; i < 10000; i++) file_log_on_log(COMMONS_LOG_LEVEL_INFO, "Thread", "concurrent record");
    return NULL;
}

int main(void) {
    char dir[] = "/tmp/aurora-file-log-XXXXXX", path[256], extra[256];
    require(mkdtemp(dir) != NULL, "temporary directory");
    snprintf(path, sizeof(path), "%s/aurora.log", dir);
    require(!file_log_init("/nonexistent-aurora-directory/aurora.log"), "invalid directory fails safely");
    mode_t old_umask = umask(0077);
    require(file_log_init(path), "initialize");
    umask(old_umask);
    file_log_on_log(COMMONS_LOG_LEVEL_INFO, "APP", "first session");
    file_log_on_log(COMMONS_LOG_LEVEL_DEBUG, "HTTP", "SECRET_REQUEST_URL");
    file_log_deinit();
    struct stat initial, current;
    require(stat(path, &initial) == 0, "stat");
    require((initial.st_mode & 0777) == 0644, "readable diagnostic log despite restrictive umask");
    require(chmod(path, 0600) == 0, "simulate previous build permissions");
    require(file_log_init(path), "restart");
    require(stat(path, &current) == 0 && (current.st_mode & 0777) == 0644,
            "repair existing log permissions");
    file_log_on_log(COMMONS_LOG_LEVEL_WARN, "APP", "second session\nmultiline");
    file_log_deinit();
    char *text = read_log(path);
    require(strstr(text, "first session") && strstr(text, "second session multiline"), "preserve sessions and flatten multiline");
    require(!strstr(text, "SECRET_REQUEST_URL"), "exclude debug credentials");
    require(strstr(text, "Z mono="), "wall and monotonic timestamps");
    free(text);

    char message[800];
    memset(message, 'x', sizeof(message) - 1);
    message[sizeof(message) - 1] = 0;
    for (int batch = 0; batch < 8; batch++) {
        require(file_log_init(path), "reopen for rollover");
        for (int i = 0; i < 32; i++) file_log_on_log(COMMONS_LOG_LEVEL_INFO, "Fill", message);
        file_log_deinit();
        text = read_log(path);
        require(strstr(text, "[Fill]"), "retain recent records");
        free(text);
        require(stat(path, &current) == 0 && initial.st_ino == current.st_ino, "same file inode after rollover");
    }
    require(file_log_init(path), "initialize concurrency test");
    pthread_t threads[4];
    for (int i = 0; i < 4; i++) require(pthread_create(&threads[i], NULL, producer, NULL) == 0, "start producer");
    for (int i = 0; i < 4; i++) pthread_join(threads[i], NULL);
    file_log_deinit();
    require(file_log_init(path), "reopen after saturation");
    file_log_on_log(COMMONS_LOG_LEVEL_ERROR, "APP", "FINAL_MARKER");
    file_log_deinit();
    text = read_log(path);
    require(strstr(text, "FINAL_MARKER") != NULL, "writer recovers after saturation");
    for (char *line = strtok(text, "\n"); line; line = strtok(NULL, "\n")) {
        require(strncmp(line, "[FileLog] dropped ", 18) == 0 ||
                (strncmp(line, "20", 2) == 0 && strstr(line, "Z mono=")), "records are not interleaved");
    }
    free(text);

    FILE *f = fopen(path, "ab");
    require(f != NULL, "seed oversized log");
    for (int i = 0; i < 16; i++) fprintf(f, "%s\n", message);
    fclose(f);
    require(file_log_init(path), "trim oversized log at startup");
    file_log_on_log(COMMONS_LOG_LEVEL_INFO, "APP", "AFTER_OVERSIZE");
    file_log_deinit();
    text = read_log(path);
    require(strstr(text, "AFTER_OVERSIZE") != NULL, "append after oversized recovery");
    free(text);
    snprintf(extra, sizeof(extra), "%s/aurora.log.1", dir);
    require(access(extra, F_OK) != 0, "no rotated backup");
    unlink(path);
    char private_path[256];
    snprintf(private_path, sizeof(private_path), "%s/private", dir);
    f = fopen(private_path, "wb");
    require(f != NULL, "create private target");
    fputs("private data", f);
    fclose(f);
    require(chmod(private_path, 0600) == 0, "private target permissions");
    require(symlink(private_path, path) == 0, "create symlink");
    require(!file_log_init(path), "refuse symlink target");
    require(stat(private_path, &current) == 0 && (current.st_mode & 0777) == 0600,
            "symlink target permissions untouched");
    unlink(path);
    require(link(private_path, path) == 0, "create hardlink");
    require(!file_log_init(path), "refuse shared inode");
    require(stat(private_path, &current) == 0 && (current.st_mode & 0777) == 0600,
            "hardlink target permissions untouched");
    unlink(path);
    unlink(private_path);
    require(rmdir(dir) == 0, "no other log files created");
    puts("file_log: size, rollover, restart, concurrency, overflow recovery and privacy checks passed");
    return 0;
}
