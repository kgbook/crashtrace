include(ExternalProject)

set(LIBBACKTRACE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libbacktrace")
set(LIBBACKTRACE_BUILD_DIR "${CMAKE_BINARY_DIR}/libbacktrace-build")
set(LIBBACKTRACE_INSTALL_DIR "${CMAKE_BINARY_DIR}/libbacktrace-install")
set(LIBBACKTRACE_INCLUDE_DIR "${LIBBACKTRACE_SOURCE_DIR}")
set(LIBBACKTRACE_LIBRARY "${LIBBACKTRACE_INSTALL_DIR}/lib/libbacktrace.a")
set(BACKTRACE_DEMO_AUTOTOOLS_HOST_TRIPLE "" CACHE STRING
    "Optional autotools host triple for cross compilation")

set(LIBUNWIND_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libunwind")
set(LIBUNWIND_BUILD_DIR "${CMAKE_BINARY_DIR}/libunwind-build")
set(LIBUNWIND_WORK_DIR "${LIBUNWIND_BUILD_DIR}/source")
set(LIBUNWIND_INSTALL_DIR "${CMAKE_BINARY_DIR}/libunwind-install")
set(LIBUNWIND_LIBRARY "${LIBUNWIND_INSTALL_DIR}/lib/libunwind.so")

set(AUTOTOOLS_HOST_ARGS)
if (BACKTRACE_DEMO_AUTOTOOLS_HOST_TRIPLE)
    list(APPEND AUTOTOOLS_HOST_ARGS
        "--host=${BACKTRACE_DEMO_AUTOTOOLS_HOST_TRIPLE}")
endif()

if (NOT EXISTS "${LIBBACKTRACE_SOURCE_DIR}/configure")
    message(FATAL_ERROR
        "libbacktrace submodule is missing. Run: "
        "git submodule update --init --recursive")
endif()

if (NOT APPLE AND NOT EXISTS "${LIBUNWIND_SOURCE_DIR}/configure.ac")
    message(FATAL_ERROR
        "libunwind submodule is missing. Run: "
        "git submodule update --init --recursive")
endif()

ExternalProject_Add(libbacktrace_project
    SOURCE_DIR "${LIBBACKTRACE_SOURCE_DIR}"
    BINARY_DIR "${LIBBACKTRACE_BUILD_DIR}"
    CONFIGURE_COMMAND
        "${CMAKE_COMMAND}" -E env
        "CC=${CMAKE_C_COMPILER}"
        "CFLAGS=-fPIC"
        "ac_cv_lib_z_compress=no"
        "ac_cv_lib_zstd_ZSTD_compress=no"
        "ac_cv_lib_lzma_lzma_auto_decoder=no"
        "ac_cv_header_mach_o_dyld_h=no"
        "${LIBBACKTRACE_SOURCE_DIR}/configure"
        "--prefix=${LIBBACKTRACE_INSTALL_DIR}"
        ${AUTOTOOLS_HOST_ARGS}
        "--disable-shared"
        "--enable-static"
    BUILD_COMMAND "${MAKE_EXECUTABLE}" -j1 -C "${LIBBACKTRACE_BUILD_DIR}"
    INSTALL_COMMAND "${MAKE_EXECUTABLE}" -C "${LIBBACKTRACE_BUILD_DIR}" install
    BUILD_BYPRODUCTS "${LIBBACKTRACE_LIBRARY}"
)

add_library(libbacktrace::libbacktrace STATIC IMPORTED GLOBAL)
set_target_properties(libbacktrace::libbacktrace PROPERTIES
    IMPORTED_LOCATION "${LIBBACKTRACE_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${LIBBACKTRACE_INCLUDE_DIR}"
)
add_dependencies(libbacktrace::libbacktrace libbacktrace_project)

if (APPLE)
    return()
endif()

string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" LIBUNWIND_PROCESSOR)
if (LIBUNWIND_PROCESSOR MATCHES "^(x86_64|amd64)$")
    set(LIBUNWIND_ARCH_NAME "x86_64")
elseif (LIBUNWIND_PROCESSOR MATCHES "^(aarch64|arm64)$")
    set(LIBUNWIND_ARCH_NAME "aarch64")
elseif (LIBUNWIND_PROCESSOR MATCHES "^arm")
    set(LIBUNWIND_ARCH_NAME "arm")
elseif (LIBUNWIND_PROCESSOR MATCHES "^(i[3-6]86|x86)$")
    set(LIBUNWIND_ARCH_NAME "x86")
elseif (LIBUNWIND_PROCESSOR MATCHES "^riscv")
    set(LIBUNWIND_ARCH_NAME "riscv")
elseif (LIBUNWIND_PROCESSOR MATCHES "^loongarch64")
    set(LIBUNWIND_ARCH_NAME "loongarch64")
endif()

if (LIBUNWIND_ARCH_NAME)
    set(LIBUNWIND_ARCH_LIBRARY
        "${LIBUNWIND_INSTALL_DIR}/lib/libunwind-${LIBUNWIND_ARCH_NAME}.so")
    set(LIBUNWIND_BYPRODUCTS
        "${LIBUNWIND_LIBRARY}"
        "${LIBUNWIND_ARCH_LIBRARY}")
else()
    set(LIBUNWIND_BYPRODUCTS "${LIBUNWIND_LIBRARY}")
endif()

find_program(AUTORECONF_EXECUTABLE autoreconf REQUIRED)
file(MAKE_DIRECTORY "${LIBUNWIND_INSTALL_DIR}/include")

ExternalProject_Add(libunwind_project
    SOURCE_DIR "${LIBUNWIND_SOURCE_DIR}"
    BINARY_DIR "${LIBUNWIND_BUILD_DIR}"
    CONFIGURE_COMMAND
        "${CMAKE_COMMAND}" -E rm -rf "${LIBUNWIND_WORK_DIR}"
    COMMAND
        "${CMAKE_COMMAND}" -E copy_directory
        "${LIBUNWIND_SOURCE_DIR}"
        "${LIBUNWIND_WORK_DIR}"
    COMMAND
        "${CMAKE_COMMAND}" -E chdir "${LIBUNWIND_WORK_DIR}"
        "${CMAKE_COMMAND}" -E env
        "${AUTORECONF_EXECUTABLE}" -i
    COMMAND
        "${CMAKE_COMMAND}" -E chdir "${LIBUNWIND_WORK_DIR}"
        "${CMAKE_COMMAND}" -E env
        "CC=${CMAKE_C_COMPILER}"
        "CXX=${CMAKE_CXX_COMPILER}"
        "CFLAGS=-fPIC"
        "CXXFLAGS=-fPIC"
        "${LIBUNWIND_WORK_DIR}/configure"
        "--prefix=${LIBUNWIND_INSTALL_DIR}"
        ${AUTOTOOLS_HOST_ARGS}
        "--enable-shared"
        "--disable-static"
        "--disable-coredump"
        "--disable-ptrace"
        "--disable-setjmp"
    BUILD_COMMAND "${MAKE_EXECUTABLE}" -j1 -C "${LIBUNWIND_WORK_DIR}"
    INSTALL_COMMAND "${MAKE_EXECUTABLE}" -C "${LIBUNWIND_WORK_DIR}" install
    BUILD_BYPRODUCTS ${LIBUNWIND_BYPRODUCTS}
)

add_library(libunwind::libunwind SHARED IMPORTED GLOBAL)
set_target_properties(libunwind::libunwind PROPERTIES
    IMPORTED_LOCATION "${LIBUNWIND_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${LIBUNWIND_INSTALL_DIR}/include"
)
if (LIBUNWIND_ARCH_LIBRARY)
    set_property(TARGET libunwind::libunwind APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES "${LIBUNWIND_ARCH_LIBRARY}")
endif()
add_dependencies(libunwind::libunwind libunwind_project)
