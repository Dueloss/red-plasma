add_executable(rp_triangle)

target_sources(rp_triangle
    PRIVATE
        main.cpp
)

target_link_libraries(rp_triangle
    PRIVATE
        red_plasma_engine
)
