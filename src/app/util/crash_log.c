#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
#define _DARWIN_C_SOURCE 1
#endif

#include "crash_log.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/syscall.h>
#include <ucontext.h>
#endif

#define REPORT_MAX (64u * 1024u)
#define MAPS_READ_MAX (1024u * 1024u)
#define STATE_BYTES 512u

static const int fatal_signals[] = {SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT, SIGTRAP};
static int report_fd = -1, state_fd = -1;
static char identity[256], previous[STATE_BYTES];
static unsigned char signal_stack[64u * 1024u];
static atomic_flag handling = ATOMIC_FLAG_INIT;
static bool initialized;

/* The signal path uses only fixed buffers and async-signal-safe operations.
 * In particular: no logger, stdio, allocation, unwinder, mutex or SDL calls. */
static bool write_all(int fd, const char *data, size_t size) {
    while (size) {
        ssize_t n = write(fd, data, size);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        data += n;
        size -= (size_t) n;
    }
    return true;
}

static char *add_text(char *out, const char *text) {
    while (*text) *out++ = *text++;
    return out;
}

static char *add_number(char *out, uint64_t number, unsigned base) {
    char digits[32];
    unsigned n = 0;
    do {
        digits[n++] = "0123456789abcdef"[number % base];
        number /= base;
    } while (number);
    while (n) *out++ = digits[--n];
    return out;
}

static char *add_signed(char *out, int number) {
    if (number < 0) *out++ = '-';
    return add_number(out, number < 0 ? (uint64_t) -(int64_t) number : (uint64_t) number, 10);
}

/* Only executable mappings: enough to locate PC/LR in a module, without dumping
 * stack/heap contents. Read and output limits also bound pathological maps. */
static void write_code_maps(size_t budget) {
#ifdef __linux__
    int fd = open("/proc/self/maps", O_RDONLY | O_CLOEXEC);
    if (fd < 0) return;
    char chunk[1024], line[1024];
    size_t used = 0, seen = 0;
    bool overflow = false;
    ssize_t count;
    while (seen < MAPS_READ_MAX && (count = read(fd, chunk, sizeof(chunk))) > 0) {
        seen += (size_t) count;
        for (ssize_t i = 0; i < count; i++) {
            if (used < sizeof(line)) line[used++] = chunk[i];
            else overflow = true;
            if (chunk[i] != '\n') continue;
            bool executable = false;
            for (size_t j = 0; j + 3 < used; j++) {
                if (line[j] == ' ') {
                    executable = line[j + 3] == 'x';
                    break;
                }
            }
            if (!overflow && executable) {
                if (used > budget || !write_all(report_fd, line, used)) goto finish;
                budget -= used;
            }
            used = 0;
            overflow = false;
        }
    }
finish:
    close(fd);
#else
    (void) budget;
#endif
}

static void fatal_handler(int signal_number, siginfo_t *info, void *context) {
    /* A second crashing thread must not truncate the first thread's report.
     * The first handler re-raises its signal to terminate the whole process. */
    if (atomic_flag_test_and_set_explicit(&handling, memory_order_relaxed)) {
        for (;;) pause();
    }
    char record[1024];
    char *out = add_text(record, "Aurora fatal diagnostic v1\n");
    out = add_text(out, identity);
    struct timespec wall = {0}, mono = {0};
    clock_gettime(CLOCK_REALTIME, &wall);
    clock_gettime(CLOCK_MONOTONIC, &mono);
    out = add_text(out, "utc_epoch_s=");
    out = add_number(out, (uint64_t) wall.tv_sec, 10);
    out = add_text(out, " monotonic_s=");
    out = add_number(out, (uint64_t) mono.tv_sec, 10);
    out = add_text(out, " signal=");
    out = add_signed(out, signal_number);
    out = add_text(out, " signal_name=");
    switch (signal_number) {
        case SIGSEGV: out = add_text(out, "SIGSEGV"); break;
        case SIGBUS: out = add_text(out, "SIGBUS"); break;
        case SIGILL: out = add_text(out, "SIGILL"); break;
        case SIGFPE: out = add_text(out, "SIGFPE"); break;
        case SIGABRT: out = add_text(out, "SIGABRT"); break;
        case SIGTRAP: out = add_text(out, "SIGTRAP"); break;
    }
    out = add_text(out, " si_code=");
    out = add_signed(out, info ? info->si_code : 0);
#if defined(__linux__) && defined(SYS_gettid)
    out = add_text(out, " tid=");
    out = add_number(out, (uint64_t) syscall(SYS_gettid), 10);
#endif
    if (info && info->si_code > 0 && signal_number != SIGABRT) {
        out = add_text(out, " fault_address=0x");
        out = add_number(out, (uintptr_t) info->si_addr, 16);
    }
    uintptr_t pc = 0, sp = 0, lr = 0;
#if defined(__linux__) && defined(__arm__)
    ucontext_t *uc = context;
    if (uc) {
        pc = uc->uc_mcontext.arm_pc;
        sp = uc->uc_mcontext.arm_sp;
        lr = uc->uc_mcontext.arm_lr;
    }
#elif defined(__linux__) && defined(__aarch64__)
    ucontext_t *uc = context;
    if (uc) {
        pc = uc->uc_mcontext.pc;
        sp = uc->uc_mcontext.sp;
        lr = uc->uc_mcontext.regs[30];
    }
#elif defined(__linux__) && defined(__x86_64__)
    ucontext_t *uc = context;
    if (uc) {
        pc = (uintptr_t) uc->uc_mcontext.gregs[REG_RIP];
        sp = (uintptr_t) uc->uc_mcontext.gregs[REG_RSP];
    }
#else
    (void) context;
#endif
    out = add_text(out, "\npc=0x");
    out = add_number(out, pc, 16);
    out = add_text(out, " sp=0x");
    out = add_number(out, sp, 16);
    out = add_text(out, " lr=0x");
    out = add_number(out, lr, 16);
    out = add_text(out, "\nExecutable mappings (start-end permissions file-offset device inode path):\n");
    if (ftruncate(report_fd, 0) == 0 && lseek(report_fd, 0, SEEK_SET) >= 0 &&
        write_all(report_fd, record, (size_t) (out - record))) {
        write_code_maps(REPORT_MAX - (size_t) (out - record));
        fsync(report_fd);
    }
    /* Keep the run marked abnormal even if it crashed during final exit. */
    if (pwrite(state_fd, "crashed", 7, 0) == 7) fsync(state_fd);

    struct sigaction action = {0};
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    sigaction(signal_number, &action, NULL);
    sigset_t unblocked;
    sigemptyset(&unblocked);
    sigaddset(&unblocked, signal_number);
    sigprocmask(SIG_UNBLOCK, &unblocked, NULL);
    raise(signal_number);
    _exit(128 + signal_number);
}

