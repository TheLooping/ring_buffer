cmake_minimum_required(VERSION 3.10)
project(RingBufferProject)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加头文件目录
include_directories(include)

# 添加子目录
add_subdirectory(tests)

enable_testing()