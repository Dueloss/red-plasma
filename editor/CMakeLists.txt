add_executable(
    red_plasma_editor
    src/main.cpp
)

target_sources(red_plasma_editor
    PRIVATE
        # editor sources later
)

target_link_libraries(red_plasma_editor
    PRIVATE
        red_plasma_engine
)
