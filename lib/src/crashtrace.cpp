#include <crashtrace/crashtrace.hpp>

#include <backtrace.h>
#if defined(__APPLE__)
#include <unwind.h>
#else
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#endif

#include <algorithm>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <string>
#include <vector>
#include <unistd.h>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

#if defined(__GNUC__) || defined(__clang__)
#include <cxxabi.h>
#endif

namespace crashtrace {
namespace {

struct ModuleAddress {
    uintptr_t pc = 0;
    uintptr_t module_base = 0;
    uintptr_t module_offset = 0;
    uintptr_t object_pc = 0;
    const char* module_path = "<unknown>";
};

struct FrameRecord {
    std::size_t index = 0;
    uintptr_t object_pc = 0;
    std::string module;
};

struct ResolveContext {
    std::FILE* output = nullptr;
    std::size_t index = 0;
    uintptr_t object_pc = 0;
    bool callback_called = false;
};

constexpr const char* kCollectorLogTag = "crashtrace_collector";
constexpr const char* kDebugLogTag = "crashtrace_debug";
constexpr const char* kFrameEvent = "FRAME";

void backtrace_error_callback(void*, const char* message, int errnum) {
    if (errnum != 0) {
        std::fprintf(stderr, "libbacktrace error: %s: %s\n", message, std::strerror(errnum));
        return;
    }

    std::fprintf(stderr, "libbacktrace error: %s\n", message);
}

std::string demangle(const char* function_name) {
    if (function_name == nullptr) {
        return "<unknown>";
    }

#if defined(__GNUC__) || defined(__clang__)
    int status = 0;
    char* demangled = abi::__cxa_demangle(function_name, nullptr, nullptr, &status);
    if (status == 0 && demangled != nullptr) {
        std::string result(demangled);
        std::free(demangled);
        return result;
    }
    std::free(demangled);
#endif

    return function_name;
}

const char* normalize_filename(const char* filename) {
    if (filename == nullptr) {
        return nullptr;
    }

    const char* duplicated_separator = std::strstr(filename, "//");
    if (duplicated_separator != nullptr && duplicated_separator != filename &&
        duplicated_separator[2] != '\0') {
        return duplicated_separator + 1;
    }

    return filename;
}

#if defined(__APPLE__)
uintptr_t find_mach_o_slide(const void* image_header) {
    const auto image_count = _dyld_image_count();
    for (uint32_t i = 0; i < image_count; ++i) {
        if (reinterpret_cast<const void*>(_dyld_get_image_header(i)) == image_header) {
            return static_cast<uintptr_t>(_dyld_get_image_vmaddr_slide(i));
        }
    }

    return 0;
}
#endif

ModuleAddress resolve_module_address(uintptr_t pc) {
    ModuleAddress result;
    result.pc = pc;

    Dl_info info {};
    if (dladdr(reinterpret_cast<void*>(pc), &info) == 0) {
        result.object_pc = pc;
        return result;
    }

    result.module_base = reinterpret_cast<uintptr_t>(info.dli_fbase);
    result.module_offset = result.module_base > 0 ? pc - result.module_base : 0;
    result.module_path = info.dli_fname != nullptr ? info.dli_fname : "<unknown>";

#if defined(__APPLE__)
    const uintptr_t slide = find_mach_o_slide(info.dli_fbase);
    result.object_pc = slide > 0 ? pc - slide : pc;
#else
    result.object_pc = result.module_offset;
#endif

    return result;
}

#if !defined(__APPLE__)
std::vector<uintptr_t> collect_unwind_frames(const DumpOptions& options) {
    std::vector<uintptr_t> pcs;
    pcs.reserve(options.max_frames);

    unw_context_t context;
    unw_cursor_t cursor;
    if (unw_getcontext(&context) < 0 || unw_init_local(&cursor, &context) < 0) {
        return pcs;
    }

    std::size_t skipped = 0;
    do {
        unw_word_t ip = 0;
        if (unw_get_reg(&cursor, UNW_REG_IP, &ip) < 0) {
            break;
        }

        if (ip != 0) {
            if (skipped < options.skip_frames) {
                ++skipped;
            } else if (pcs.size() < options.max_frames) {
                pcs.push_back(static_cast<uintptr_t>(ip));
            }
        }
    } while (pcs.size() < options.max_frames && unw_step(&cursor) > 0);

    return pcs;
}
#else
std::vector<uintptr_t> collect_unwind_frames(const DumpOptions& options) {
    std::vector<uintptr_t> pcs;
    pcs.reserve(options.max_frames);

    struct CallbackContext {
        std::vector<uintptr_t>* pcs = nullptr;
        std::size_t max_frames = 0;
        std::size_t skip_frames = 0;
        std::size_t visited = 0;
    };

    CallbackContext context;
    context.pcs = &pcs;
    context.max_frames = options.max_frames;
    context.skip_frames = options.skip_frames;

    _Unwind_Backtrace(
        [](_Unwind_Context* unwind_context, void* data) -> _Unwind_Reason_Code {
            auto* callback_context = static_cast<CallbackContext*>(data);
            const uintptr_t ip =
                static_cast<uintptr_t>(_Unwind_GetIP(unwind_context));

            if (ip != 0) {
                if (callback_context->visited < callback_context->skip_frames) {
                    ++callback_context->visited;
                } else if (callback_context->pcs->size() < callback_context->max_frames) {
                    callback_context->pcs->push_back(ip);
                }
            }

            return callback_context->pcs->size() < callback_context->max_frames
                       ? _URC_NO_REASON
                       : _URC_END_OF_STACK;
        },
        &context);

    return pcs;
}
#endif

bool parse_hex(const char* value, uintptr_t* out) {
    if (value == nullptr || out == nullptr) {
        return false;
    }

    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(value, &end, 16);
    if (end == value || *end != '\0') {
        return false;
    }

    *out = static_cast<uintptr_t>(parsed);
    return true;
}

bool parse_decimal(const char* value, std::size_t* out) {
    if (value == nullptr || out == nullptr) {
        return false;
    }

    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(value, &end, 10);
    if (end == value || *end != '\0') {
        return false;
    }

    *out = static_cast<std::size_t>(parsed);
    return true;
}

char* find_tagged_event_payload(char* line, const char* tag, const char* event) {
    if (line == nullptr || tag == nullptr || event == nullptr) {
        return nullptr;
    }

    char* tag_separator = std::strstr(line, ": ");
    if (tag_separator == nullptr) {
        return nullptr;
    }

    const std::size_t tag_length = static_cast<std::size_t>(tag_separator - line);
    if (std::strlen(tag) != tag_length || std::strncmp(line, tag, tag_length) != 0) {
        return nullptr;
    }

    char* payload = tag_separator + 2;
    const std::size_t event_length = std::strlen(event);
    if (std::strncmp(payload, event, event_length) != 0 ||
        (payload[event_length] != ' ' && payload[event_length] != '\t')) {
        return nullptr;
    }

    return payload;
}

bool parse_frame_line(char* line, FrameRecord* record) {
    char* payload = find_tagged_event_payload(line, kCollectorLogTag, kFrameEvent);
    if (payload == nullptr || record == nullptr) {
        return false;
    }

    FrameRecord parsed;
    char* save = nullptr;
    (void)::strtok_r(payload, " \t\r\n", &save);
    for (char* token = ::strtok_r(nullptr, " \t\r\n", &save);
         token != nullptr;
         token = ::strtok_r(nullptr, " \t\r\n", &save)) {
        char* separator = std::strchr(token, '=');
        if (separator == nullptr) {
            continue;
        }

        *separator = '\0';
        const char* key = token;
        const char* value = separator + 1;
        if (std::strcmp(key, "index") == 0) {
            parse_decimal(value, &parsed.index);
        } else if (std::strcmp(key, "object_pc") == 0) {
            parse_hex(value, &parsed.object_pc);
        } else if (std::strcmp(key, "module") == 0) {
            parsed.module = value;
        }
    }

    if (parsed.object_pc == 0) {
        return false;
    }

    *record = parsed;
    return true;
}

int resolved_frame_callback(void* data,
                            uintptr_t lookup_pc,
                            const char* filename,
                            int line_number,
                            const char* function_name) {
    auto* context = static_cast<ResolveContext*>(data);
    context->callback_called = true;

    const char* normalized_filename = normalize_filename(filename);
    std::fprintf(context->output,
                 "frame #%zu object_pc=0x%" PRIxPTR " lookup_pc=0x%" PRIxPTR
                 " function=%s at %s",
                 context->index,
                 context->object_pc,
                 lookup_pc,
                 demangle(function_name).c_str(),
                 normalized_filename != nullptr ? normalized_filename : "<unknown>");
    if (line_number > 0) {
        std::fprintf(context->output, ":%d", line_number);
    }
    std::fprintf(context->output, "\n");
    return 0;
}

void resolve_frame(std::FILE* output,
                   backtrace_state* state,
                   std::size_t index,
                   uintptr_t object_pc) {
    ResolveContext context;
    context.output = output;
    context.index = index;
    context.object_pc = object_pc;

    const uintptr_t lookup_pc = object_pc > 0 ? object_pc - 1 : object_pc;
    const int result = backtrace_pcinfo(state,
                                        lookup_pc,
                                        resolved_frame_callback,
                                        backtrace_error_callback,
                                        &context);
    if (result != 0 || !context.callback_called) {
        std::fprintf(output,
                     "frame #%zu object_pc=0x%" PRIxPTR " lookup_pc=0x%" PRIxPTR
                     " function=<unknown> at <unknown>\n",
                     index,
                     object_pc,
                     lookup_pc);
    }
}

}  // namespace

int dump_release_stack_addresses(std::FILE* output,
                                 int signo,
                                 void* fault_address,
                                 const DumpOptions& options) {
    if (output == nullptr || options.max_frames == 0) {
        return -1;
    }

    std::fprintf(output,
                 "%s: CRASH signal=%d fault=%p pid=%d\n",
                 kCollectorLogTag,
                 signo,
                 fault_address,
                 static_cast<int>(getpid()));

    std::vector<uintptr_t> pcs = collect_unwind_frames(options);
    for (std::size_t i = 0; i < pcs.size(); ++i) {
        const ModuleAddress frame = resolve_module_address(pcs[i]);
        std::fprintf(output,
                     "%s: %s index=%zu pc=0x%" PRIxPTR
                     " module_base=0x%" PRIxPTR
                     " module_offset=0x%" PRIxPTR
                     " object_pc=0x%" PRIxPTR
                     " module=%s\n",
                     kCollectorLogTag,
                     kFrameEvent,
                     i,
                     frame.pc,
                     frame.module_base,
                     frame.module_offset,
                     frame.object_pc,
                     frame.module_path);
    }

    return static_cast<int>(pcs.size());
}

int symbolize_release_stack(std::FILE* output,
                            const char* symbol_file,
                            std::FILE* crash_log_input) {
    if (output == nullptr || symbol_file == nullptr || crash_log_input == nullptr) {
        return -1;
    }

    backtrace_state* state = backtrace_create_state(symbol_file, 1, backtrace_error_callback, nullptr);
    if (state == nullptr) {
        return -1;
    }

    char* line = nullptr;
    std::size_t line_capacity = 0;
    int resolved_count = 0;
    while (::getline(&line, &line_capacity, crash_log_input) >= 0) {
        FrameRecord frame;
        if (!parse_frame_line(line, &frame)) {
            continue;
        }

        resolve_frame(output, state, frame.index, frame.object_pc);
        ++resolved_count;
    }
    std::free(line);

    return resolved_count > 0 ? resolved_count : -1;
}

int dump_debug_stack_symbols(std::FILE* output,
                             const char* executable_path,
                             int signo,
                             void* fault_address,
                             const DumpOptions& options) {
    if (output == nullptr || executable_path == nullptr || options.max_frames == 0) {
        return -1;
    }

    backtrace_state* state = backtrace_create_state(executable_path, 1, backtrace_error_callback, nullptr);
    if (state == nullptr) {
        return -1;
    }

    std::fprintf(output,
                 "%s: CRASH signal=%d fault=%p pid=%d\n",
                 kDebugLogTag,
                 signo,
                 fault_address,
                 static_cast<int>(getpid()));

    std::vector<uintptr_t> pcs = collect_unwind_frames(options);
    for (std::size_t i = 0; i < pcs.size(); ++i) {
        const ModuleAddress frame = resolve_module_address(pcs[i]);
        resolve_frame(output, state, i, frame.object_pc);
    }

    return static_cast<int>(pcs.size());
}

}  // namespace crashtrace
