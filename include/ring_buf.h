#ifndef RING_BUF_H
#define RING_BUF_H

// 线程安全的多生产者多消费者的动态扩容无锁环形缓冲区
#include <vector>
#include <atomic>
#include <cstddef>
#include <memory>
#include <iostream>
#include <errno.h>
#define CACHE_LINE_SIZE 64
namespace ring_buf {
    template <typename T> class RingBuf
    {
        private:
            struct Node;
        public:
            explicit RingBuf(uint32_t capacity);
            RingBuf(const RingBuf&) = delete;
            RingBuf& operator=(const RingBuf&) = delete;

            // Push data
            // Requires: size() < capacity(); atomic operation.
            int push(const T* data);

            // Batch push
            // Requires: size() + data.size() < capacity(); atomic operation.
            int push(const T* data, uint32_t size);

            // Pop data
            // Requires: size() > 0; atomic operation.
            int pop(std::shared_ptr<T[]>& data);

            // Batch pop
            // Requires: size() >= size; atomic operation.
            int pop(std::shared_ptr<T[]>& data, uint32_t size);

            // Try push data
            int try_push(const T* data);

            // Try batch push
            int try_push(const T* data, uint32_t size);

            // Try pop data
            int try_pop(std::shared_ptr<T[]>& data);

            // Try batch pop
            int try_pop(std::shared_ptr<T[]>& data, uint32_t size);

        private:
            enum { kMinSize = 16 };
            enum { kMaxSize = 1204 };
            enum { burstMaxSize = 16 };
            enum { expansionFactor = 2 };
            static constexpr double expansionThreshold = 0.1;
            inline bool IsEmpty() const { return size_.load() == 0; }
            inline bool IsFull() const { return (capacity_.load() - size_.load()) < capacity_.load() * expansionThreshold; }
            inline uint32_t Next(uint32_t current, uint32_t offset) const { return (current + offset) % capacity_.load(); }
            
            // Expand the capacity of the ring buffer
            // Requires: capacity() - size() < expansion_threshold * capacity(); atomic operation.
            void TryExpand();

            alignas(CACHE_LINE_SIZE) std::atomic<uint32_t> size_; // 当前个数
            alignas(CACHE_LINE_SIZE) std::atomic<uint32_t> capacity_; // 总容量
            alignas(CACHE_LINE_SIZE) std::atomic<uint32_t> prod_head_;
            alignas(CACHE_LINE_SIZE) std::atomic<uint32_t> prod_tail_;
            alignas(CACHE_LINE_SIZE) std::atomic<uint32_t> cons_head_;
            alignas(CACHE_LINE_SIZE) std::atomic<uint32_t> cons_tail_;            
            alignas(CACHE_LINE_SIZE) std::atomic<bool> is_in_expanding_;
            std::shared_ptr<std::vector<Node>> queue_;
    };

    template <typename T> struct RingBuf<T>::Node {
        explicit Node(const T& data) : data(data) {}
        T data;
        void SetData(const T& data) { this->data = data; }
    };

    template <typename T> 
    RingBuf<T>::RingBuf(uint32_t capacity) 
        : size_(0),
          prod_head_(1),
          prod_tail_(1),
          cons_head_(0),
          cons_tail_(0),
          is_in_expanding_(false),
          capacity_(capacity < kMinSize ? kMinSize : capacity) {
        if (capacity > kMaxSize) {
            perror("capacity is too large");
        }
        queue_ = std::make_shared<std::vector<Node>>();
        queue_->reserve(capacity);
    }
    
    template <typename T> 
    int RingBuf<T>::push(const T* data, uint32_t size) {
        while (is_in_expanding_.load()){
            continue;
        }
        while(IsFull() || size > capacity_.load() - size_.load()) {
            if (!is_in_expanding_.load()) {                
                TryExpand();
            }
        }
        uint32_t old_head;
        uint32_t new_head;
        do {
            old_head = prod_head_.load(std::memory_order_relaxed);
            new_head = Next(old_head, size);           
        } while(!prod_head_.compare_exchange_weak(old_head, new_head, std::memory_order_release, std::memory_order_relaxed));

        uint32_t push_count = 0;
        for (uint32_t i = 0; i < size; ++i) {
            Node* cur_node = &(*queue_)[Next(old_head, i)];
            cur_node->SetData(data[i]);
            ++push_count;
        }
        uint32_t expected_tail;
        do {
            expected_tail = old_head;         
        } while (!prod_tail_.compare_exchange_weak(expected_tail, new_head, std::memory_order_release, std::memory_order_relaxed)); 
        size_.fetch_add(size, std::memory_order_relaxed);
        return push_count;
    }