static int open_diagnostic(const char *directory, const char *name, size_t cap) {
    char path[4096];
    int n = snprintf(path, sizeof(path), "%s/%s", directory, name);
    if (n < 0 || (size_t) n >= sizeof(path)) return -1;
    int flags = O_RDWR | O_CREAT | O_CLOEXEC | O_NONBLOCK;
#ifdef O_NOFOLLOW
    flags |= O_NOFOLLOW;
#endif
    int fd = open(path, flags, 0644);
    if (fd < 0) return -1;
    struct stat st;
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_nlink != 1 ||
        fchmod(fd, 0644) != 0 || (st.st_size > (off_t) cap && ftruncate(fd, (off_t) cap) != 0)) {
        close(fd);
        return -1;
    }
    return fd;
}

bool crash_log_init(const char *directory, const char *version) {
    if (initialized) return true;
    if (!directory || !version) return false;
    report_fd = open_diagnostic(directory, "aurora-crash.log", REPORT_MAX);
    state_fd = open_diagnostic(directory, "aurora-run.state", STATE_BYTES - 1);
    if (report_fd < 0 || state_fd < 0) goto fail;
    ssize_t n = pread(state_fd, previous, sizeof(previous) - 1, 0);
    if (n < 0) goto fail;
    previous[n] = '\0';
    /* State is diagnostic input: keep it one printable line in the normal log. */
    for (ssize_t i = 0; i < n; i++) {
        if ((unsigned char) previous[i] < ' ' || previous[i] == 127) previous[i] = ' ';
    }
    char boot_id[64] = "unknown";
#ifdef __linux__
    int boot = open("/proc/sys/kernel/random/boot_id", O_RDONLY | O_CLOEXEC);
    if (boot >= 0) {
        ssize_t size = read(boot, boot_id, sizeof(boot_id) - 1);
        if (size > 0) {
            boot_id[size] = '\0';
            boot_id[strcspn(boot_id, "\n")] = '\0';
        }
        close(boot);
    }
#endif
    snprintf(identity, sizeof(identity), "version=%.48s pid=%ld started_utc_epoch_s=%lld boot_id=%.63s\n",
             version, (long) getpid(), (long long) time(NULL), boot_id);
    char state[STATE_BYTES];
    int length = snprintf(state, sizeof(state), "running %s", identity);
    if (pwrite(state_fd, state, (size_t) length, 0) != length ||
        ftruncate(state_fd, length) != 0 || fsync(state_fd) != 0) goto fail;

    stack_t old_stack, alternate = {.ss_sp = signal_stack, .ss_size = sizeof(signal_stack)};
    if (sigaltstack(&alternate, &old_stack) != 0) goto fail;
    struct sigaction action = {0}, old_actions[sizeof(fatal_signals) / sizeof(fatal_signals[0])];
    action.sa_sigaction = fatal_handler;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&action.sa_mask);
    for (size_t i = 0; i < sizeof(fatal_signals) / sizeof(fatal_signals[0]); i++) {
        sigaddset(&action.sa_mask, fatal_signals[i]);
    }
    size_t installed = 0;
    for (; installed < sizeof(fatal_signals) / sizeof(fatal_signals[0]); installed++) {
        if (sigaction(fatal_signals[installed], &action, &old_actions[installed]) != 0) {
            while (installed) {
                installed--;
                sigaction(fatal_signals[installed], &old_actions[installed], NULL);
            }
            sigaltstack(&old_stack, NULL);
            goto fail;
        }
    }
    /* Descriptors and main-thread alternate stack stay valid until process exit,
     * including crashes after normal app teardown. Other threads use their own
     * regular stack; stack overflow on those threads is not guaranteed capturable. */
    initialized = true;
    return true;
fail:
    if (report_fd >= 0) close(report_fd);
    if (state_fd >= 0) close(state_fd);
    report_fd = state_fd = -1;
    return false;
}

const char *crash_log_previous_run(void) {
    return previous;
}

bool crash_log_mark_clean(void) {
    return initialized && pwrite(state_fd, "clean  ", 7, 0) == 7 && fsync(state_fd) == 0;
}
