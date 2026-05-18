# Workflow

## Dependency Policy

`libbacktrace` is always built from source:

- `third_party/libbacktrace`

`libunwind` can be selected with `BACKTRACE_DEMO_LIBUNWIND_PROVIDER`:

- `auto`: prefer `third_party/libunwind`, fall back to system/SDK libunwind if bootstrapping is unavailable.
- `bundled`: require `third_party/libunwind`.
- `system`: use system/SDK libunwind.

GNU `libunwind` does not commit a generated `configure` script in the Git repository, so `bundled` needs autotools (`autoreconf`, automake, libtool) to bootstrap `third_party/libunwind`.

## Debug Scenario

```text
debug binary
  signal handler
    crashtrace::dump_debug_stack_symbols()
      libunwind collects PCs
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
  crash log contains only addresses

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
