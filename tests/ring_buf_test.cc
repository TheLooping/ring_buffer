#include <gtest/gtest.h>
#include "ring_buf.h"

using namespace ring_buf;

class RingBufTest : public ::testing::Test {
protected:
    RingBuf<int> ring_buffer{16}; // 初始化一个容量为16的缓冲区
};
// TEST_F(RingBufTest, PushPopSingleElement) {
//     int value = 42;
//     int result;
//     EXPECT_EQ(ring_buffer.Push(&value), 1);
//     std::shared_ptr<int[]> data;
//     EXPECT_EQ(ring_buffer.Pop(data), 1);
//     EXPECT_EQ(data[0], 42);
// }

// TEST_F(RingBufTest, PushPopMultipleElements) {
//     int values[] = {1, 2, 3, 4, 5};
//     int size = sizeof(values) / sizeof(values[0]);
    
//     EXPECT_EQ(ring_buffer.Push(values, size), size);
    
//     std::shared_ptr<int[]> data;
//     EXPECT_EQ(ring_buffer.Pop(data, size), size);
    
//     for (int i = 0; i < size; ++i) {
//         EXPECT_EQ(data.get()[i], values[i]);
//     }
// }

// TEST_F(RingBufTest, TryPushPop) {
//     int value = 7;
    
//     EXPECT_EQ(ring_buffer.TryPush(&value), 1);
//     std::shared_ptr<int[]> data;
//     EXPECT_EQ(ring_buffer.TryPop(data), 1);
    
//     EXPECT_EQ(data.get()[0], 7);
// }

// TEST_F(RingBufTest, OverflowExpand) {
//     const int capacity = 16;
//     RingBuf<int> buffer(capacity);
    
//     int values[capacity + 1];
//     for (int i = 0; i < capacity + 1; ++i) {
//         values[i] = i;
//     }
    
//     EXPECT_EQ(buffer.Push(values, capacity + 1), capacity + 1);
    
//     std::shared_ptr<int[]> data;
//     EXPECT_EQ(buffer.Pop(data, capacity + 1), capacity + 1);
    
//     for (int i = 0; i < capacity + 1; ++i) {
//         EXPECT_EQ(data.get()[i], i);
//     }
// }


// TEST_F(RingBufTest, MultiplePushPop) {
//     int capacity = 16;
//     int values_size_1 = capacity - 4;
//     int values_size_2 = capacity -2;    
    
//     int values_1[values_size_1];
//     int values_2[values_size_2];

//     for (int i = 0; i < values_size_1; ++i) {
//         values_1[i] = i;
//     }

//     for (int i = 0; i < values_size_2; ++i) {
//         values_2[i] = i + values_size_1;
//     }
//     // Push and Pop 1st set of values
//     EXPECT_EQ(ring_buffer.Push(values_1, values_size_1), values_size_1);

//     std::shared_ptr<int[]> data;
//     EXPECT_EQ(ring_buffer.Pop(data, values_size_1), values_size_1);
    
//     for (int i = 0; i < values_size_1; ++i) {
//         EXPECT_EQ(data.get()[i], values_1[i]);
//     }
    
//     // Push and Pop 2nd set of values
//     EXPECT_EQ(ring_buffer.Push(values_2, values_size_2), values_size_2);
    
//     EXPECT_EQ(ring_buffer.Pop(data, values_size_2), values_size_2);
    
//     for (int i = 0; i < values_size_2; ++i) {
//         EXPECT_EQ(data.get()[i], values_2[i]);
//     }

// }

// TEST_F(RingBufTest, MultiplePushPopAndExpand) {
//     int capacity = 16;
//     int values_size_1 = capacity - 4;
//     int values_size_2 = capacity -2;    
    
//     int values_1[values_size_1];
//     int values_2[values_size_2];

//     for (int i = 0; i < values_size_1; ++i) {
//         values_1[i] = i;
//     }

//     for (int i = 0; i < values_size_2; ++i) {
//         values_2[i] = i + values_size_1;
//     }
//     // Push and Pop 1st set of values
//     EXPECT_EQ(ring_buffer.Push(values_1, values_size_1), values_size_1);

//     // Push and Pop 2nd set of values
//     EXPECT_EQ(ring_buffer.Push(values_2, values_size_2), values_size_2);
    
//     std::shared_ptr<int[]> data;
//     EXPECT_EQ(ring_buffer.Pop(data, values_size_1), values_size_1);
    
//     for (int i = 0; i < values_size_1; ++i) {
//         EXPECT_EQ(data.get()[i], values_1[i]);
//     }
    

//     EXPECT_EQ(ring_buffer.Pop(data, values_size_2), values_size_2);
    
//     for (int i = 0; i < values_size_2; ++i) {
//         EXPECT_EQ(data.get()[i], values_2[i]);
//     }

// }

TEST_F(RingBufTest, CrossMultiplePushPopAndExpand) {
    int capacity = 16;
    int values_size_1 = capacity - 2;
    int values_size_2 = capacity - 3;   
    int values_size_3 = capacity - 1;   
    
    int values_1[values_size_1];
    int values_2[values_size_2];
    int values_3[values_size_3];

    for (int i = 0; i < values_size_1; ++i) {
        values_1[i] = i;
    }

    for (int i = 0; i < values_size_2; ++i) {
        values_2[i] = i + values_size_1;
    }

    for (int i = 0; i < values_size_3; ++i) {
        values_3[i] = i + values_size_1 + values_size_2;
    }
    // Push and Pop 1st set of values
    EXPECT_EQ(ring_buffer.Push(values_1, values_size_1), values_size_1);
    
 

    // Push and Pop 2nd set of values
    EXPECT_EQ(ring_buffer.Push(values_2, values_size_2), values_size_2);

    // Push and Pop 3th set of values
    EXPECT_EQ(ring_buffer.Push(values_3, values_size_3), values_size_3);
    
    
    std::shared_ptr<int[]> data;
    EXPECT_EQ(ring_buffer.Pop(data, values_size_1), values_size_1);
    for (int i = 0; i < values_size_1; ++i) {
        EXPECT_EQ(data.get()[i], values_1[i]);
    }   

    EXPECT_EQ(ring_buffer.Pop(data, values_size_2), values_size_2);
    for (int i = 0; i < values_size_2; ++i) {
        EXPECT_EQ(data.get()[i], values_2[i]);
    }
    EXPECT_EQ(ring_buffer.Pop(data, values_size_3), values_size_3);
    for (int i = 0; i < values_size_3; ++i) {
        EXPECT_EQ(data.get()[i], values_3[i]);
    }
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}




