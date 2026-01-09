add_library(red_plasma_engine STATIC)

target_include_directories(red_plasma_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_sources(red_plasma_engine
    PRIVATE
        # engine source files later
)

target_link_libraries(red_plasma_engine
    PRIVATE
        red_plasma_shared
)
