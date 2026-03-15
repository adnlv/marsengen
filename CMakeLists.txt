cmake_minimum_required(VERSION 3.28)
project(marsengen LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_executable(marsengen src/main.c src/memory.c)

target_compile_options(marsengen
    PRIVATE
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
    $<$<CONFIG:Debug>:$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O0 -g3>>
    $<$<CONFIG:Release>:$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O3 -g0>>
)
