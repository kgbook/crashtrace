# crashtrace

这个工程把 C++ 崩溃栈能力拆成库、demo、工具和文档四层：

- `lib/`: `crashtrace` 动态库，封装 3 个公开接口。
- `demo/`: 两个运行场景的示例程序。
- `tools/`: release 崩溃日志的离线解析工具。
- `scripts/`: 构建、运行、解析脚本。
- `docs/`: API 和时序说明。

## 三个接口

公开头文件：[crashtrace.hpp](lib/include/crashtrace/crashtrace.hpp)

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

`run_release_collect.sh` 里使用 shell 重定向保存日志，只是 demo 为了演示完整流程。实际项目中更建议把 `dump_release_stack_addresses()` 的输出接入现有日志库、崩溃上报 SDK 或预分配的崩溃日志文件，并随日志一起带上版本号、git commit、build id 等信息，方便服务端用同版本符号文件离线解析。

离线解析只处理 `crashtrace_collector: FRAME ...` 行。`crashtrace_collector: CRASH ...` 只记录崩溃摘要，其他业务日志、其他 `LOG_TAG` 或无 tag 的行都会被 `symbolize_release_stack()` 忽略。

生成产物：

- `artifacts/release/backtrace_collector`: 模拟生产部署的 stripped release 二进制。
- `artifacts/symbols/`: 开发侧保存的同版本符号文件。
- `artifacts/tools/backtrace_symbolizer`: 调用接口 2 的离线工具。
- `artifacts/release/`、`artifacts/tools/`、`artifacts/sdk/`: `libcrashtrace.so`/`libcrashtrace.dylib`、Linux vendored `libunwind*.so*` 运行库和公开头文件。

## 为什么这样拆

`libbacktrace` 也能通过 `backtrace_full()` 遍历栈并拿到 PC，同时尝试符号化。Debug 场景下可以直接使用它。

生产环境更推荐先拿地址、再用 `libbacktrace` 离线解析：

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

## 构建依赖

本仓库不依赖包管理器提供的 `libunwind` 或 `libbacktrace`：

- `libbacktrace`: 固定从 `third_party/libbacktrace` 源码构建。
- Linux/ELF `libunwind`: 固定从 `third_party/libunwind` 源码构建为 shared `.so`，不链接系统或包管理器版本。
- macOS: GNU `libunwind` 的本地 unwinder 依赖 ELF 头和 `link.h`，不适合作为 Mach-O 本地 unwinder；macOS 使用编译器运行时的 `_Unwind_Backtrace` 采集 PC，符号解析仍使用 vendored `libbacktrace`。

初始化 submodule：

```bash
git submodule update --init --recursive
```

通用依赖：

- CMake 3.16+
- C/C++17 编译器
- `make` 或 `gmake`

Linux 额外依赖：

- autotools: `autoconf`、`automake`、`libtool`
- `objcopy`/`strip`: 通常来自 `binutils`

macOS 额外依赖：

- Xcode Command Line Tools
- `dsymutil`

脚本层 PATH 配置：

- CMake 不硬编码工具链 PATH。
- `scripts/*.sh` 会加载 `scripts/env.sh`。
- 脚本默认把 `/usr/bin:/bin:/usr/sbin:/sbin` 放到 PATH 前面，macOS 下会追加 `/opt/homebrew/bin`，避免外部 SDK 覆盖 autotools 依赖的 `diff` 等系统工具。
- 临时指定 PATH 前缀可用 `CRASHTRACE_PATH_PREPEND`。
- 机器级配置可复制 `scripts/local_env.sh.example` 为 `scripts/local_env.sh` 后编辑；该文件已被 git 忽略。

示例：

```bash
CRASHTRACE_PATH_PREPEND="/usr/bin:/bin:/usr/sbin:/sbin:/opt/homebrew/bin" \
  ./scripts/build_release_symbols.sh
```

GNU `libunwind` 仓库不提交生成后的 `configure`，Linux 构建会用 `autoreconf` 从源码生成构建脚本：

```bash
autoreconf --version
```

常见安装命令：

```bash
# Ubuntu/Debian
sudo apt-get install -y cmake build-essential autoconf automake libtool binutils

# macOS
xcode-select --install
```

Linux arm32 交叉编译还需要：

```bash
sudo apt-get install -y gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf binutils-arm-linux-gnueabihf
```

## 构建

```bash
./scripts/build_release_symbols.sh
```

手动构建：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --parallel
```

Linux arm32 交叉编译示例：

```bash
cmake -S . -B build-arm32 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=arm \
  -DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc \
  -DCMAKE_CXX_COMPILER=arm-linux-gnueabihf-g++ \
  -DBACKTRACE_DEMO_AUTOTOOLS_HOST_TRIPLE=arm-linux-gnueabihf
cmake --build build-arm32 --parallel
```

## 平台说明

- macOS: release 符号文件保存为 `backtrace_collector` + `backtrace_collector.dSYM/`。本工程让 vendored `libbacktrace` 走文件路径解析分支，并固定 `-gdwarf-4`。
- Linux/ELF: release 符号文件保存为 `backtrace_collector.debug`，生产 release 二进制通过 `gnu_debuglink` 关联符号文件。
- 地址日志包含 `pc/module_base/module_offset/object_pc`。离线解析使用 `object_pc`。
