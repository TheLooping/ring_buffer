cmake_minimum_required(VERSION 3.10)
project(RingBufferProject)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

add_subdirectory(tests)

add_library(ring_buffer src/ring_buf.cpp)
