# backtrace-demo

一个 C++17 示例程序：启动时初始化 `libbacktrace`，注册 `SIGSEGV` 处理器，然后主动触发空指针写入。在信号处理器中打印调用栈，日志输出由 `SpdlogHelper` 接管。

## 依赖

项目使用两个 git submodule：

- `third_party/libbacktrace`: 生成崩溃调用栈。
- `third_party/SpdlogHelper`: 封装 `spdlog` 日志输出，包含嵌套 submodule `spdlog`。

首次 clone 后需要初始化递归 submodule：

```bash
git submodule update --init --recursive
```

## 构建

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

CMake 会自动构建 `libbacktrace` 静态库，并把 `SpdlogHelper/helper/ALog.cpp` 与嵌套的 `spdlog` 链接进示例程序。

## 运行

```bash
./build/backtrace_demo
```

程序会主动触发 `SIGSEGV`，因此非零退出是预期行为。退出码通常是 `139`，也就是 `128 + SIGSEGV`。


```text
[2026-05-15 19:59:59.134][pid91248][tid2368377][info][BACKTRACE_DEMO][main#77]starting backtrace demo
[2026-05-15 19:59:59.134][pid91248][tid2368377][warning][BACKTRACE_DEMO][main#87]triggering SIGSEGV to demonstrate libbacktrace logging
[2026-05-15 19:59:59.134][pid91248][tid2368377][error][BACKTRACE_DEMO][print_backtrace#41]==== libbacktrace stack ====
[2026-05-15 19:59:59.138][pid91248][tid2368377][error][BACKTRACE_DEMO][handle_backtrace_frame#36]  pc=0x104e611eb function=_Z15print_backtracev at /Volumes/workspace/me/backtrace-demo/src/main.cpp:47
[2026-05-15 19:59:59.138][pid91248][tid2368377][error][BACKTRACE_DEMO][handle_backtrace_frame#36]  pc=0x104e61253 function=_Z15SIGSEGV_handleri at /Volumes/workspace/me/backtrace-demo/src/main.cpp:60
[2026-05-15 19:59:59.138][pid91248][tid2368377][error][BACKTRACE_DEMO][handle_backtrace_frame#36]  pc=0x18dd5e743 function=<unknown> at <unknown>
[2026-05-15 19:59:59.138][pid91248][tid2368377][error][BACKTRACE_DEMO][handle_backtrace_frame#36]  pc=0x104e612ab function=_Z11doSomethingv at /Volumes/workspace/me/backtrace-demo/src/main.cpp:73
[2026-05-15 19:59:59.149][pid91248][tid2368377][error][BACKTRACE_DEMO][handle_backtrace_frame#36]  pc=0x104e613ab function=main at /Volumes/workspace/me/backtrace-demo/src/main.cpp:88
[2026-05-15 19:59:59.149][pid91248][tid2368377][error][BACKTRACE_DEMO][print_backtrace#49]============================

Process finished with exit code 139 (interrupted by signal 11:SIGSEGV)
```

## 说明

- `main()` 会先打印一条日志来初始化 `SpdlogHelper`，再安装 `SIGSEGV` handler。
- macOS 下如果找到 `dsymutil`，构建后会自动生成 dSYM，帮助 `libbacktrace` 符号化。
- 这是崩溃日志演示代码。生产环境中，复杂日志库通常不适合直接在信号处理器中做大量工作。
