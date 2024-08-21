
# 添加Google Test
include_directories(${GTEST_INCLUDE_DIRS})
link_directories(${GTEST_LIBRARY_DIRS})

# 添加测试文件
add_executable(ring_buffer_test ring_buffer_test.cpp)

# 链接Google Test库
target_link_libraries(ring_buffer_test ${GTEST_LIBRARIES} pthread)

# 添加测试到CTest
add_test(NAME RingBufferTest COMMAND ring_buffer_test)
