# PROJECT KNOWLEDGE BASE

**Generated:** 2026-05-18
**Commit:** 7b29a65
**Branch:** main

## OVERVIEW
C++ crash stacktrace library (`crashtrace`) with debug-direct and release-collector scenarios. Uses `libunwind` for stack unwinding and `libbacktrace` for DWARF/elf symbolization.

## STRUCTURE
```
./
├── lib/               # crashtrace static library (crashtrace.hpp + crashtrace.cpp)
├── demo/              # backtrace_debug_direct, backtrace_collector (release)
├── tools/             # backtrace_symbolizer (offline symbolication)
├── scripts/           # build/run/symbolize shell scripts
├── docs/              # api.md, workflow.md
├── third_party/       # libbacktrace + libunwind submodules
└── CMakeLists.txt    # ExternalProject for deps, libunwind provider switching
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

## COMMANDS

```bash
# First-time only
git submodule update --init --recursive

# Build (Release when no CMAKE_BUILD_TYPE; RelWithDebInfo for debug symbols)
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build

# Build single target
cmake --build build --target backtrace_debug_direct

# Run demos (exit code 139 = 128 + SIGSEGV(11) expected)
./build/backtrace_debug_direct     # debug: direct symbolization
./build/backtrace_collector        # release: address dump only

# Release workflow scripts
./scripts/build_release_symbols.sh
./scripts/run_release_collect.sh
./scripts/symbolize_release_log.sh

# Debug workflow
./scripts/run_debug_direct.sh
```

## PLATFORM REQUIREMENTS
- **macOS**: `dsymutil` required — for debug dSYM generation AND `build_release_symbols.sh`
- **Linux**: `objcopy` required — for splitting debug symbols from ELF in `build_release_symbols.sh`

## ANTI-PATTERNS (THIS PROJECT)
- **NEVER edit `third_party/libbacktrace/`** — vendored submodule
- **NEVER assume `make` is available** — CMake uses `$MAKE_EXECUTABLE` (gmake/make)
- **NEVER call backtrace from signal handler before initial call outside** — order matters for libbacktrace state init
- **NEVER use release build for debugging** — stripped symbols
- **NEVER use `auto` libunwind provider in release artifacts** — use explicit `bundled` or `system`

## CONVENTIONS
- `crashtrace::` namespace for all public APIs
- Anonymous namespace for internal helpers in .cpp
- `DumpOptions` struct with `max_frames` (default 64) and `skip_frames` (default 1)
- Signal handler: `sigaction` with `SA_SIGINFO`, exits via `_exit(128+signo)`
- Platform-specific: macOS uses dyld image slide; Linux uses module_offset
- `BACKTRACE_DEMO_NOINLINE` macro prevents frame elimination
- `BACKTRACE_DEMO_LIBUNWIND_PROVIDER`: `auto` (default), `bundled`, `system`

## NOTES
- Async-signal-safety: `backtrace_create_state()` MUST be called before signal handler use
- macOS: dSYM generated post-build if `dsymutil` in PATH
- Linux: debug symbols in `.debug` file, linked via `gnu_debuglink`
- GNU `libunwind` configure not in repo — `bundled` provider needs autotools (`autoreconf`)
- Exit code 139 = 128 + SIGSEGV(11) — expected crash exit code
