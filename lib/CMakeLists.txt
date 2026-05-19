add_library(crashtrace SHARED
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
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

if (TARGET libunwind::libunwind)
    target_link_libraries(crashtrace PUBLIC
        libunwind::libunwind
    )
endif()

configure_crashtrace_target(crashtrace)
