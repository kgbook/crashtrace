add_executable(backtrace_collector
    release_collector_demo.cpp
)
target_link_libraries(backtrace_collector PRIVATE
    crashtrace
)
configure_crashtrace_target(backtrace_collector)

add_executable(backtrace_debug_direct
    debug_direct_demo.cpp
)
target_link_libraries(backtrace_debug_direct PRIVATE
    crashtrace
)
configure_crashtrace_target(backtrace_debug_direct)
