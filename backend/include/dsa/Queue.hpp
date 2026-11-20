#pragma once

// NEXA DSA #3 - Queue (circular array, FIFO)
// Sequential emergency-request processing (first come, first served).
// enqueue / dequeue amortised O(1).

#include <cstddef>
#include <utility>

namespace nexa::dsa
{

template <typename T>
class Queue
{
private:
    T* buffer_ = nullptr;
    std::size_t capacity_ = 0;
    std::size_t head_ = 0;
    std::size_t size_ = 0;

    void grow()
    {
        const std::size_t newCapacity = capacity_ == 0 ? 8 : capacity_ * 2;
        T* replacement = new T[newCapacity];
        for (std::size_t i = 0; i < size_; ++i)
            replacement[i] = std::move(buffer_[(head_ + i) % capacity_]);
        delete[] buffer_;
        buffer_ = replacement;
        capacity_ = newCapacity;
        head_ = 0;
    }

public:
    Queue() = default;
    ~Queue() { delete[] buffer_; }

    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    void enqueue(const T& value)
    {
        if (size_ == capacity_)
            grow();
        buffer_[(head_ + size_) % capacity_] = value;
        ++size_;
    }

    bool dequeue(T& output)
    {
        if (size_ == 0)
            return false;
        output = std::move(buffer_[head_]);
        head_ = (head_ + 1) % capacity_;
        --size_;
        return true;
    }

    const T* front() const { return size_ == 0 ? nullptr : &buffer_[head_]; }

    bool empty() const { return size_ == 0; }
    std::size_t size() const { return size_; }
};

} // namespace nexa::dsa
