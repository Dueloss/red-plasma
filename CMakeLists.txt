cmake_minimum_required(VERSION 3.20)
project(RedPlasma VERSION 0.1.0 LANGUAGES CXX)

# Use C++ 20 (standard for modern engines)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tell CMake where to find our sub-projects
add_subdirectory(RedPlasmaEngine)
add_subdirectory(RedPlasmaEditor)
# add_subdirectory(RedPlasmaEditor) # We will uncomment this when we start the Qt part