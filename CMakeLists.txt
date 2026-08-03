cmake_minimum_required(VERSION 3.20)
project(Dongeon VERSION 4.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 主入口 + src 下全部源文件
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/dongeon.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

add_executable(dongeon ${SOURCES})

target_include_directories(dongeon PRIVATE "${CMAKE_SOURCE_DIR}/include")

if(MSVC)
    # MSVC 下源码为 UTF-8，避免中文乱码
    target_compile_options(dongeon PRIVATE /utf-8)
else()
    target_compile_options(dongeon PRIVATE -Wall -Wextra)
endif()

# 提醒：程序内使用相对路径 config/ 与 save/，
# 运行时需以项目根目录为工作目录启动（见 README）
