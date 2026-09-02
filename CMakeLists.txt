cmake_minimum_required(VERSION 3.16)
project(dbscan CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(dbscan point.cpp)
target_include_directories(dbscan PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(dbscan PRIVATE -Wall -Wextra)

# ---------------------------------------------------------------------------
# Dear ImGui integration
# ---------------------------------------------------------------------------
# Follows the official "Getting Started / Integration" guide
# (https://github.com/ocornut/imgui/wiki/Getting-Started):
#   * Dear ImGui core is vendored in external/imgui and compiled directly
#     with the application (it does not need to be built as a library),
#   * the GLFW + OpenGL3 backends are compiled alongside it,
#   * GLFW itself is vendored in external/glfw and built as a subproject.
# imgui_viewer.hpp/.cpp wrap the ImGui setup / main loop / shutdown code.
# ---------------------------------------------------------------------------

enable_language(C)  # GLFW is written in C

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui)

add_executable(testMakeBlobs
    testMakeBlobs.cpp
    imgui_viewer.cpp
    # Dear ImGui core
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    # Dear ImGui backends (windowing + rendering)
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(testMakeBlobs PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# Build only the GLFW library itself (skip its examples/tests/docs).
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw)

find_package(OpenGL REQUIRED)
target_link_libraries(testMakeBlobs PRIVATE dbscan glfw OpenGL::GL)
