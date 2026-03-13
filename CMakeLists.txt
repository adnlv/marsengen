cmake_minimum_required(VERSION 3.28)
project(marsengen C)

set(CMAKE_C_STANDARD 99)

add_executable(marsengen src/main.c)

target_compile_options(marsengen PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pg>

    $<$<CONFIG:Debug>:
        $<$<CXX_COMPILER_ID:MSVC>:/Od /Zi>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O0 -g3>
    >

    $<$<CONFIG:Release>:
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O3 -g0>
    >
)

target_link_options(marsengen PRIVATE $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-pg>)
