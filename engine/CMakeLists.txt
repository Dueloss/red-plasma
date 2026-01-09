add_library(
    red_plasma_engine STATIC
    src/engine.cpp
)

target_include_directories(red_plasma_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_sources(red_plasma_engine
    PRIVATE
        # engine source files later
)

find_package(Vulkan REQUIRED)
target_link_libraries(red_plasma_engine
    PRIVATE
        red_plasma_shared
        Vulkan::Vulkan
)

target_compile_definitions(red_plasma_engine PRIVATE VK_USE_PLATFORM_WAYLAND_KHR)
target_compile_features(red_plasma_engine PUBLIC cxx_std_20)
