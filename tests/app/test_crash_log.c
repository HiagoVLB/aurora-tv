#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#define _DARWIN_C_SOURCE 1

/* Real child-process faults: a handler that only formats a mock record would
 * not verify signal termination, worker threads, retention or SIGKILL gaps. */
#include "util/crash_log.h"

#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef CRASH_LOG_TEST_FAULTY_UNWIND
#define UNW_LOCAL_ONLY
#include <libunwind.h>
extern void *__real_dlsym(void *handle, const char *name);
static int faulty_unwind_step(unw_cursor_t *cursor) {
    (void) cursor;
    raise(SIGSEGV);
    return -1;
}
void *__wrap_dlsym(void *handle, const char *name) {
    if (strstr(name, "_step")) return (void *) faulty_unwind_step;
    return __real_dlsym(handle, name);
}
#endif

static void require(bool ok, const char *message) {
    if (!ok) { fprintf(stderr, "crash_log: %s\n", message); exit(1); }
}

static char *read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    require(file != NULL, "open diagnostic");
    require(fseek(file, 0, SEEK_END) == 0, "seek diagnostic");
    long size = ftell(file);
    require(size >= 0 && size <= 65536, "bounded diagnostic size");
    rewind(file);
    char *text = calloc((size_t) size + 1, 1);
    require(text != NULL, "allocate read buffer");
    require(fread(text, 1, (size_t) size, file) == (size_t) size, "read diagnostic");
    fclose(file);
    return text;
}

