cmake_minimum_required(VERSION 3.28)
project(marsengen C)

set(CMAKE_C_STANDARD 99)

add_executable(marsengen main.c)
target_compile_options(marsengen PRIVATE -pg)
target_link_options(marsengen PRIVATE -pg)
