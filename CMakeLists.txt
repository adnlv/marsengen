cmake_minimum_required(VERSION 3.28)
project(marsengen C)

set(CMAKE_C_STANDARD 99)

option(ENABLE_PROFILING "Enable profiling with gprof (-pg)" OFF)

add_executable(marsengen src/main.c)

target_compile_options(marsengen PRIVATE
    $<$<C_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>

    $<$<CONFIG:Debug>:
        $<$<C_COMPILER_ID:MSVC>:/Od /Zi>
        $<$<NOT:$<C_COMPILER_ID:MSVC>>:-O0 -g3>
    >

    $<$<CONFIG:Release>:
        $<$<NOT:$<C_COMPILER_ID:MSVC>>:-O3 -g0>
    >
)

if(ENABLE_PROFILING AND NOT MSVC)
    target_compile_options(marsengen PRIVATE -pg)
    target_link_options(marsengen PRIVATE -pg)
endif()
