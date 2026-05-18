# crashtrace API

公开头文件：

```cpp
#include <crashtrace/crashtrace.hpp>
```

## 1. Release 崩溃地址采集

```cpp
int crashtrace::dump_release_stack_addresses(FILE* output,
                                             int signo,
                                             void* fault_address,
                                             const DumpOptions& options = {});
```

用于生产 release 二进制。接口只采集调用栈地址并输出结构化日志，不解析 debug 符号。

输出示例：

```text
BTDEMO_CRASH_V1 signal=11 fault=0x0 pid=45662
BTDEMO_FRAME index=0 pc=0x1045c0e5c module_base=0x1045c0000 module_offset=0xe5c object_pc=0x100000e5c module=/path/backtrace_collector
```

## 2. Release 离线解析

```cpp
int crashtrace::symbolize_release_stack(FILE* output,
                                        const char* symbol_file,
                                        FILE* crash_log_input);
```

用于开发侧。输入接口 1 生成的日志和同版本符号文件，输出源码文件、行号、函数名。

## 3. Debug 直接解析

```cpp
int crashtrace::dump_debug_stack_symbols(FILE* output,
                                         const char* executable_path,
                                         int signo,
                                         void* fault_address,
                                         const DumpOptions& options = {});
```

用于调试环境。Debug 版本二进制带符号，崩溃时直接采集并解析调用栈。

## DumpOptions

```cpp
struct DumpOptions {
    std::size_t max_frames = 64;
    std::size_t skip_frames = 1;
};
```

- `max_frames`: 最大采集帧数。
- `skip_frames`: 跳过库内部采集函数的帧数。

## 注意

这些接口用于演示基础能力封装。真实生产环境中，signal handler 里应尽量避免复杂 I/O、锁和堆分配，可将地址采集结果写入预分配缓冲区或最小化日志通道。
