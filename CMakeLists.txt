cmake_minimum_required(VERSION 3.16)
project(steganography)

include_directories(ext)

add_library(
    stb STATIC
    ext/stb/stb_image.c
    ext/stb/stb_image_write.c
)

add_library(
    zlib STATIC
	ext/zlib/adler32.c
	ext/zlib/compress.c
	ext/zlib/crc32.c
	ext/zlib/crc32.h
	ext/zlib/deflate.c
	ext/zlib/gzclose.c
	ext/zlib/gzlib.c
	ext/zlib/gzread.c
	ext/zlib/gzwrite.c
	ext/zlib/infback.c
	ext/zlib/inffast.c
	ext/zlib/inflate.c
	ext/zlib/inftrees.c
	ext/zlib/trees.c
	ext/zlib/uncompr.c
	ext/zlib/zutil.c)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED On)

# GUI library
add_library(
    gui_lib STATIC
    src/gui_main.cpp
)

target_include_directories(gui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${glfw_SOURCE_DIR}/include
)

target_link_libraries(gui_lib
    imgui_lib
    glfw
)

# Main executable
add_executable(
    steganography
    src/aes.cpp
    src/crc32.cpp
    src/image.cpp
    src/main.cpp
    src/sha256.cpp
)

include(FetchContent)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.3.8
)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.89.8
)

FetchContent_MakeAvailable(glfw imgui)

add_library(
  imgui_lib STATIC
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
)

target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${glfw_SOURCE_DIR}/include
)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Link libraries
target_link_libraries(
    steganography
    stb
    zlib
    gui_lib
    OpenGL::GL
)

target_include_directories(steganography PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${glfw_SOURCE_DIR}/include
)
