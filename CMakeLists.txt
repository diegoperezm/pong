cmake_minimum_required(VERSION 3.12)
project(15_puzzle C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(raylib REQUIRED)

add_executable(15_puzzle
    src/main.c
    src/app.c
    src/simulation.c
    src/render.c
    src/events.c
    src/input.c
    src/input_source.c
    src/rng.c
    src/log.c
    src/debug.c
)

target_link_libraries(15_puzzle PRIVATE raylib)
