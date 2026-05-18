#pragma once

#include <cstddef>
#include <cstdio>

namespace crashtrace {

struct DumpOptions {
    std::size_t max_frames = 64;
    std::size_t skip_frames = 1;
};

// Interface 1: release binary crash-site stack address dump.
int dump_release_stack_addresses(std::FILE* output,
                                 int signo,
                                 void* fault_address,
                                 const DumpOptions& options = {});

// Interface 2: offline symbolication for release crash logs.
int symbolize_release_stack(std::FILE* output,
                            const char* symbol_file,
                            std::FILE* crash_log_input);

// Interface 3: debug binary crash-site stack address and symbol dump.
int dump_debug_stack_symbols(std::FILE* output,
                             const char* executable_path,
                             int signo,
                             void* fault_address,
                             const DumpOptions& options = {});

}  // namespace crashtrace
