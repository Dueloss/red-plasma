cmake_minimum_required(VERSION 3.24)
project(red_plasma LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(RP_BUILD_EDITOR "Build the editor" ON)
option(RP_BUILD_SAMPLES "Build samples" ON)

add_subdirectory(shared)
add_subdirectory(engine)

if(RP_BUILD_SAMPLES)
    add_subdirectory(engine/samples/triangle)
endif()

if(RP_BUILD_EDITOR)
    add_subdirectory(editor)
endif()
