add_executable(backtrace_symbolizer
    release_symbolizer.cpp
)

target_link_libraries(backtrace_symbolizer PRIVATE
    crashtrace
)

configure_crashtrace_target(backtrace_symbolizer)
