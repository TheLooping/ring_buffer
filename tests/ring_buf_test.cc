#include <gtest/gtest.h>
#include "ring_buf.h"

using namespace ring_buf;

class RingBufTest : public ::testing::Test {
protected:
    RingBuf<int> ring_buffer{16}; // 初始化一个容量为16的缓冲区
};

TEST_F(RingBufTest, PushPopSingleElement) {
    int value = 42;
    int result;
    
    EXPECT_EQ(ring_buffer.push(&value), 1);
    std::shared_ptr<int[]> data;
    EXPECT_EQ(ring_buffer.pop(data), 1);
    
    EXPECT_EQ(data.get()[0], 42);
}

TEST_F(RingBufTest, PushPopMultipleElements) {
    int values[] = {1, 2, 3, 4, 5};
    int size = sizeof(values) / sizeof(values[0]);
    
    EXPECT_EQ(ring_buffer.push(values, size), size);
    
    std::shared_ptr<int[]> data;
    EXPECT_EQ(ring_buffer.pop(data, size), size);
    
    for (int i = 0; i < size; ++i) {
        EXPECT_EQ(data.get()[i], values[i]);
    }
}

TEST_F(RingBufTest, TryPushPop) {
    int value = 7;
    
    EXPECT_EQ(ring_buffer.try_push(&value), 1);
    std::shared_ptr<int[]> data;
    EXPECT_EQ(ring_buffer.try_pop(data), 1);
    
    EXPECT_EQ(data.get()[0], 7);
}

TEST_F(RingBufTest, OverflowExpand) {
    const int capacity = 16;
    RingBuf<int> buffer(capacity);
    
    int values[capacity + 1];
    for (int i = 0; i < capacity + 1; ++i) {
        values[i] = i;
    }
    
    EXPECT_EQ(buffer.push(values, capacity + 1), capacity + 1);
    
    std::shared_ptr<int[]> data;
    EXPECT_EQ(buffer.pop(data, capacity + 1), capacity + 1);
    
    for (int i = 0; i < capacity + 1; ++i) {
        EXPECT_EQ(data.get()[i], i);
    }
}
