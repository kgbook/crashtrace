cmake_minimum_required(VERSION 3.16)

project(backtrace_demo LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(ExternalProject)

find_package(Threads REQUIRED)
find_program(MAKE_EXECUTABLE NAMES gmake make REQUIRED)

set(LIBBACKTRACE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libbacktrace")
set(LIBBACKTRACE_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/libbacktrace-build")
set(LIBBACKTRACE_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/libbacktrace-install")
set(LIBBACKTRACE_INCLUDE_DIR "${LIBBACKTRACE_SOURCE_DIR}")
set(LIBBACKTRACE_LIBRARY "${LIBBACKTRACE_INSTALL_DIR}/lib/libbacktrace.a")
set(LIBBACKTRACE_BUILD_PATH "/usr/bin:/bin:/usr/sbin:/sbin:/opt/homebrew/bin:$ENV{PATH}")
set(SPDLOG_HELPER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/SpdlogHelper")
set(SPDLOG_SOURCE_DIR "${SPDLOG_HELPER_SOURCE_DIR}/spdlog")

if (NOT EXISTS "${LIBBACKTRACE_SOURCE_DIR}/configure")
    message(FATAL_ERROR
        "libbacktrace submodule is missing. Run: "
        "git submodule update --init --recursive")
endif()

if (NOT EXISTS "${SPDLOG_HELPER_SOURCE_DIR}/helper/ALog.h")
    message(FATAL_ERROR
        "SpdlogHelper submodule is missing. Run: "
        "git submodule update --init --recursive")
endif()

if (NOT EXISTS "${SPDLOG_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "SpdlogHelper nested spdlog submodule is missing. Run: "
        "git submodule update --init --recursive")
endif()

ExternalProject_Add(libbacktrace_project
    SOURCE_DIR "${LIBBACKTRACE_SOURCE_DIR}"
    BINARY_DIR "${LIBBACKTRACE_BUILD_DIR}"
    CONFIGURE_COMMAND
        "${CMAKE_COMMAND}" -E env
        "PATH=${LIBBACKTRACE_BUILD_PATH}"
        "CC=${CMAKE_C_COMPILER}"
        "ac_cv_lib_z_compress=no"
        "ac_cv_lib_zstd_ZSTD_compress=no"
        "ac_cv_lib_lzma_lzma_auto_decoder=no"
        "${LIBBACKTRACE_SOURCE_DIR}/configure"
        "--prefix=${LIBBACKTRACE_INSTALL_DIR}"
        "--disable-shared"
        "--enable-static"
    BUILD_COMMAND "${MAKE_EXECUTABLE}"
    INSTALL_COMMAND "${MAKE_EXECUTABLE}" install
    BUILD_BYPRODUCTS "${LIBBACKTRACE_LIBRARY}"
)

add_library(libbacktrace::libbacktrace STATIC IMPORTED GLOBAL)
set_target_properties(libbacktrace::libbacktrace PROPERTIES
    IMPORTED_LOCATION "${LIBBACKTRACE_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${LIBBACKTRACE_INCLUDE_DIR}"
)
add_dependencies(libbacktrace::libbacktrace libbacktrace_project)

set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
add_subdirectory("${SPDLOG_SOURCE_DIR}" EXCLUDE_FROM_ALL)

if (TARGET spdlog::spdlog)
    set(SPDLOG_TARGET spdlog::spdlog)
else()
    set(SPDLOG_TARGET spdlog)
endif()

add_library(SpdlogHelper STATIC
    "${SPDLOG_HELPER_SOURCE_DIR}/helper/ALog.cpp"
)
target_compile_definitions(SpdlogHelper PUBLIC ENABLE_SPDLOG)
target_include_directories(SpdlogHelper SYSTEM PUBLIC
    "${SPDLOG_HELPER_SOURCE_DIR}/helper"
)
target_link_libraries(SpdlogHelper PUBLIC
    ${SPDLOG_TARGET}
)

add_executable(backtrace_demo
    src/main.cpp
)

target_include_directories(backtrace_demo PRIVATE
    ${LIBBACKTRACE_INCLUDE_DIR}
)

target_link_libraries(backtrace_demo PRIVATE
    libbacktrace::libbacktrace
    SpdlogHelper
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

if (UNIX)
    target_compile_options(backtrace_demo PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -g
    )
endif()

if (APPLE)
    find_program(DSYMUTIL_EXECUTABLE dsymutil)
    if (DSYMUTIL_EXECUTABLE)
        add_custom_command(TARGET backtrace_demo POST_BUILD
            COMMAND "${DSYMUTIL_EXECUTABLE}" "$<TARGET_FILE:backtrace_demo>"
            COMMENT "Generating dSYM for libbacktrace symbolication"
        )
    endif()
endif()
