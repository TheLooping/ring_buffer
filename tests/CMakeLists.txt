# 查找GTest包
find_package(GTest REQUIRED)

# 添加测试可执行文件
add_executable(ring_buf_test ring_buf_test.cc)

# 链接GTest库
target_link_libraries(ring_buf_test GTest::GTest GTest::Main)

# 添加测试
add_test(NAME RingBufferTest COMMAND ring_buf_test)
