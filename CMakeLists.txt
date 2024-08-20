cmake_minimum_required(VERSION 3.10)
project(ring_buffer)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加 include 目录
include_directories(include)

# 添加源文件
file(GLOB SOURCES "src/*.cpp")

# 添加可执行文件
add_executable(ring_buffer ${SOURCES})

# 链接 pthread 库
find_package(Threads REQUIRED)
target_link_libraries(ring_buffer ${CMAKE_THREAD_LIBS_INIT})

# 添加子目录
# add_subdirectory(tests)