    template <typename T>
    int RingBuf<T>::push(const T* data) {
        return push(data, 1);
    }

    template <typename T>
    int RingBuf<T>::pop(std::shared_ptr<T[]>& data, uint32_t size) {
        while (IsEmpty()) {
            return 0;
        }        
        while (is_in_expanding_.load()) {
            continue;
        }

        uint32_t old_head;
        uint32_t new_head;
        do {
            old_head = cons_head_.load(std::memory_order_relaxed);
            new_head = Next(old_head, size);
        } while (!cons_head_.compare_exchange_weak(old_head, new_head, std::memory_order_release, std::memory_order_relaxed));

        size = std::min(size, size_.load(std::memory_order_relaxed));
        size = size > burstMaxSize ? burstMaxSize : size;

        if (data == nullptr) {
            data = std::shared_ptr<T[]>(new T[size]);
        }

        uint32_t pop_count = 0;
        for (uint32_t i = 0; i < size; ++i) {
            Node* cur_node = &(*queue_)[Next(old_head, i)];
            data[i] = cur_node->data;
            ++pop_count;
        }
        uint32_t expected_tail;
        do {
            expected_tail = old_head;
        }while (!cons_tail_.compare_exchange_weak(expected_tail, new_head, std::memory_order_release, std::memory_order_relaxed)); 
        size_.fetch_sub(size, std::memory_order_relaxed);
        return pop_count;
    }
        
    template <typename T>
    int RingBuf<T>::pop(std::shared_ptr<T[]>& data) {
        return pop(data, 1);
    }

    template <typename T>
    int RingBuf<T>::try_push(const T* data, uint32_t size) {
        if (is_in_expanding_.load()) {
            return 0; // Buffer is expanding, wait for the expanding to complete
        }
        return push(data, size); // Call the existing push method
    }

    template <typename T>
    int RingBuf<T>::try_push(const T* data) {
        return try_push(data, 1);
    }

    template <typename T>
    int RingBuf<T>::try_pop(std::shared_ptr<T[]>& data, uint32_t size) {
        if (IsEmpty()) {
            return 0; // Buffer is empty
        }
        return pop(data, size); // Call the existing pop method
    }

    template <typename T>
    int RingBuf<T>::try_pop(std::shared_ptr<T[]>& data) {
        return try_pop(data, 1);
    }

    template <typename T>
    void RingBuf<T>::TryExpand() {
        bool expected_flag = false;
        if (!is_in_expanding_.compare_exchange_weak(expected_flag, true, std::memory_order_release, std::memory_order_relaxed)) {
            return;
        }
        std::shared_ptr<std::vector<Node>> new_queue = std::make_shared<std::vector<Node>>();
        new_queue->reserve(capacity_.load() * expansionFactor);

        uint32_t prod_head, prod_tail, cons_head, cons_tail;
        // 等待当前生产者、消费者操作完成，并阻塞想要生产的线程
        do {
            prod_head = prod_head_.load(std::memory_order_relaxed);
            prod_tail = prod_tail_.load(std::memory_order_relaxed);
        } while (!(prod_tail == prod_head));

        // 复制数据
        uint32_t expand_len = new_queue->capacity() - capacity_.load();
        if (prod_tail_.load() > cons_head_.load()) {
            // 无环，连续，直接复制
            std::copy(queue_->begin(), queue_->end(), new_queue->begin());
        } else {
            // 有环，分开复制
            std::copy(queue_->begin(), queue_->begin() + prod_head_.load(), new_queue->begin());
            cons_tail = cons_tail_.load();
            uint32_t new_cons_tail = cons_tail + expand_len;
            std::copy(queue_->begin() + cons_tail, queue_->end(), new_queue->begin() + new_cons_tail);
        }

        do {
            cons_head = cons_head_.load();
            cons_tail = cons_tail_.load();
        } while (!(cons_head == cons_tail));

        // 更新属性，cons_head_、cons_tail_、capacity_
        do {
            cons_tail = cons_tail_.load();
            cons_head = cons_head_.load();
        } while ((!cons_tail_.compare_exchange_weak(cons_tail, cons_tail + expand_len))
                || (!cons_head_.compare_exchange_weak(cons_head, cons_head + expand_len)));
        capacity_.store(new_queue->capacity(), std::memory_order_release);
        queue_ = std::move(new_queue);
        is_in_expanding_.store(false, std::memory_order_release);
    }
}
#endif // RING_BUF_H
