#include <crashtrace/crashtrace.hpp>

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <unistd.h>

#if defined(__GNUC__) || defined(__clang__)
#define BACKTRACE_DEMO_NOINLINE __attribute__((noinline))
#else
#define BACKTRACE_DEMO_NOINLINE
#endif

namespace {

constexpr const char* LOG_TAG = "crashtrace_release_demo";

void SIGSEGV_handler(int signo, siginfo_t* info, void*) {
    crashtrace::DumpOptions options;
    options.skip_frames = 1;
    crashtrace::dump_release_stack_addresses(stderr,
                                             signo,
                                             info != nullptr ? info->si_addr : nullptr,
                                             options);
    _exit(128 + SIGSEGV);
}

void signal_handle_init() {
    struct sigaction action {};
    sigemptyset(&action.sa_mask);
    action.sa_sigaction = SIGSEGV_handler;
    action.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &action, nullptr);
}

BACKTRACE_DEMO_NOINLINE void trigger_segfault() {
    volatile std::uintptr_t address = 0;
    volatile int* bad_address = reinterpret_cast<volatile int*>(address);
    *bad_address = 1;
}

BACKTRACE_DEMO_NOINLINE void doSomething() {
    trigger_segfault();
}

}  // namespace

int main() {
    std::fprintf(stderr, "%s: START mode=release_collector\n", LOG_TAG);
    signal_handle_init();
    doSomething();
    return 0;
}
