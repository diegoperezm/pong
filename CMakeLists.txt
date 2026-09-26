cmake_minimum_required(VERSION 3.20)

project(Pong VERSION 0.1.0 DESCRIPTION "Pong" LANGUAGES C)

# Compiler Settings
###############################################################################

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Wall -Werror  -Wdouble-promotion -Wno-sign-conversion
set(CMAKE_C_FLAGS_DEBUG "-g3 -fsanitize=address,undefined -Wno-unused-parameter -Wno-unused-function ")
set(CMAKE_C_FLAGS_RELEASE "-O3")


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_BUILD_TYPE Debug)
endif()





# Third-Party Libraries
###############################################################################

# raylib
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

add_subdirectory(third_party/raylib)

# Box2D
set(BOX2D_SAMPLES OFF CACHE BOOL "" FORCE)
set(BOX2D_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_BENCHMARKS OFF CACHE BOOL "" FORCE)

add_subdirectory(third_party/box2d)

# Executable
###############################################################################

add_executable(
    pong
    
    src/main.c
    src/third_party_impl.c
    src/input.c
    src/game.c
    src/simulation.c
    src/entities.c
    src/render.c
    src/log.c
    src/debug.c
)

# Include Directories
###############################################################################

target_include_directories(
    pong 
    PRIVATE
        include
	src
        third_party/raygui
)

# Link Libraries
###############################################################################

target_link_libraries(
    pong
    PRIVATE
        raylib
        box2d
)

# Output
###############################################################################

set_target_properties(
    pong 
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Installation
###############################################################################

install(
    TARGETS pong 
    RUNTIME DESTINATION bin
)


# cmake -S . -B build
# cmake --build build


#rm -rf build
#cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
#cmake --build build
