add_executable(
    red_plasma_editor
    src/main.cpp
)


target_compile_features(red_plasma_editor PRIVATE cxx_std_20)

target_link_libraries(red_plasma_editor
  PRIVATE
    red_plasma_engine
)

# Build shaders before building the editor
add_dependencies(red_plasma_editor rp_shaders)

# Provide shader SPV paths as compile-time macros
target_compile_definitions(red_plasma_editor PRIVATE
  RP_TRIANGLE_VERT_SPV="$<SHELL_PATH:${CMAKE_BINARY_DIR}/assets/shaders/vulkan/triangle.vert.spv>"
  RP_TRIANGLE_FRAG_SPV="$<SHELL_PATH:${CMAKE_BINARY_DIR}/assets/shaders/vulkan/triangle.frag.spv>"
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)

target_include_directories(red_plasma_editor PRIVATE ${GLFW_INCLUDE_DIRS})
target_link_libraries(red_plasma_editor PRIVATE ${GLFW_LIBRARIES})
target_compile_options(red_plasma_editor PRIVATE ${GLFW_CFLAGS_OTHER})

target_compile_definitions(red_plasma_editor PRIVATE GLFW_EXPOSE_NATIVE_WAYLAND)
