#include <iostream>
#include <thread>
#include "ring_buffer.h"

std::mutex print_mutex;

void Producer(ring_buf_::RingBuf<int> &buf, int id)
{
    for (int i = 100; i < 200; ++i)
    {
        int data = i + id * 1000;  // 为每个生产者生成不同的数据
        int ret;
        while ((ret = buf.Push(&data, 1)) != 1)
        {
            std::cout << "Producer " << id << " push failed " << ret << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 等待并重试
        }
        
        std::cout << "Producer " << id << " pushed " << data << std::endl;

        {
            std::lock_guard<std::mutex> lock(print_mutex);
            buf.PrintRing();
        }
        std::this_thread::sleep_for(std::chrono::seconds(1)); // 等待并重试
    }
}

void Consumer(ring_buf_::RingBuf<int> &buf, int id)
{
    int data;
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1500)); // 等待并重试
        int result = buf.Pop(&data, 1);
        if (result == 1)
        {
            std::cout << "Consumer " << id << " consumed: " << data << std::endl;
        }
        else if (result == 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 等待并重试
            std::cout << "Consumer " << id << " pop failed or buffer empty" << std::endl;
        }
        else
        {
            std::cout << "Consumer " << id << " pop failed or buffer empty" << std::endl;
            break;
        }
    }
}

int main()
{
    constexpr uint32_t buffer_size = 16;
    ring_buf_::RingBuf<int> buf(buffer_size);
    std::cout << "Start test" << std::endl;

    // 启动多个生产者和消费者线程
    std::thread producer_thread1(Producer, std::ref(buf), 1);
    std::thread producer_thread2(Producer, std::ref(buf), 2);
    std::thread producer_thread3(Producer, std::ref(buf), 3);

    std::thread consumer_thread1(Consumer, std::ref(buf), 1);
    std::thread consumer_thread2(Consumer, std::ref(buf), 2);

    producer_thread1.join();
    producer_thread2.join();
    producer_thread3.join();
    consumer_thread1.join();
    consumer_thread2.join();

    return 0;
}
