#include <crashtrace/crashtrace.hpp>

#include <array>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <limits.h>
#include <unistd.h>

#if defined(__GNUC__) || defined(__clang__)
#define BACKTRACE_DEMO_NOINLINE __attribute__((noinline))
#else
#define BACKTRACE_DEMO_NOINLINE
#endif

namespace {

const char* g_executable_path = nullptr;

const char* resolve_executable_path(const char* argv0) {
    static std::array<char, PATH_MAX> executable_path {};
    if (argv0 != nullptr && realpath(argv0, executable_path.data()) != nullptr) {
        return executable_path.data();
    }

    return argv0;
}

void SIGSEGV_handler(int signo, siginfo_t* info, void*) {
    crashtrace::DumpOptions options;
    options.skip_frames = 1;
    crashtrace::dump_debug_stack_symbols(stderr,
                                         g_executable_path,
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

int main(int argc, char** argv) {
    std::fprintf(stderr, "BTDEMO_START mode=debug_direct\n");
    g_executable_path = resolve_executable_path(argc > 0 ? argv[0] : nullptr);
    signal_handle_init();
    doSomething();
    return 0;
}
