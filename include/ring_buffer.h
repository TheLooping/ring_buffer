#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <vector>
#include <atomic>
#include <cstddef>
#include <memory>
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <thread>
#include <cstdlib>

#define RING_BUF_SUCCESS 0
#define RING_BUF_PUSH_ERROR -1
#define RING_BUF_POP_TIMEOUT -2
#define RING_BUF_POP_ERROR -3
#define RING_BUF_IN_EXPANDING -4
#define RING_BUF_CAP_EXCEED_UPPER_BOUND -5
#define RING_BUF_CAP_NOT_ENOUGH -6

namespace ring_buf_
{
    using namespace std::chrono;
    class Semaphore
    {
    public:
        explicit Semaphore() {}

        // 阻塞等待信号量
        void wait()
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock); // 等待直到 count > 0
        }

        // 释放信号量并唤醒所有等待的线程
        void signal_all()
        {
            std::lock_guard<std::mutex> lock(mtx);
            cv.notify_all(); // 唤醒所有等待中的线程
        }

    private:
        std::mutex mtx;
        std::condition_variable cv;
    };

    template <typename T>
    class RingBuf
    {
    private:
        Semaphore sem_;

    public:
        explicit RingBuf(uint32_t capacity);
        RingBuf(const RingBuf &) = delete;
        RingBuf &operator=(const RingBuf &) = delete;

        int Push(const T *data, uint32_t size);        
        int Push(const std::vector<T>& data);
        int Pop(T *data, uint32_t size);
        int Pop(std::vector<T>& data);
        void PrintRing();

    private:
        enum
        {
            kMinSize = 16
        };
        enum
        {
            kMaxSize = 1024
        };
        enum
        {
            burstMaxSize = 16
        };
        enum
        {
            expansionFactor = 2
        };
        enum
        {
            popTimeOutSeconds = 2
        };
        static constexpr double expansionThreshold = 0.9;

        inline bool IsEmpty() const { return size_.load() == 0; }
        inline bool IsNearlyFull() const { return size_.load() > capacity_threshold_.load(); }
        int rearrange_circular_array(const T *old_array, T *new_array, uint32_t old_start, uint32_t old_end, uint32_t old_capacity, uint32_t new_capacity, uint32_t new_start);
        int reset_circular_array(T *array, uint32_t start, uint32_t end, uint32_t capacity);
        void SimulateTimeWait();
        int Expand();

        std::atomic<uint32_t> size_;
        std::atomic<uint32_t> capacity_;
        std::atomic<uint32_t> capacity_threshold_;

        std::atomic<uint32_t> prod_head_;
        std::atomic<uint32_t> prod_tail_;
        std::atomic<uint32_t> cons_head_;
        std::atomic<uint32_t> cons_tail_;
        std::atomic<int> workers_;
        // mutex
        std::atomic<bool> in_expanding_;
        std::atomic<bool> unique_expand_flag_;
        //  std::atomic<bool> working_done_;
        // 普通数组
        std::unique_ptr<T[]> buffer_;
    };

    template <typename T>
    RingBuf<T>::RingBuf(uint32_t capacity)
    {
        if (capacity < kMinSize)
        {
            capacity = kMinSize;
        }
        else if (capacity > kMaxSize)
        {
            capacity = kMaxSize;
        }
        capacity_ = capacity;
        size_.store(0);
        prod_head_.store(0);
        prod_tail_.store(0);
        cons_head_.store(0);
        cons_tail_.store(0);
        in_expanding_.store(false);
        unique_expand_flag_.store(false);
        workers_.store(0);
        capacity_threshold_.store(capacity * expansionThreshold);
        buffer_ = std::unique_ptr<T[]>(new T[capacity]);
    }

    template <typename T>
    int RingBuf<T>::Push(const T *data, uint32_t size)
    {
        std::cout << "Push test ------------->>>>>>>>>>>>" << std::endl;
        while (in_expanding_.load())
        {
            sem_.wait();
        }
        size_.fetch_add(size);
        while (IsNearlyFull() || size_.load() > capacity_.load())
        {
            int new_cap = Expand();
            if (new_cap > kMaxSize * expansionFactor)
            {
                break;
            }
        }
        workers_.fetch_add(1);
        uint32_t old_prod_head, new_prod_head;
        do
        {
            old_prod_head = prod_head_.load();
            new_prod_head = (old_prod_head + size) % capacity_.load();
        } while (!prod_head_.compare_exchange_weak(old_prod_head, new_prod_head));

        while (prod_tail_.load() != old_prod_head)
        {
            // std::this_thread::yield();
            continue;
        }
        // 从 prod_tail_ 处开始写入数据
        // std::copy(data, data + size, buffer_ + prod_tail_.load());
        T *raw_buffer = buffer_.get();
        rearrange_circular_array(data, raw_buffer, 0, size, size, capacity_.load(), old_prod_head);
        // SimulateTimeWait();
        // prod_tail_.store((prod_tail_.load() + size) % capacity_.load());
        uint32_t expected_prod_tail, desire_prod_tail;
        do
        {
            expected_prod_tail = prod_tail_.load();
            desire_prod_tail = (expected_prod_tail + size) % capacity_.load();
        } while (!prod_tail_.compare_exchange_weak(expected_prod_tail, desire_prod_tail));

        uint32_t final_push_size = desire_prod_tail > expected_prod_tail ? (desire_prod_tail - expected_prod_tail) : (capacity_.load() - expected_prod_tail + desire_prod_tail);
        workers_.fetch_sub(1);
        if (final_push_size != size)
        {
            return RING_BUF_PUSH_ERROR;
        }
        std::cout << "Push end <<<<<<<<<<<<<<<-------------" << std::endl;
        return size;
    }

    template <typename T>
    int RingBuf<T>::Push(const std::vector<T>& data) {
        return Push(data.data(), data.size());
    }

    template <typename T>
    int RingBuf<T>::Pop(T *data, uint32_t size)
    {
        // std::chrono::steady_clock::now();
        auto start = steady_clock::now();
        while (in_expanding_.load())
        {
            sem_.wait();
        }
        uint32_t pop_max_size = prod_tail_.load() > cons_head_.load() ? (prod_tail_.load() - cons_head_.load()) : (capacity_.load() + prod_tail_.load() - cons_head_.load());
        int fact_pop_size = std::min(pop_max_size, size);
        while (fact_pop_size <= 0)
        {
            if (duration_cast<seconds>(steady_clock::now() - start).count() > popTimeOutSeconds)
            {
                return RING_BUF_POP_TIMEOUT;
            }
            std::this_thread::yield();
        }
        workers_.fetch_add(1);
        uint32_t old_cons_head, new_cons_head;
        do
        {
            old_cons_head = cons_head_.load();
            new_cons_head = (old_cons_head + fact_pop_size) % capacity_.load();
        } while (!cons_head_.compare_exchange_weak(old_cons_head, new_cons_head));

        // 从 cons_head_ 处开始读取数据
        T *raw_buffer = buffer_.get();
        int copy_num = rearrange_circular_array(raw_buffer, data, old_cons_head, new_cons_head, capacity_.load(), size, 0);
        reset_circular_array(raw_buffer, old_cons_head, new_cons_head, capacity_.load());
        // SimulateTimeWait();
        if (copy_num != fact_pop_size)
        {
            std::cout << "maybe wrong pop. desire fact_pop_size:" << fact_pop_size << "copy_num" << copy_num << std::endl;
            return RING_BUF_POP_ERROR;
        }


        uint32_t expected_cons_tail, desire_cons_tail;
        do
        {
            // expected_cons_tail = cons_tail_.load();
            expected_cons_tail = old_cons_head;
            desire_cons_tail = (expected_cons_tail + fact_pop_size) % capacity_.load();
        } while (!cons_tail_.compare_exchange_weak(expected_cons_tail, desire_cons_tail));

        uint32_t final_pop_size = desire_cons_tail > expected_cons_tail ? (desire_cons_tail - expected_cons_tail) : (capacity_.load() - expected_cons_tail + desire_cons_tail);
        size_.fetch_sub(fact_pop_size);
        workers_.fetch_sub(1);
        if (final_pop_size != fact_pop_size)
        {
            return RING_BUF_POP_ERROR;
        }
        return fact_pop_size;
    }

    template <typename T>
    int RingBuf<T>::Pop(std::vector<T>& data) {
        return Pop(data.data(), data.size());
    }
    template <typename T>
    int RingBuf<T>::Expand()
    {
        std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>> Expand start <<<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl;
        bool expected_state = false;
        if (!unique_expand_flag_.compare_exchange_weak(expected_state, true))
        {
            // 值为true，当前时刻有其他线程正在扩容
            std::cout << "Another thread is being expanded." << std::endl;
            return RING_BUF_IN_EXPANDING;
        }
        int new_capcity;
        if (capacity_.load() >= kMaxSize)
        {
            std::cout << "The buffer capacity exceeds the upper bound. " << std::endl;
            return RING_BUF_CAP_EXCEED_UPPER_BOUND;
        }
        else
        {
            new_capcity = capacity_.load() * expansionFactor < kMaxSize ? capacity_.load() * expansionFactor : kMaxSize;
        }

        std::unique_ptr<T[]> new_buffer(new T[new_capcity]);
        uint32_t cur_prod_tail, cur_cons_head, cur_size_1, cur_size_2;
        cur_cons_head = cons_head_.load();
        cur_prod_tail = prod_tail_.load();
        // 提前拷贝部分数据
        T *raw_old_buffer = buffer_.get();
        T *raw_new_buffer = new_buffer.get();
        cur_size_1 = rearrange_circular_array(raw_old_buffer, raw_new_buffer, cur_cons_head, cur_prod_tail, capacity_.load(), new_capcity, 0);

        expected_state = false;
        if (!in_expanding_.compare_exchange_weak(expected_state, true))
        {
            // 值为true，当前时刻有其他线程正在扩容
            std::cout << "Another thread is being expanded." << std::endl;
            return RING_BUF_IN_EXPANDING;
        }
        while (workers_.load() != 0)
        {
            std::this_thread::yield();
        }
        // 当前只有该线程在扩容，其他线程均被阻塞
        // 从 cur_prod_tail（提前复制的末尾） 到最新的 prod_head_
        cur_size_2 = rearrange_circular_array(raw_old_buffer, raw_new_buffer, cur_prod_tail, prod_head_.load(), capacity_.load(), new_capcity, cur_size_1);
        size_.store(cur_size_1 + cur_size_2);
        capacity_.store(new_capcity);
        capacity_threshold_.store(new_capcity * expansionThreshold);
        cons_head_.store(0);
        cons_tail_.store(0);
        prod_head_.store(cur_size_1 + cur_size_2);
        prod_tail_.store(cur_size_1 + cur_size_2);

        buffer_ = std::move(new_buffer);

        bool expected = true;
        if (!unique_expand_flag_.compare_exchange_weak(expected, false))
        {
            std::cout << "unique_expand_flag_ from true to false, wrong!" << std::endl;
        }
        expected = true;
        if (!in_expanding_.compare_exchange_weak(expected, false))
        {
            std::cout << "in_expanding_ from true to false, wrong!" << std::endl;
        }
        sem_.signal_all();
        std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>> Expand end <<<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl;
        return capacity_.load();
    }

    template <typename T>
    int RingBuf<T>::rearrange_circular_array(const T *old_array, T *new_array, uint32_t old_start, uint32_t old_end, uint32_t old_capacity, uint32_t new_capacity, uint32_t new_start)
    {
        // 计算数组中待复制的元素个数
        int size = (old_end >= old_start) ? (old_end - old_start) : (old_capacity - old_start + old_end);

        // 如果新数组的容量不足以容纳旧数组中的元素，则进行边界检查
        if (size > new_capacity)
        {
            return RING_BUF_CAP_NOT_ENOUGH; // 返回 -1 表示新数组的容量不足
        }

        // 如果数组是分段存储的
        if (old_start <= old_end)
        {
            // 直接复制从 old_start 到 old_end 的数据到新数组的指定起始位置
            if (new_start + size <= new_capacity)
            {
                std::copy(old_array + old_start, old_array + old_end, new_array + new_start);
            }
            else
            {
                uint32_t first_part_size = new_capacity - new_start;
                uint32_t second_part_size = size - first_part_size;
                std::copy(old_array + old_start, old_array + old_start + first_part_size, new_array + new_start);
                std::copy(old_array + old_start + first_part_size, old_array + old_end, new_array);
            }
        }
        else
        {
            if (new_start + size <= new_capacity)
            {
                // 先复制从 old_start 到 old_capacity 的部分到新数组的指定起始位置
                std::copy(old_array + old_start, old_array + old_capacity, new_array + new_start);
                // 再复制从 0 到 old_end 的部分到新数组的指定位置
                std::copy(old_array, old_array + old_end, new_array + new_start + (old_capacity - old_start));
            }
            else
            {
                std::unique_ptr<T[]> temp_array(new T[size]);
                T *raw_temp_array = temp_array.get();
                rearrange_circular_array(old_array, raw_temp_array, old_start, old_end, old_capacity, size, 0);
                rearrange_circular_array(raw_temp_array, new_array, 0, size, size, new_capacity, new_start);
            }
        }

        return size; // 返回元素个数
    }

    template <typename T>
    int RingBuf<T>::reset_circular_array(T *array, uint32_t start, uint32_t end, uint32_t capacity){
        if (start <= end)
        {
            std::fill(array + start, array + end, 0);
        }
        else
        {
            std::fill(array + start, array + capacity, 0);
            std::fill(array, array + end, 0);
        }
        return 0;
    }


    template <typename T>
    void RingBuf<T>::SimulateTimeWait()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(rand() & 0x03ff)); // 模拟生产者生产数据的时间
    }
    template <typename T>
    void RingBuf<T>::PrintRing()
    {
        T *raw_buffer = buffer_.get();
        uint32_t buf_capacity = capacity_.load();

        // 打印整个缓冲区内容
        std::cout <<  "********************* PrintRing *********************" << std::endl;
        for (uint32_t i = 0; i < buf_capacity; ++i)
        {
            std::cout << raw_buffer[i];
            if (cons_tail_.load() == i)
            {
                std::cout << " <- cons_tail_";
            }
            if (cons_head_.load() == i)
            {
                std::cout << " <- cons_head_";
            }
            if (prod_tail_.load() == i)
            {
                std::cout << " <- prod_tail_";
            }
            if (prod_head_.load() == i)
            {
                std::cout << " <- prod_head_";
            }
            std::cout << std::endl;
        }
        std::cout <<  "************************ END ************************\n" << std::endl;
    }

}

#endif // RING_BUFFER_H