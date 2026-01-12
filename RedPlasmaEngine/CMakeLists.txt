# Create the Engine as a Shared Library (.so file)
add_library(RedPlasmaEngine SHARED
        src/Engine.cpp
        include/Engine.h
        include/Engine.h
        src/Engine.cpp
        ../RedPlasmaEditor/src/main.cpp
        include/IGraphicsDevice.h
        include/IWindowSurface.h
        src/VulkanGraphicsDevice.cpp
        include/VulkanGraphicsDevice.h
)

# Tell the engine where to find its own header files
target_include_directories(RedPlasmaEngine PUBLIC include)
# Find Vulkan (This is what you'll need for the triangle!)
find_package(Vulkan REQUIRED)
target_link_libraries(RedPlasmaEngine PRIVATE Vulkan::Vulkan)
