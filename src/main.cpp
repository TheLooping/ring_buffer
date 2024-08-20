#include <iostream>
#include "ring_buffer.h"

void Producer(ring_buf_::RingBuf<int> &buf)
{
    for (int i = 100; i < 200; ++i)
    {
        int data = i;
        int ret;
        while ((ret = buf.Push(&data, 1)) != 1)
        {
            std::cout << ret << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 等待并重试
        }
        std::cout << "push " << data << std::endl;

        buf.PrintRing();
        std::this_thread::sleep_for(std::chrono::seconds(1)); // 等待并重试
    }
}

void Consumer(ring_buf_::RingBuf<int> &buf)
{
    int data;
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(2)); // 等待并重试
        int result = buf.Pop(&data, 1);
        if (result == 1)
        {
            std::cout << "Consumed: " << data << std::endl;
        }
        else if (result == 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 等待并重试
        }
        else
        {
            std::cout << "Pop failed or buffer empty" << std::endl;
            break;
        }
    }
}

int main()
{
    constexpr uint32_t buffer_size = 16;
    ring_buf_::RingBuf<int> buf(buffer_size);
    std::cout << "start test" << std::endl;

    std::thread producer_thread(Producer, std::ref(buf));
    std::thread consumer_thread(Consumer, std::ref(buf));

    producer_thread.join();
    consumer_thread.join();

    return 0;
}
