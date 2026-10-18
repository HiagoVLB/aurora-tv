#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
#define _DARWIN_C_SOURCE 1
#endif

#include "file_log.h"

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef FILE_LOG_MAX_BYTES
#define FILE_LOG_MAX_BYTES (2u * 1024u * 1024u)
#endif
#define KEEP_BYTES (FILE_LOG_MAX_BYTES / 2u)
#define QUEUE_LINES 256u
#define LINE_BYTES 1024u

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ready = PTHREAD_COND_INITIALIZER;
static pthread_t writer;
static bool running, stopping;
static char queue[QUEUE_LINES][LINE_BYTES];
static unsigned head, count, dropped;
static FILE *output;
static size_t file_size;

/* Preserve complete recent lines in the same inode: no .1 or temporary files. */
static bool trim_file(void) {
    size_t keep = file_size < KEEP_BYTES ? file_size : KEEP_BYTES;
    char *tail = malloc(keep + 1);
    if (!tail) return false;
    size_t start = 0;
    bool ok = fseek(output, (long) (file_size - keep), SEEK_SET) == 0;
    if (ok) ok = fread(tail, 1, keep, output) == keep;
    if (ok && file_size > keep) {
        while (start < keep && tail[start] != '\n') start++;
        if (start < keep) start++;
    }
    if (ok) ok = fseek(output, 0, SEEK_SET) == 0;
    if (ok) ok = fwrite(tail + start, 1, keep - start, output) == keep - start;
    if (ok) ok = fflush(output) == 0;
    if (ok) ok = ftruncate(fileno(output), (off_t) (keep - start)) == 0;
    file_size = keep - start;
    free(tail);
    return ok && fseek(output, 0, SEEK_END) == 0;
}

static bool append_line(const char *line) {
    size_t len = strlen(line);
    if (file_size + len > FILE_LOG_MAX_BYTES && !trim_file()) return false;
    if (fwrite(line, 1, len, output) != len || fflush(output) != 0) return false;
    file_size += len;
    return true;
}

static void *write_loop(void *unused) {
    (void) unused;
    for (;;) {
        char line[LINE_BYTES];
        pthread_mutex_lock(&lock);
        while (!count && !dropped && !stopping) pthread_cond_wait(&ready, &lock);
        if (!count && !dropped && stopping) {
            pthread_mutex_unlock(&lock);
            break;
        }
        if (dropped) {
            snprintf(line, sizeof(line), "[FileLog] dropped %u log lines: queue full\n", dropped);
            dropped = 0;
        } else {
            memcpy(line, queue[head], sizeof(line));
            head = (head + 1) % QUEUE_LINES;
            count--;
        }
        pthread_mutex_unlock(&lock);
        if (output && !append_line(line)) {
            /* Never recurse into the logger from its writer. */
            fprintf(stderr, "Aurora: file logging disabled after write/trim failure\n");
            fclose(output);
            output = NULL;
        }
    }
    return NULL;
}

bool file_log_init(const char *path) {
    if (running) return true;
    int flags = O_RDWR | O_CREAT;
#ifdef O_NOFOLLOW
    flags |= O_NOFOLLOW;
#endif
    int fd = open(path, flags, 0644);
    if (fd < 0) return false;
    struct stat info;
    /* Dev Manager's SSH user may differ from the native app's owner. Only
     * this diagnostic file is readable by others; no group/other write access.
     * Apply to existing files too, so installing this build repairs mode 0600.
     * Refuse links before changing permissions on the opened descriptor. */
    if (fstat(fd, &info) != 0 || !S_ISREG(info.st_mode) || info.st_nlink != 1 ||
        fchmod(fd, 0644) != 0 ||
        fcntl(fd, F_SETFD, FD_CLOEXEC) != 0) {
        close(fd);
        return false;
    }
    output = fdopen(fd, "r+b");
    if (!output) {
        close(fd);
        return false;
    }
    long size = -1;
    if (fseek(output, 0, SEEK_END) == 0) size = ftell(output);
    file_size = size >= 0 ? (size_t) size : 0;
    if (size < 0 || (file_size > FILE_LOG_MAX_BYTES && !trim_file())) {
        fclose(output);
        output = NULL;
        return false;
    }
    head = count = dropped = 0;
    stopping = false;
    /* Embedded TVs need no multi-megabyte default stack for this simple worker. */
    pthread_attr_t attributes;
    int thread_ret = pthread_attr_init(&attributes);
    if (thread_ret == 0) {
        thread_ret = pthread_attr_setstacksize(&attributes, 128u * 1024u);
        if (thread_ret == 0) thread_ret = pthread_create(&writer, &attributes, write_loop, NULL);
        pthread_attr_destroy(&attributes);
    }
    if (thread_ret != 0) {
        fclose(output);
        output = NULL;
        return false;
    }
    pthread_mutex_lock(&lock);
    running = true;
    pthread_mutex_unlock(&lock);
    return true;
}

void file_log_deinit(void) {
    pthread_mutex_lock(&lock);
    bool join = running;
    running = false;
    stopping = true;
    pthread_cond_signal(&ready);
    pthread_mutex_unlock(&lock);
    if (!join) return;
    pthread_join(writer, NULL);
    if (output) fclose(output);
    output = NULL;
}

void file_log_on_log(commons_log_level level, const char *tag, const char *message) {
    /* DEBUG can contain request URLs/credentials and hex dumps. */
    if (level > COMMONS_LOG_LEVEL_INFO || !message) return;
    struct timespec wall, mono;
    clock_gettime(CLOCK_REALTIME, &wall);
    clock_gettime(CLOCK_MONOTONIC, &mono);
    struct tm utc;
    gmtime_r(&wall.tv_sec, &utc);
    char stamp[32], line[LINE_BYTES];
    strftime(stamp, sizeof(stamp), "%Y-%m-%dT%H:%M:%S", &utc);
    snprintf(line, sizeof(line), "%s.%03ldZ mono=%ld.%03ld %c [%.48s] %s",
             stamp, wall.tv_nsec / 1000000L, (long) mono.tv_sec, mono.tv_nsec / 1000000L,
             level >= COMMONS_LOG_LEVEL_FATAL && level <= COMMONS_LOG_LEVEL_INFO ? "FEWI"[level] : '?',
             tag ? tag : "?", message);
    size_t n = strlen(line);
    if (n > sizeof(line) - 2) n = sizeof(line) - 2;
    /* One record per line, including multiline protocol messages. */
    for (size_t i = 0; i < n; i++) {
        if ((unsigned char) line[i] < ' ') line[i] = ' ';
    }
    line[n++] = '\n';
    line[n] = '\0';
    pthread_mutex_lock(&lock);
    if (running) {
        if (count == QUEUE_LINES) {
            dropped++;
        } else {
            memcpy(queue[(head + count) % QUEUE_LINES], line, n + 1);
            count++;
        }
        pthread_cond_signal(&ready);
    }
    pthread_mutex_unlock(&lock);
}
