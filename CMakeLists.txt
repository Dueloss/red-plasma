cmake_minimum_required(VERSION 3.24)
project(red_plasma LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(RP_BUILD_EDITOR "Build the editor" ON)
option(RP_BUILD_ENGINE "Build engine library" ON)
option(RP_BUILD_SAMPLES "Build samples" OFF)

add_subdirectory(shared)
add_subdirectory(engine)

find_program(GLSLC glslc)
if (NOT GLSLC)
  message(FATAL_ERROR "glslc not found. Install a Vulkan SDK/glslang package providing glslc.")
endif()

set(RP_SHADER_SRC_DIR ${CMAKE_SOURCE_DIR}/assets/shaders/vulkan)
set(RP_SHADER_OUT_DIR ${CMAKE_BINARY_DIR}/assets/shaders/vulkan)
file(MAKE_DIRECTORY ${RP_SHADER_OUT_DIR})

add_custom_command(
  OUTPUT ${RP_SHADER_OUT_DIR}/triangle.vert.spv
  COMMAND ${GLSLC} -o ${RP_SHADER_OUT_DIR}/triangle.vert.spv ${RP_SHADER_SRC_DIR}/triangle.vert
  DEPENDS ${RP_SHADER_SRC_DIR}/triangle.vert
)

add_custom_command(
  OUTPUT ${RP_SHADER_OUT_DIR}/triangle.frag.spv
  COMMAND ${GLSLC} -o ${RP_SHADER_OUT_DIR}/triangle.frag.spv ${RP_SHADER_SRC_DIR}/triangle.frag
  DEPENDS ${RP_SHADER_SRC_DIR}/triangle.frag
)

add_custom_target(rp_shaders ALL
  DEPENDS
    ${RP_SHADER_OUT_DIR}/triangle.vert.spv
    ${RP_SHADER_OUT_DIR}/triangle.frag.spv
)

if(RP_BUILD_SAMPLES)
    add_subdirectory(engine/samples/triangle)
endif()

if(RP_BUILD_EDITOR)
    add_subdirectory(editor)
endif()
