# Workflow

## Dependency Policy

`crashtrace` does not use package-manager `libunwind` or `libbacktrace`.

- `libbacktrace` is always built from `third_party/libbacktrace`.
- Linux/ELF stack unwinding is always built from `third_party/libunwind` as shared `libunwind*.so*` runtime libraries.
- macOS stack unwinding uses `_Unwind_Backtrace` from the compiler runtime because GNU `libunwind`'s local unwinder is ELF-oriented and does not build cleanly as a Mach-O local unwinder.

Linux builds require autotools because the GNU `libunwind` submodule provides `configure.ac`, not a generated `configure` script:

```bash
sudo apt-get install -y autoconf automake libtool binutils
```

Build scripts source `scripts/env.sh` before invoking CMake or tooling. The script puts `/usr/bin:/bin:/usr/sbin:/sbin` first, and adds `/opt/homebrew/bin` on macOS, so external SDKs do not override autotools-critical commands such as `diff`. Use one of these script-level mechanisms for machine-specific PATH setup:

```bash
CRASHTRACE_PATH_PREPEND="/usr/bin:/bin:/usr/sbin:/sbin:/opt/homebrew/bin" \
  ./scripts/build_release_symbols.sh
```

Or copy `scripts/local_env.sh.example` to `scripts/local_env.sh` and edit it locally. `scripts/local_env.sh` is ignored by git.

macOS builds require Xcode Command Line Tools and `dsymutil`:

```bash
xcode-select --install
```

## Build Matrix

GitHub Actions builds these targets on push and pull request:

- macOS x86_64
- macOS arm64
- Linux x86_64
- Linux arm64
- Linux arm32 via `arm-linux-gnueabihf` cross compilation

Linux arm32 cross compilation passes the autotools host triple through CMake:

```bash
cmake -S . -B build-arm32 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=arm \
  -DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc \
  -DCMAKE_CXX_COMPILER=arm-linux-gnueabihf-g++ \
  -DBACKTRACE_DEMO_AUTOTOOLS_HOST_TRIPLE=arm-linux-gnueabihf
```

## Debug Scenario

```text
debug binary
  signal handler
    crashtrace::dump_debug_stack_symbols()
      collect PCs
        Linux: third_party/libunwind shared .so
        macOS: _Unwind_Backtrace
      libbacktrace resolves PCs immediately
  prints address + file + line + function
```

Script:

```bash
./scripts/run_debug_direct.sh
```

## Release Scenario

```text
build host
  build release collector
  save matching debug symbols
  strip release collector

production
  release collector crashes
  signal handler calls crashtrace::dump_release_stack_addresses()
  crash log contains addresses only

developer machine
  crashtrace::symbolize_release_stack()
  parses crash log with matching symbols
  prints file + line + function
```

Scripts:

```bash
./scripts/build_release_symbols.sh
./scripts/run_release_collect.sh
./scripts/symbolize_release_log.sh
```

## Symbol Files

- macOS: `artifacts/symbols/backtrace_collector.dSYM/` plus an unstripped binary copy.
- Linux: `artifacts/symbols/backtrace_collector.debug`, linked from the stripped binary with `gnu_debuglink`.

The crash log records `pc`, `module_base`, `module_offset`, `object_pc`, and `module`. Offline symbolication uses `object_pc` because it is normalized to the object file address space.

Linux release artifacts copy the vendored `libunwind*.so*` files next to `backtrace_collector`, `backtrace_symbolizer`, and `libcrashtrace.so`. The build sets `$ORIGIN` rpath so those executables and libraries resolve the vendored unwind runtime from their own directory.
