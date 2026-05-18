add_library(crashtrace STATIC
    src/crashtrace.cpp
)

target_include_directories(crashtrace PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_include_directories(crashtrace PRIVATE
    "${CMAKE_SOURCE_DIR}/third_party/libbacktrace"
)
target_link_libraries(crashtrace PUBLIC
    libbacktrace::libbacktrace
    libunwind::libunwind
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

configure_crashtrace_target(crashtrace)
