#include <gtest/gtest.h>
#include "ring_buffer.h"

// 测试Push和Pop功能
TEST(RingBufTest, BasicPushPop) {
    ring_buf_::RingBuf<int> ring_buf(16);

    int data_to_push[5] = {1, 2, 3, 4, 5};
    int data_popped[5] = {0};

    // 测试Push
    EXPECT_EQ(ring_buf.Push(data_to_push, 5), 5);

    // 测试Pop
    EXPECT_EQ(ring_buf.Pop(data_popped, 5), 5);

    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(data_popped[i], data_to_push[i]);
    }
}

// 测试环形数组扩展
TEST(RingBufTest, ExpandCapacity) {
    ring_buf_::RingBuf<int> ring_buf(4);

    int data_to_push[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    int data_popped[8] = {0};

    // 推入数据到环形缓冲区
    EXPECT_EQ(ring_buf.Push(data_to_push, 8), 8);

    // 弹出数据
    EXPECT_EQ(ring_buf.Pop(data_popped, 8), 8);

    for (int i = 0; i < 8; ++i) {
        EXPECT_EQ(data_popped[i], data_to_push[i]);
    }
}
