include(ExternalProject)

set(LIBBACKTRACE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libbacktrace")
set(LIBBACKTRACE_BUILD_DIR "${CMAKE_BINARY_DIR}/libbacktrace-build")
set(LIBBACKTRACE_INSTALL_DIR "${CMAKE_BINARY_DIR}/libbacktrace-install")
set(LIBBACKTRACE_INCLUDE_DIR "${LIBBACKTRACE_SOURCE_DIR}")
set(LIBBACKTRACE_LIBRARY "${LIBBACKTRACE_INSTALL_DIR}/lib/libbacktrace.a")
set(LIBBACKTRACE_BUILD_PATH "/usr/bin:/bin:/usr/sbin:/sbin:/opt/homebrew/bin:$ENV{PATH}")

set(LIBUNWIND_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libunwind")
set(LIBUNWIND_BUILD_DIR "${CMAKE_BINARY_DIR}/libunwind-build")
set(LIBUNWIND_WORK_DIR "${LIBUNWIND_BUILD_DIR}/source")
set(LIBUNWIND_INSTALL_DIR "${CMAKE_BINARY_DIR}/libunwind-install")
set(LIBUNWIND_LIBRARY "${LIBUNWIND_INSTALL_DIR}/lib/libunwind.a")
set(LIBUNWIND_BUILD_PATH "/usr/bin:/bin:/usr/sbin:/sbin:/opt/homebrew/bin:$ENV{PATH}")

if (NOT EXISTS "${LIBBACKTRACE_SOURCE_DIR}/configure")
    message(FATAL_ERROR
        "libbacktrace submodule is missing. Run: "
        "git submodule update --init --recursive")
endif()

if (BACKTRACE_DEMO_LIBUNWIND_PROVIDER STREQUAL "bundled"
        AND NOT EXISTS "${LIBUNWIND_SOURCE_DIR}/configure.ac")
    message(FATAL_ERROR
        "libunwind submodule is missing. Run: "
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
        "ac_cv_header_mach_o_dyld_h=no"
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

set(USE_BUNDLED_LIBUNWIND OFF)
if (BACKTRACE_DEMO_LIBUNWIND_PROVIDER STREQUAL "bundled")
    set(USE_BUNDLED_LIBUNWIND ON)
elseif (BACKTRACE_DEMO_LIBUNWIND_PROVIDER STREQUAL "system")
    set(USE_BUNDLED_LIBUNWIND OFF)
elseif (BACKTRACE_DEMO_LIBUNWIND_PROVIDER STREQUAL "auto")
    find_program(AUTORECONF_EXECUTABLE autoreconf)
    if (AUTORECONF_EXECUTABLE AND EXISTS "${LIBUNWIND_SOURCE_DIR}/configure.ac")
        set(USE_BUNDLED_LIBUNWIND ON)
    else()
        set(USE_BUNDLED_LIBUNWIND OFF)
        message(STATUS
            "Using system libunwind because bundled libunwind cannot be bootstrapped "
            "(autoreconf missing or submodule unavailable).")
    endif()
else()
    message(FATAL_ERROR
        "BACKTRACE_DEMO_LIBUNWIND_PROVIDER must be auto, bundled, or system")
endif()

if (USE_BUNDLED_LIBUNWIND)
    find_program(AUTORECONF_EXECUTABLE autoreconf REQUIRED)
    file(MAKE_DIRECTORY "${LIBUNWIND_INSTALL_DIR}/include")

    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" LIBUNWIND_PROCESSOR)
    if (LIBUNWIND_PROCESSOR MATCHES "^(x86_64|amd64)$")
        set(LIBUNWIND_ARCH_NAME "x86_64")
    elseif (LIBUNWIND_PROCESSOR MATCHES "^(aarch64|arm64)$")
        set(LIBUNWIND_ARCH_NAME "aarch64")
    elseif (LIBUNWIND_PROCESSOR MATCHES "^(i[3-6]86|x86)$")
        set(LIBUNWIND_ARCH_NAME "x86")
    elseif (LIBUNWIND_PROCESSOR MATCHES "^riscv")
        set(LIBUNWIND_ARCH_NAME "riscv")
    elseif (LIBUNWIND_PROCESSOR MATCHES "^loongarch64")
        set(LIBUNWIND_ARCH_NAME "loongarch64")
    endif()

    if (LIBUNWIND_ARCH_NAME)
        set(LIBUNWIND_ARCH_LIBRARY
            "${LIBUNWIND_INSTALL_DIR}/lib/libunwind-${LIBUNWIND_ARCH_NAME}.a")
        set(LIBUNWIND_BYPRODUCTS
            "${LIBUNWIND_LIBRARY}"
            "${LIBUNWIND_ARCH_LIBRARY}")
    else()
        set(LIBUNWIND_BYPRODUCTS "${LIBUNWIND_LIBRARY}")
    endif()

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
            "PATH=${LIBUNWIND_BUILD_PATH}"
            "${AUTORECONF_EXECUTABLE}" -i
        COMMAND
            "${CMAKE_COMMAND}" -E chdir "${LIBUNWIND_WORK_DIR}"
            "${CMAKE_COMMAND}" -E env
            "PATH=${LIBUNWIND_BUILD_PATH}"
            "CC=${CMAKE_C_COMPILER}"
            "CXX=${CMAKE_CXX_COMPILER}"
            "${LIBUNWIND_WORK_DIR}/configure"
            "--prefix=${LIBUNWIND_INSTALL_DIR}"
            "--disable-shared"
            "--enable-static"
            "--disable-coredump"
            "--disable-ptrace"
            "--disable-setjmp"
        BUILD_COMMAND "${MAKE_EXECUTABLE}" -C "${LIBUNWIND_WORK_DIR}"
        INSTALL_COMMAND "${MAKE_EXECUTABLE}" -C "${LIBUNWIND_WORK_DIR}" install
        BUILD_BYPRODUCTS ${LIBUNWIND_BYPRODUCTS}
    )

    add_library(libunwind::libunwind STATIC IMPORTED GLOBAL)
    set_target_properties(libunwind::libunwind PROPERTIES
        IMPORTED_LOCATION "${LIBUNWIND_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBUNWIND_INSTALL_DIR}/include"
    )
    if (LIBUNWIND_ARCH_LIBRARY)
        set_property(TARGET libunwind::libunwind APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES "${LIBUNWIND_ARCH_LIBRARY}")
    endif()
    add_dependencies(libunwind::libunwind libunwind_project)
else()
    if (APPLE)
        add_library(libunwind::libunwind INTERFACE IMPORTED GLOBAL)
    else()
        find_path(SYSTEM_LIBUNWIND_INCLUDE_DIR libunwind.h REQUIRED)
        find_library(SYSTEM_LIBUNWIND_LIBRARY NAMES unwind REQUIRED)
        add_library(libunwind::libunwind INTERFACE IMPORTED GLOBAL)
        set_target_properties(libunwind::libunwind PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${SYSTEM_LIBUNWIND_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${SYSTEM_LIBUNWIND_LIBRARY}"
        )
    endif()
endif()
