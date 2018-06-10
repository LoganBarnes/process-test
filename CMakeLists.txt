cmake_minimum_required(VERSION 3.10)
project(ImPython)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

######################
### EXTERNAL REPOS ###
######################
include(DownloadProject)

# Download and unpack glfw at configure time
download_project(PROJ glfw-dl
        PREFIX thirdparty
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG master
        UPDATE_DISCONNECTED ${UPDATE_STATUS}
        ${QUIET_FLAG}
        )

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# compile glfw with current project
add_subdirectory(${glfw-dl_SOURCE_DIR} ${glfw-dl_BINARY_DIR})

# Download and unpack imgui at configure time
download_project(PROJ imgui-dl
        PREFIX thirdparty
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.60
        UPDATE_DISCONNECTED ${UPDATE_STATUS}
        ${QUIET_FLAG}
        )

add_library(impy-thirdparty
        # imgui
        ${imgui-dl_SOURCE_DIR}/imconfig.h
        ${imgui-dl_SOURCE_DIR}/imgui.cpp
        ${imgui-dl_SOURCE_DIR}/imgui.h
        ${imgui-dl_SOURCE_DIR}/imgui_demo.cpp
        ${imgui-dl_SOURCE_DIR}/imgui_draw.cpp
        ${imgui-dl_SOURCE_DIR}/imgui_internal.h
        ${imgui-dl_SOURCE_DIR}/stb_rect_pack.h
        ${imgui-dl_SOURCE_DIR}/stb_textedit.h
        ${imgui-dl_SOURCE_DIR}/stb_truetype.h
        ${imgui-dl_SOURCE_DIR}/examples/opengl3_example/imgui_impl_glfw_gl3.cpp
        ${imgui-dl_SOURCE_DIR}/examples/opengl3_example/imgui_impl_glfw_gl3.h

        src/thirdparty/gl3w/gl3w.c
        src/thirdparty/gl3w/GL/gl3w.h
        src/thirdparty/gl3w/GL/glcorearb.h
        src/thirdparty/gl3w/KHR/khrplatform.h
        )
target_include_directories(impy-thirdparty SYSTEM PUBLIC
        ${imgui-dl_SOURCE_DIR}
        ${imgui-dl_SOURCE_DIR}/examples/opengl3_example
        src/thirdparty/gl3w
        )
target_link_libraries(impy-thirdparty glfw)

add_executable(impy
        src/impy/main.cpp
        src/impy/PythonConsole.cpp
        src/impy/PythonConsole.hpp
        )
target_link_libraries(impy impy-thirdparty)

add_executable(impy-cli src/cli.cpp)
