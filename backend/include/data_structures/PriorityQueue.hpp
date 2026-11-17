
#pragma once

#include <cstddef>
#include <stdexcept>
#include <utility>

namespace nexa
{
template <typename T>
class PriorityQueue
{
private:
    T* heap = nullptr;
    std::size_t count = 0;
    std::size_t capacity = 0;

    static bool higherPriority(const T& a, const T& b)
    {
        return a > b;
    }

    void swapItems(std::size_t a, std::size_t b)
    {
        using std::swap;
        swap(heap[a], heap[b]);
    }

    void grow()
    {
        const std::size_t newCapacity =
            capacity == 0 ? 8 : capacity * 2;

        T* replacement = new T[newCapacity];

        for (std::size_t i = 0; i < count; ++i)
            replacement[i] = std::move(heap[i]);

        delete[] heap;
        heap = replacement;
        capacity = newCapacity;
    }

    void siftUp(std::size_t index)
    {
        while (index > 0)
        {
            const std::size_t parent = (index - 1) / 2;

            if (!higherPriority(heap[index], heap[parent]))
                break;

            swapItems(index, parent);
            index = parent;
        }
    }

    void siftDown(std::size_t index)
    {
        while (true)
        {
            const std::size_t left = 2 * index + 1;
            const std::size_t right = 2 * index + 2;
            std::size_t best = index;

            if (left < count &&
                higherPriority(heap[left], heap[best]))
                best = left;

            if (right < count &&
                higherPriority(heap[right], heap[best]))
                best = right;

            if (best == index)
                break;

            swapItems(index, best);
            index = best;
        }
    }

public:
    PriorityQueue() = default;

    ~PriorityQueue()
    {
        delete[] heap;
    }

    PriorityQueue(const PriorityQueue&) = delete;
    PriorityQueue& operator=(const PriorityQueue&) = delete;

    PriorityQueue(PriorityQueue&& other) noexcept
        : heap(other.heap),
          count(other.count),
          capacity(other.capacity)
    {
        other.heap = nullptr;
        other.count = 0;
        other.capacity = 0;
    }

    PriorityQueue& operator=(PriorityQueue&& other) noexcept
    {
        if (this != &other)
        {
            delete[] heap;

            heap = other.heap;
            count = other.count;
            capacity = other.capacity;

            other.heap = nullptr;
            other.count = 0;
            other.capacity = 0;
        }

        return *this;
    }

    void push(const T& value)
    {
        if (count == capacity)
            grow();

        heap[count] = value;
        siftUp(count);
        ++count;
    }

    void push(T&& value)
    {
        if (count == capacity)
            grow();

        heap[count] = std::move(value);
        siftUp(count);
        ++count;
    }

    bool pop(T& output)
    {
        if (empty())
            return false;

        output = std::move(heap[0]);
        --count;

        if (count > 0)
        {
            heap[0] = std::move(heap[count]);
            siftDown(0);
        }

        return true;
    }

    const T* top() const
    {
        return empty() ? nullptr : &heap[0];
    }

    bool empty() const
    {
        return count == 0;
    }

    std::size_t size() const
    {
        return count;
    }

    void clear()
    {
        count = 0;
    }
};
}