static void *faulting_worker(void *unused) {
    (void) unused;
    /* A real protected-page fault supplies si_addr and a useful instruction PC. */
    void *page = mmap(NULL, 4096, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (page == MAP_FAILED) _exit(80);
    *(volatile unsigned char *) page = 1;
    _exit(81);
}

enum mode { CLEAN = 1000, UNCLEAN, MAIN_FAULT, WORKER_FAULT, FAIL_INIT, ABORT_CHAIN, ASSERT_CHAIN };

#ifdef CRASH_LOG_TEST_NO_UNWIND
void *__wrap_dlopen(const char *path, int flags) {
    (void) path;
    (void) flags;
    return NULL;
}
#endif

__attribute__((noinline)) static void diagnostic_abort_leaf(void) {
    abort();
}

__attribute__((noinline)) static void diagnostic_abort_parent(void) {
    diagnostic_abort_leaf();
}

__attribute__((noinline)) static void diagnostic_assert_leaf(void) {
    assert(0 && "diagnostic assertion probe");
}

#if defined(CRASH_LOG_UNWIND) && !defined(CRASH_LOG_TEST_NO_UNWIND)
static void check_backtrace(const char *text, bool want_caller) {
    require(strstr(text, "Backtrace (instruction addresses") != NULL, "backtrace heading");
    const char *first = strstr(text, "frame=0 ip=0x");
    unsigned long long frame_pc, signal_pc;
    require(first && sscanf(first, "frame=0 ip=0x%llx", &frame_pc) == 1 &&
            sscanf(strstr(text, "\npc=0x"), "\npc=0x%llx", &signal_pc) == 1 && frame_pc == signal_pc,
            "unwind starts at original fault PC, not handler or incompatible context");
    unsigned frames = 0;
    bool found_caller = false;
    for (const char *line = first; (line = strstr(line, "frame=")); line++) {
        unsigned frame;
        unsigned long long ip;
        require(sscanf(line, "frame=%u ip=0x%llx", &frame, &ip) == 2, "parse backtrace frame");
        frames++;
        uintptr_t begin = (uintptr_t) diagnostic_abort_leaf & ~(uintptr_t) 1;
        if ((ip & ~1ull) >= begin && (ip & ~1ull) < begin + 64) found_caller = true;
    }
    require(frames > 0 && frames <= 32, "bounded frame count");
#ifdef CRASH_LOG_TEST_FAULTY_UNWIND
    (void) found_caller;
    (void) want_caller;
    require(strstr(text, "backtrace_fault_signal=") != NULL, "unwinder fault preserves evidence and original signal");
#else
    if (want_caller) {
#ifdef __arm__
        (void) found_caller;
        /* Some ARM libc abort implementations have no usable unwind table.
         * Metadata-only walking must stop cleanly rather than guess APCS. */
        require(frames >= 3 && !strstr(text, "backtrace_fault_signal="), "ARM abort trace stops cleanly at missing metadata");
#else
        require(found_caller && frames >= 3, "trace passes libc abort and reaches application caller");
#endif
    }
#endif
}
#endif

#ifdef __APPLE__
#define PAGE_FAULT_SIGNAL SIGBUS
#else
#define PAGE_FAULT_SIGNAL SIGSEGV
#endif

static void launch(const char *dir, int mode, const char *expected_previous) {
    pid_t child = fork();
    require(child >= 0, "fork");
    if (!child) {
        struct rlimit disabled = {0, 0};
        setrlimit(RLIMIT_CORE, &disabled);
        umask(0077);
        bool armed = crash_log_init(dir, "diagnostic-test");
        if (mode == FAIL_INIT) _exit(armed ? 82 : 0);
        if (!armed) _exit(83);
#if defined(CRASH_LOG_UNWIND) && !defined(CRASH_LOG_TEST_NO_UNWIND)
        if (!crash_log_backtrace_available()) _exit(89);
#else
        if (crash_log_backtrace_available()) _exit(90);
#endif
        if (expected_previous && strncmp(crash_log_previous_run(), expected_previous,
                                         strlen(expected_previous)) != 0) _exit(84);
        if (mode == CLEAN) _exit(crash_log_mark_clean() ? 0 : 85);
        if (mode == UNCLEAN) _exit(0);
        if (mode == WORKER_FAULT) {
            pthread_t worker;
            if (pthread_create(&worker, NULL, faulting_worker, NULL) != 0) _exit(86);
            pthread_join(worker, NULL);
            _exit(87);
        }
        if (mode == MAIN_FAULT) faulting_worker(NULL);
        if (mode == ABORT_CHAIN) diagnostic_abort_parent();
        if (mode == ASSERT_CHAIN) diagnostic_assert_leaf();
        raise(mode);
        _exit(88);
    }
    int status;
    require(waitpid(child, &status, 0) == child, "wait for child");
    if (mode == CLEAN || mode == UNCLEAN || mode == FAIL_INIT) {
        if (!(WIFEXITED(status) && WEXITSTATUS(status) == 0)) {
            fprintf(stderr, "child lifecycle mode=%d wait_status=%d\n", mode, status);
        }
        require(WIFEXITED(status) && WEXITSTATUS(status) == 0, "child lifecycle checks");
    } else {
        int expected = mode == WORKER_FAULT || mode == MAIN_FAULT ? PAGE_FAULT_SIGNAL :
                       mode == ABORT_CHAIN || mode == ASSERT_CHAIN ? SIGABRT : mode;
        if (!(WIFSIGNALED(status) && WTERMSIG(status) == expected)) {
            fprintf(stderr, "child mode=%d wait_status=%d\n", mode, status);
        }
        require(WIFSIGNALED(status) && WTERMSIG(status) == expected,
                "fatal handler must preserve signal termination");
    }
}

int main(void) {
    char directory[] = "/tmp/aurora-crash-log-XXXXXX";
    require(mkdtemp(directory) != NULL, "temporary directory");
    char report[512], state[512], other[512];
    snprintf(report, sizeof(report), "%s/aurora-crash.log", directory);
    snprintf(state, sizeof(state), "%s/aurora-run.state", directory);
    snprintf(other, sizeof(other), "%s/untouched", directory);

    launch("/nonexistent-aurora-directory", FAIL_INIT, NULL);
    launch(directory, CLEAN, "");
    struct stat st;
    require(stat(state, &st) == 0 && (st.st_mode & 0777) == 0644, "state downloadable with restrictive umask");
    char *text = read_file(state);
    require(strncmp(text, "clean  ", 7) == 0 && strstr(text, "version=diagnostic-test"), "completed run marked clean");
    free(text);

    launch(directory, UNCLEAN, "clean  ");
    launch(directory, CLEAN, "running");
    launch(directory, SIGABRT, "clean  ");
    text = read_file(report);
    require(strstr(text, "Aurora fatal diagnostic v2") && strstr(text, "version=diagnostic-test") &&
            strstr(text, "utc_epoch_s=") && strstr(text, "si_code="), "fatal report contains identity and signal context");
#ifdef __linux__
    require(strstr(text, "pc=0x0 ") == NULL && strstr(text, "r-x"), "Linux fault PC and executable mappings captured");
    require(strstr(text, "sender_pid=") && strstr(text, "thread_name="), "software signal sender and thread name");
#endif
#if defined(CRASH_LOG_UNWIND) && !defined(CRASH_LOG_TEST_NO_UNWIND)
    check_backtrace(text, false);
#else
    require(strstr(text, "Backtrace unavailable") != NULL, "unwinder failure retains basic report");
#endif
    require(stat(report, &st) == 0 && (st.st_mode & 0777) == 0644, "fatal report downloadable");
    launch(directory, CLEAN, "crashed");

    char *retained = read_file(report);
    require(strcmp(text, retained) == 0, "clean restart preserves last crash");
    free(text);
    free(retained);

    launch(directory, ASSERT_CHAIN, "clean  ");
    text = read_file(report);
#if defined(CRASH_LOG_UNWIND) && defined(__linux__)
    require(strstr(text, "libc_abort_message=") && strstr(text, "diagnostic assertion probe"),
            "glibc assertion reason survives even when the abort stack is partial");
#endif
    free(text);
    launch(directory, CLEAN, "crashed");

    const int signals[] = {SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGTRAP};
    for (size_t i = 0; i < sizeof(signals) / sizeof(signals[0]); i++) {
        launch(directory, signals[i], i ? "crashed" : "clean  ");
        text = read_file(report);
        char expected[32];
        snprintf(expected, sizeof(expected), "signal=%d ", signals[i]);
        require(strstr(text, expected) != NULL, "fatal signal written");
        free(text);
    }
    launch(directory, MAIN_FAULT, "crashed");
    text = read_file(report);
    require(strstr(text, "fault_address=0x") != NULL, "main-thread hardware fault captured");
#if defined(CRASH_LOG_UNWIND) && !defined(CRASH_LOG_TEST_NO_UNWIND)
    check_backtrace(text, false);
#endif
    free(text);
    launch(directory, WORKER_FAULT, "crashed");
    text = read_file(report);
    require(strstr(text, "fault_address=0x") != NULL, "worker-thread hardware fault captured");
#if defined(CRASH_LOG_UNWIND) && !defined(CRASH_LOG_TEST_NO_UNWIND)
    check_backtrace(text, false);
#endif
    launch(directory, SIGKILL, "crashed");
    retained = read_file(report);
    require(strcmp(text, retained) == 0, "SIGKILL cannot invent a fatal report");
    free(text);
    free(retained);
    launch(directory, CLEAN, "running");

    launch(directory, ABORT_CHAIN, "clean  ");
    text = read_file(report);
#if defined(CRASH_LOG_UNWIND) && !defined(CRASH_LOG_TEST_NO_UNWIND)
    check_backtrace(text, true);
#endif
    free(text);
    launch(directory, CLEAN, "crashed");

    require(chmod(report, 0600) == 0 && chmod(state, 0600) == 0, "simulate restrictive old diagnostic permissions");
    int fd = open(report, O_WRONLY);
    require(fd >= 0 && ftruncate(fd, 128 * 1024) == 0, "create oversized prior report");
    close(fd);
    launch(directory, CLEAN, "clean  ");
    require(stat(report, &st) == 0 && st.st_size == 65536 && (st.st_mode & 0777) == 0644,
            "bound existing report and repair export permissions");

    require(unlink(report) == 0, "remove report for link checks");
    fd = open(other, O_CREAT | O_WRONLY, 0600);
    require(fd >= 0, "create protected target");
    close(fd);
    require(symlink(other, report) == 0, "create symlink");
    launch(directory, FAIL_INIT, NULL);
    require(stat(other, &st) == 0 && (st.st_mode & 0777) == 0600, "symlink target permissions preserved");
    require(unlink(report) == 0 && link(other, report) == 0, "create hard link");
    launch(directory, FAIL_INIT, NULL);
    require(stat(other, &st) == 0 && st.st_size == 0 && (st.st_mode & 0777) == 0600,
            "hard-link target contents and permissions preserved");

    unlink(report);
    unlink(state);
    unlink(other);
    rmdir(directory);
    puts("crash_log: real fatal signals, worker fault, clean/unclean runs, SIGKILL, retention, bounds and export checks passed");
    return 0;
}
