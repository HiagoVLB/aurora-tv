#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#if defined(__linux__) && !defined(_FILE_OFFSET_BITS)
#define _FILE_OFFSET_BITS 64
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
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/syscall.h>
#include <ucontext.h>
#endif
#if defined(CRASH_LOG_UNWIND) && defined(__linux__)
#include <dlfcn.h>
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#define CRASH_STRING_IMPL(value) #value
#define CRASH_STRING(value) CRASH_STRING_IMPL(value)
#endif

#define REPORT_MAX (64u * 1024u)
#define MAPS_READ_MAX (1024u * 1024u)
#define STATE_BYTES 512u
#define TRACE_FRAMES 32u
#define TRACE_RESERVE 4096u

static const int fatal_signals[] = {SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT, SIGTRAP};
static int report_fd = -1, state_fd = -1;
static char identity[256], previous[STATE_BYTES];
static unsigned char signal_stack[64u * 1024u];
static atomic_flag handling = ATOMIC_FLAG_INIT;
static bool initialized;
static bool trace_available;
#if defined(__linux__) && defined(SYS_gettid)
static volatile sig_atomic_t trace_owner, trace_signal;
#endif
static int memory_fd = -1;
static uintptr_t abort_message_slot;

static _Noreturn void terminate_by_signal(int signal_number) {
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

#if defined(CRASH_LOG_UNWIND) && defined(__linux__)
static int (*trace_init)(unw_cursor_t *, unw_context_t *, int);
static int (*trace_reg)(unw_cursor_t *, int, unw_word_t *);
static int (*trace_step)(unw_cursor_t *);

static unw_context_t *unwind_context(void *context, unw_context_t *adapted) {
#if defined(__arm__)
    /* ARM libunwind uses its own GP-register context, NOT Linux ucontext_t.
     * Passing the sigaction pointer directly would unwind unrelated memory. */
    const ucontext_t *uc = context;
    adapted->regs[0] = uc->uc_mcontext.arm_r0;
    adapted->regs[1] = uc->uc_mcontext.arm_r1;
    adapted->regs[2] = uc->uc_mcontext.arm_r2;
    adapted->regs[3] = uc->uc_mcontext.arm_r3;
    adapted->regs[4] = uc->uc_mcontext.arm_r4;
    adapted->regs[5] = uc->uc_mcontext.arm_r5;
    adapted->regs[6] = uc->uc_mcontext.arm_r6;
    adapted->regs[7] = uc->uc_mcontext.arm_r7;
    adapted->regs[8] = uc->uc_mcontext.arm_r8;
    adapted->regs[9] = uc->uc_mcontext.arm_r9;
    adapted->regs[10] = uc->uc_mcontext.arm_r10;
    adapted->regs[11] = uc->uc_mcontext.arm_fp;
    adapted->regs[12] = uc->uc_mcontext.arm_ip;
    adapted->regs[13] = uc->uc_mcontext.arm_sp;
    adapted->regs[14] = uc->uc_mcontext.arm_lr;
    adapted->regs[15] = uc->uc_mcontext.arm_pc;
    return adapted;
#else
    (void) adapted;
    return (unw_context_t *) context;
#endif
}

static void prepare_backtrace(void) {
    /* Optional runtime probing keeps older firmware usable. Resolve/warm all
     * entry points during startup, never dlopen/dlsym in the handler.
     * Keep a successful library handle loaded for the process lifetime. */
    /* Optional glibc fatal reason (assertion/allocator/fortify), read through a
     * preopened /proc fd to avoid dereferencing a corrupted private pointer. */
    abort_message_slot = (uintptr_t) dlsym(RTLD_DEFAULT, "__abort_msg");
    if (abort_message_slot) memory_fd = open("/proc/self/mem", O_RDONLY | O_CLOEXEC);
#if defined(__arm__)
    /* Metadata only: old libunwind's APCS frame heuristic is unsafe for EABI.
     * A missing table should stop the trace instead of guessing a frame chain. */
    setenv("UNW_ARM_UNWIND_METHOD", "5", 1); /* DWARF | EXIDX, no APCS heuristic */
#endif
    void *library = dlopen("libunwind.so.8", RTLD_NOW | RTLD_LOCAL);
    if (!library) return;
    trace_init = (int (*)(unw_cursor_t *, unw_context_t *, int)) dlsym(library, CRASH_STRING(unw_init_local2));
    trace_reg = (int (*)(unw_cursor_t *, int, unw_word_t *)) dlsym(library, CRASH_STRING(unw_get_reg));
    trace_step = (int (*)(unw_cursor_t *)) dlsym(library, CRASH_STRING(unw_step));
    unw_context_t context = {0};
    unw_cursor_t cursor;
    /* ARM libc getcontext() may be a failing stub, and older libunwind's ARM
     * inline getcontext miscompiles with newer GCC. Warm cursor initialization
     * and register access only, using a valid local PC/stack address; never
     * walk this synthetic context. Real faults use all GP registers from the
     * kernel signal frame, converted above. */
#if defined(__arm__)
    context.regs[15] = (uintptr_t) prepare_backtrace & ~(uintptr_t) 1;
    context.regs[13] = (uintptr_t) &context;
    int context_result = 0;
#else
    int context_result = getcontext((ucontext_t *) &context);
#endif
    if (trace_init && trace_reg && trace_step && context_result == 0 &&
        trace_init(&cursor, &context, 0) == 0) {
        unw_word_t ip;
        trace_available = trace_reg(&cursor, UNW_REG_IP, &ip) == 0;
    }
    if (!trace_available) dlclose(library);
}
#else
static void prepare_backtrace(void) {}
#endif

/* The signal path uses only fixed buffers and async-signal-safe operations.
 * No logger, stdio, allocation, symbol lookup, mutex or SDL calls. The optional
 * local libunwind entry points used below are documented signal-safe. */
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

static void write_abort_message(int signal_number) {
    if (signal_number != SIGABRT || memory_fd < 0 || !abort_message_slot) return;
    uintptr_t message = 0;
    unsigned size = 0;
    if (pread(memory_fd, &message, sizeof(message), (off_t) abort_message_slot) != sizeof(message) || !message ||
        pread(memory_fd, &size, sizeof(size), (off_t) message) != sizeof(size) || size <= sizeof(size)) return;
    char text[512];
    size_t count = size - sizeof(size);
    if (count > sizeof(text)) count = sizeof(text);
    ssize_t length = pread(memory_fd, text, count, (off_t) message + sizeof(size));
    if (length <= 0) return;
    char line[560];
    char *end = add_text(line, "libc_abort_message=");
    for (ssize_t i = 0; i < length && text[i]; i++) {
        *end++ = text[i] >= ' ' && text[i] < 127 ? text[i] : ' ';
    }
    *end++ = '\n';
    write_all(report_fd, line, (size_t) (end - line));
}

static void write_backtrace(void *context) {
    static const char unavailable[] = "Backtrace unavailable (local unwinder not initialized)\n";
#if defined(CRASH_LOG_UNWIND) && defined(__linux__)
    if (trace_available && context) {
        unw_cursor_t cursor;
        unw_context_t adapted = {0};
        int result = trace_init(&cursor, unwind_context(context, &adapted), UNW_INIT_SIGNAL_FRAME);
        static const char heading[] = "Backtrace (instruction addresses, newest first; max=32):\n";
        if (!write_all(report_fd, heading, sizeof(heading) - 1)) return;
        unw_word_t previous_ip = 0, previous_sp = 0;
        for (unsigned frame = 0; result >= 0 && frame < TRACE_FRAMES; frame++) {
            unw_word_t ip = 0, sp = 0;
            if ((result = trace_reg(&cursor, UNW_REG_IP, &ip)) < 0 ||
                (result = trace_reg(&cursor, UNW_REG_SP, &sp)) < 0 || !ip) break;
            if (frame && ip == previous_ip && sp == previous_sp) break;
            char line[128];
            char *out = add_text(line, "frame=");
            out = add_number(out, frame, 10);
            out = add_text(out, " ip=0x");
            out = add_number(out, (uint64_t) ip, 16);
            out = add_text(out, " sp=0x");
            out = add_number(out, (uint64_t) sp, 16);
            *out++ = '\n';
            if (!write_all(report_fd, line, (size_t) (out - line))) return;
            previous_ip = ip;
            previous_sp = sp;
            result = trace_step(&cursor);
            if (result <= 0) break;
        }
        char status[64];
        char *end = add_text(status, "backtrace_stop=");
        end = add_signed(end, result);
        *end++ = '\n';
        write_all(report_fd, status, (size_t) (end - status));
        return;
    }
#else
    (void) context;
#endif
    write_all(report_fd, unavailable, sizeof(unavailable) - 1);
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
#if defined(__linux__) && defined(SYS_gettid)
        if (trace_owner == syscall(SYS_gettid) && trace_signal) {
            char line[128];
            char *end = add_text(line, "backtrace_fault_signal=");
            end = add_signed(end, signal_number);
#if defined(__arm__)
            end = add_text(end, " pc=0x");
            end = add_number(end, ((ucontext_t *) context)->uc_mcontext.arm_pc, 16);
#endif
            *end++ = '\n';
            write_all(report_fd, line, (size_t) (end - line));
            fsync(report_fd);
            terminate_by_signal(trace_signal);
        }
#endif
        for (;;) pause();
    }
    char record[1024];
    char *out = add_text(record, "Aurora fatal diagnostic v2\n");
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
    if (info && info->si_code <= 0) {
        out = add_text(out, " sender_pid=");
        out = add_number(out, (uint64_t) info->si_pid, 10);
    }
#if defined(__linux__) && defined(SYS_gettid)
    out = add_text(out, " tid=");
    out = add_number(out, (uint64_t) syscall(SYS_gettid), 10);
#endif
#ifdef __linux__
    int thread = open("/proc/thread-self/comm", O_RDONLY | O_CLOEXEC);
    if (thread >= 0) {
        char name[32];
        ssize_t length = read(thread, name, sizeof(name));
        close(thread);
        if (length > 0) {
            out = add_text(out, " thread_name=");
            for (ssize_t i = 0; i < length && name[i] != '\n'; i++) {
                *out++ = name[i] >= ' ' && name[i] < 127 ? name[i] : '?';
            }
        }
    }
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
        write_abort_message(signal_number);
        write_code_maps(REPORT_MAX - (size_t) (out - record) - TRACE_RESERVE - 560u);
        /* Preserve basic evidence before attempting a best-effort stack walk. */
        fsync(report_fd);
        if (pwrite(state_fd, "crashed", 7, 0) == 7) fsync(state_fd);
#if defined(__linux__) && defined(SYS_gettid)
        if (trace_available) {
            trace_signal = signal_number;
            trace_owner = (sig_atomic_t) syscall(SYS_gettid);
            sigset_t walk_faults;
            sigemptyset(&walk_faults);
            sigaddset(&walk_faults, SIGSEGV);
            sigaddset(&walk_faults, SIGBUS);
            sigprocmask(SIG_UNBLOCK, &walk_faults, NULL);
        }
#endif
        write_backtrace(context);
        fsync(report_fd);
    }
    /* Keep the run marked abnormal even if it crashed during final exit. */
    if (pwrite(state_fd, "crashed", 7, 0) == 7) fsync(state_fd);

    terminate_by_signal(signal_number);
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
    /* Basic capture is already armed if the optional firmware unwinder fails
     * during its startup probe; trace_available stays false until warmed. */
    prepare_backtrace();
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

bool crash_log_backtrace_available(void) {
    return trace_available;
}
