cmake_minimum_required(VERSION 3.6)
project(hello_opengl)

# OpenGL Package
find_package(OpenGL REQUIRED)

# Link Libraries
set(ALL_LIBS
        ${OPENGL_LIBRARY}
        glew32
        gdi32
        glfw3
        psapi
        )

link_libraries(${ALL_LIBS})

# Include Current Directory
include_directories(./common ./imgui)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(IMGUI_LIBRARY imgui/imgui_internal.h imgui/imgui.h imgui/imgui.cpp imgui/imconfig.h
        imgui/imgui_impl_glfw_gl3.h imgui/imgui_impl_glfw_gl3.cpp imgui/imgui_demo.cpp imgui/imgui_draw.cpp
        imgui/stb_rect_pack.h imgui/stb_textedit.h imgui/stb_truetype.h)
set(SOURCE_FILES main.cpp common/LoadShader.h common/LoadShader.cpp common/Controls.hpp)
add_executable(hello_opengl ${SOURCE_FILES} ${IMGUI_LIBRARY})