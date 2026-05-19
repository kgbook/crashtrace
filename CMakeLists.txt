cmake_minimum_required(VERSION 3.16)

project(crashtrace LANGUAGES C CXX)

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

        if (APPLE)
            set(_crashtrace_rpath "@loader_path")
        else()
            set(_crashtrace_rpath "$ORIGIN")
        endif()

        set_target_properties(${target_name} PROPERTIES
            BUILD_RPATH "${_crashtrace_rpath}"
            INSTALL_RPATH "${_crashtrace_rpath}"
        )
    endif()
endfunction()

add_subdirectory(third_party)
add_subdirectory(lib)
add_subdirectory(tools)
add_subdirectory(demo)
