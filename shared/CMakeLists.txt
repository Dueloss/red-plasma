add_library(red_plasma_shared STATIC)

target_include_directories(red_plasma_shared
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_sources(red_plasma_shared
    PRIVATE
        # add .cpp files later
)
