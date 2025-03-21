add_library(gui_lib
    window.cpp
)

target_include_directories(gui_lib PUBLIC
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(gui_lib PUBLIC
    monitor_lib
    imgui
    OpenGL::GL
) 