# 假设你使用 Google Test 库
# 需要提前安装 Google Test 或者下载其源代码，并指定其路径

# 找到 Google Test
find_package(GTest REQUIRED)

# 将测试源文件添加到测试目标中
file(GLOB TEST_SOURCES "tests/*.cpp")

# 添加测试可执行文件
add_executable(run_tests ${TEST_SOURCES})

# 链接 Google Test 库和 pthread 库
target_link_libraries(run_tests GTest::GTest GTest::Main pthread)

# 添加测试到 CTest
enable_testing()
add_test(NAME ring_buffer_tests COMMAND run_tests)
