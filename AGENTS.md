# PROJECT KNOWLEDGE BASE

**Generated:** 2026-05-18
**Commit:** 7b29a65
**Branch:** main

## OVERVIEW
C++ crash stacktrace library (`crashtrace`) with debug-direct and release-collector demo scenarios. Uses `libunwind` for stack unwinding and `libbacktrace` for DWARF/elf symbolization.

## STRUCTURE
```
./
├── lib/               # crashtrace static library (crashtrace.hpp + crashtrace.cpp)
├── demo/              # debug_direct_demo, release_collector_demo
├── tools/             # release_symbolizer (offline symbolication)
├── scripts/           # build/run/symbolize shell scripts
├── docs/              # api.md, workflow.md
├── third_party/       # libbacktrace + libunwind submodules
└── CMakeLists.txt     # Build orchestration + ExternalProject for deps
```

## WHERE TO LOOK
| Task | Location | Notes |
|------|----------|-------|
| Core library | `lib/src/crashtrace.cpp` | 3 public APIs, stack unwinding, module resolution |
| Public API | `lib/include/crashtrace/crashtrace.hpp` | Clean interface header |
| Debug scenario | `demo/debug_direct_demo.cpp` | Signal handler → dump_debug_stack_symbols |
| Release scenario | `demo/release_collector_demo.cpp` | Signal handler → dump_release_stack_addresses |
| Offline symbolizer | `tools/release_symbolizer.cpp` | symbolize_release_stack consumer |
| Build config | `CMakeLists.txt` | ExternalProject, libunwind provider switching |

## CONVENTIONS
- `crashtrace::` namespace for all public APIs
- Anonymous namespace for internal helpers in .cpp
- `DumpOptions` struct with `max_frames` (default 64) and `skip_frames` (default 1)
- Signal handler pattern: `sigaction` with `SA_SIGINFO`, exits via `_exit(128+signo)`
- Platform-specific: macOS uses dyld image slide; Linux uses module_offset
- `BACKTRACE_DEMO_NOINLINE` macro prevents frame elimination

## ANTI-PATTERNS (THIS PROJECT)
- **NEVER edit `third_party/libbacktrace/`** — vendored submodule
- **NEVER assume `make` is available** — CMake uses `$MAKE_EXECUTABLE`
- **NEVER call backtrace from signal handler before initial call outside** — order matters
- **NEVER use release build for debugging** — stripped symbols

## COMMANDS
```bash
# First-time only
git submodule update --init --recursive

# Build
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build

# Run demos
./build/backtrace_debug_direct     # debug: direct symbolization
./build/backtrace_collector        # release: address dump only

# Build with custom libunwind provider
cmake -DBACKTRACE_DEMO_LIBUNWIND_PROVIDER=bundled ...
```

## NOTES
- Async-signal-safety: `backtrace_create_state()` MUST be called before signal handler use
- macOS: dSYM generated post-build if `dsymutil` in PATH
- Linux: debug symbols in `.debug` file, linked via `gnu_debuglink`
