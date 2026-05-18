# crashtrace-demo

这个工程把 C++ 崩溃栈能力拆成库、demo、工具和文档四层：

- `lib/`: `crashtrace` 静态库，封装 3 个公开接口。
- `demo/`: 两个运行场景的示例程序。
- `tools/`: release 崩溃日志的离线解析工具。
- `scripts/`: 构建、运行、解析脚本。
- `docs/`: API 和时序说明。

## 三个接口

公开头文件：[crashtrace.hpp](/Volumes/workspace/me/backtrace-demo/lib/include/crashtrace/crashtrace.hpp)

```cpp
namespace crashtrace {

int dump_release_stack_addresses(FILE* output,
                                 int signo,
                                 void* fault_address,
                                 const DumpOptions& options = {});

int symbolize_release_stack(FILE* output,
                            const char* symbol_file,
                            FILE* crash_log_input);

int dump_debug_stack_symbols(FILE* output,
                             const char* executable_path,
                             int signo,
                             void* fault_address,
                             const DumpOptions& options = {});

}
```

接口职责：

1. `dump_release_stack_addresses`: release 生产二进制崩溃时，只采集并 dump 调用栈地址。
2. `symbolize_release_stack`: 开发侧拿同版本 debug 符号和接口 1 的日志，解析文件名、函数名、行号。
3. `dump_debug_stack_symbols`: debug 调试环境崩溃时，直接采集并 dump 地址、文件名、函数名、符号。

## 两个场景

### 调试环境

调试环境运行 debug 版本，直接调用接口 3：

```bash
./scripts/run_debug_direct.sh
```

这个脚本会构建 `build-debug/backtrace_debug_direct`，程序崩溃后直接输出源码位置。

### 生产环境

生产环境运行 release 版本，只调用接口 1 输出地址；开发侧再调用接口 2 解析。

```bash
./scripts/build_release_symbols.sh
./scripts/run_release_collect.sh
./scripts/symbolize_release_log.sh
```

生成产物：

- `artifacts/release/backtrace_collector`: 模拟生产部署的 stripped release 二进制。
- `artifacts/symbols/`: 开发侧保存的同版本符号文件。
- `artifacts/tools/backtrace_symbolizer`: 调用接口 2 的离线工具。
- `artifacts/sdk/`: `libcrashtrace.a` 和公开头文件。

## 为什么这样拆

`libbacktrace` 也能通过 `backtrace_full()` 遍历栈并拿到 PC，同时尝试符号化。Debug 场景下可以直接使用它。

生产环境更推荐用 `libunwind` 拿地址、用 `libbacktrace` 离线解析：

- 生产二进制不带 debug 符号。
- 崩溃现场只做轻量地址采集，不做 DWARF/dSYM 解析。
- 地址日志更稳定，便于上传、归档和批量解析。
- 开发侧掌握同版本符号文件，可以离线还原文件名、函数名、行号。

## 目录

```text
lib/
  include/crashtrace/crashtrace.hpp
  src/crashtrace.cpp
demo/
  release_collector_demo.cpp
  debug_direct_demo.cpp
tools/
  release_symbolizer.cpp
scripts/
  build_release_symbols.sh
  run_release_collect.sh
  symbolize_release_log.sh
  run_debug_direct.sh
docs/
  api.md
  workflow.md
```

## libunwind / libbacktrace 依赖

`libbacktrace` 固定从 `third_party/libbacktrace` 源码集成。

`libunwind` 支持三种 provider：

- `auto`: 默认。优先构建 `third_party/libunwind`；如果缺少 `autoreconf` 或源码不可用，回退系统/SDK `libunwind`。
- `bundled`: 强制构建 `third_party/libunwind`。
- `system`: 强制使用系统/SDK `libunwind`。

初始化 submodule：

```bash
git submodule update --init --recursive
```

GNU `libunwind` 仓库不提交生成后的 `configure`。如果使用 `bundled` provider，构建机需要 autotools 来从源码生成构建脚本：

```bash
autoreconf --version
```

缺少时可以安装构建工具 `autoconf`、`automake`、`libtool`，也可以使用默认 `auto` 或显式 `system` provider。

## 构建

```bash
./scripts/build_release_symbols.sh
```

强制使用 third-party `libunwind`：

```bash
./scripts/build_release_symbols.sh -DBACKTRACE_DEMO_LIBUNWIND_PROVIDER=bundled
```

强制使用系统/SDK `libunwind`：

```bash
./scripts/build_release_symbols.sh -DBACKTRACE_DEMO_LIBUNWIND_PROVIDER=system
```

## 平台说明

- macOS: release 符号文件保存为 `backtrace_collector` + `backtrace_collector.dSYM/`。本工程让 vendored `libbacktrace` 走文件路径解析分支，并固定 `-gdwarf-4`。
- Linux/ELF: release 符号文件保存为 `backtrace_collector.debug`，生产 release 二进制通过 `gnu_debuglink` 关联符号文件。
- 地址日志包含 `pc/module_base/module_offset/object_pc`。离线解析使用 `object_pc`。
