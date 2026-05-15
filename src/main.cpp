#include <backtrace.h>
#include <ALog.h>
#include <cinttypes>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <unistd.h>

#define LOG_TAG "BACKTRACE_DEMO"

backtrace_state* g_backtrace_state = nullptr;

void handle_backtrace_error(void*, const char* message, int errnum) {
    if (errnum != 0) {
        LOGE(LOG_TAG, "libbacktrace error: %s: %s", message, std::strerror(errnum));
        return;
    }

    LOGE(LOG_TAG, "libbacktrace error: %s", message);
}

int handle_backtrace_frame(void*, uintptr_t pc, const char* filename, int line_number,
                           const char* function_name) {
    char location[4096];
    if (filename != nullptr) {
        if (line_number > 0) {
            std::snprintf(location, sizeof(location), "%s:%d", filename, line_number);
        } else {
            std::snprintf(location, sizeof(location), "%s", filename);
        }
    } else {
        std::snprintf(location, sizeof(location), "<unknown>");
    }

    LOGE(LOG_TAG, "  pc=0x%" PRIxPTR " function=%s at %s", pc,
         function_name != nullptr ? function_name : "<unknown>", location);
    return 0;
}

void print_backtrace() {
    LOGE(LOG_TAG, "==== libbacktrace stack ====");
    if (g_backtrace_state == nullptr) {
        LOGE(LOG_TAG, "libbacktrace state is not initialized");
        return;
    }

    backtrace_full(g_backtrace_state, 0,  handle_backtrace_frame, handle_backtrace_error,
                   nullptr);
    LOGE(LOG_TAG, "============================");
}

void trigger_segfault() {
    volatile std::uintptr_t address = 0;
    volatile int* bad_address = reinterpret_cast<volatile int*>(address);
    *bad_address = 1;
}

void SIGSEGV_handler(int signo) {
    (void)signo;
    print_backtrace();
    _exit(128 + SIGSEGV);
}

void signal_handle_init() {
    struct sigaction action {};
    sigemptyset(&action.sa_mask);
    action.sa_handler = SIGSEGV_handler;
    action.sa_flags = 0;
    sigaction(SIGSEGV, &action, nullptr);
}

void doSomething() {
    trigger_segfault();
}

int main(int argc, char** argv) {
    LOGI(LOG_TAG, "starting backtrace demo");

    const char* executable_path = argc > 0 ? argv[0] : nullptr;
    g_backtrace_state = backtrace_create_state(executable_path, 1, handle_backtrace_error, nullptr);
    if (g_backtrace_state == nullptr) {
        LOGE(LOG_TAG, "failed to initialize libbacktrace state");
        return 1;
    }

    signal_handle_init();
    LOGW(LOG_TAG, "triggering SIGSEGV to demonstrate libbacktrace logging");
    doSomething();
    return 0;
}
