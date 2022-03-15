cmake_minimum_required(VERSION 3.6)

# add files
add_executable(${EXE_NAME}
    ../../deps/imgui/imgui.cpp
    ../../deps/imgui/imgui_draw.cpp
    ../../deps/imgui/imgui_tables.cpp
    ../../deps/imgui/imgui_widgets.cpp
    ../../deps/imgui/imgui_demo.cpp
    ../../deps/imgui/misc/cpp/imgui_stdlib.cpp
    ../../deps/imgui-sfml/imgui-SFML.cpp

    Encoding.cpp
    main.cpp
    Processor.cpp
)

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/sim8051/stdafx.hpp
)

# includes
target_include_directories(${EXE_NAME}
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/imgui-sfml
        ${CMAKE_CURRENT_SOURCE_DIR}/../../deps
)

# linking
target_link_libraries(${EXE_NAME}
    GL sfml-graphics
)
