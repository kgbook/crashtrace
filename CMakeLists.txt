cmake_minimum_required(VERSION 3.16)

project(backtrace_demo LANGUAGES C CXX)

if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

find_package(Threads REQUIRED)
find_program(MAKE_EXECUTABLE NAMES gmake make REQUIRED)

set(BACKTRACE_DEMO_LIBUNWIND_PROVIDER "auto" CACHE STRING
    "libunwind provider: auto, bundled, or system")
set_property(CACHE BACKTRACE_DEMO_LIBUNWIND_PROVIDER PROPERTY STRINGS auto bundled system)

function(configure_crashtrace_target target_name)
    if (UNIX)
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -fno-omit-frame-pointer
            -fno-optimize-sibling-calls
            $<$<CONFIG:Release>:-O2>
            $<$<CONFIG:Release>:-g>
            $<$<CONFIG:RelWithDebInfo>:-g>
            $<$<OR:$<CONFIG:Debug>,$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-gdwarf-4>
        )
    endif()
endfunction()

add_subdirectory(third_party)
add_subdirectory(lib)
add_subdirectory(tools)
add_subdirectory(demo)
