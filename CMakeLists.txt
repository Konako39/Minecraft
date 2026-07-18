cmake_minimum_required(VERSION 3.20)
project(OPENGLMC LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(openglmc
    src/main.cpp
    src/glad.c
    src/render/texture.cpp)

target_include_directories(openglmc PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# 项目内预编译 GLFW
target_link_directories(openglmc PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
)

find_package(OpenGL REQUIRED)

target_link_libraries(openglmc PRIVATE
    glfw3
    OpenGL::GL
)

# 运行时需要 shaders/
add_custom_command(TARGET openglmc POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:openglmc>/shaders
)
# 运行时需要 assets/（贴图等）
add_custom_command(TARGET openglmc POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:openglmc>/assets
)