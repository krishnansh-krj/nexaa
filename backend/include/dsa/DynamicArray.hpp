#pragma once

// NEXA DSA #1 - Dynamic Array
// Resource registry / direct record management.
// Access O(1), push_back amortised O(1), linear search O(n).

#include <cstddef>
#include <stdexcept>
#include <utility>

namespace nexa::dsa
{

template <typename T>
class DynamicArray
{
private:
    T* data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t capacity_ = 0;

    void reallocate(std::size_t newCapacity)
    {
        T* replacement = new T[newCapacity];
        for (std::size_t i = 0; i < size_; ++i)
            replacement[i] = std::move(data_[i]);
        delete[] data_;
        data_ = replacement;
        capacity_ = newCapacity;
    }

public:
    DynamicArray() = default;

    ~DynamicArray() { delete[] data_; }

    DynamicArray(const DynamicArray& other)
    {
        if (other.size_ > 0)
        {
            reallocate(other.size_);
            for (std::size_t i = 0; i < other.size_; ++i)
                data_[i] = other.data_[i];
            size_ = other.size_;
        }
    }

    DynamicArray& operator=(const DynamicArray& other)
    {
        if (this != &other)
        {
            DynamicArray copy(other);
            swap(copy);
        }
        return *this;
    }

    DynamicArray(DynamicArray&& other) noexcept { swap(other); }

    DynamicArray& operator=(DynamicArray&& other) noexcept
    {
        if (this != &other)
        {
            delete[] data_;
            data_ = nullptr;
            size_ = capacity_ = 0;
            swap(other);
        }
        return *this;
    }

    void swap(DynamicArray& other) noexcept
    {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    void reserve(std::size_t capacity)
    {
        if (capacity > capacity_)
            reallocate(capacity);
    }

    void pushBack(const T& value)
    {
        if (size_ == capacity_)
            reallocate(capacity_ == 0 ? 8 : capacity_ * 2);
        data_[size_++] = value;
    }

    void pushBack(T&& value)
    {
        if (size_ == capacity_)
            reallocate(capacity_ == 0 ? 8 : capacity_ * 2);
        data_[size_++] = std::move(value);
    }

    bool popBack()
    {
        if (size_ == 0)
            return false;
        --size_;
        return true;
    }

    // O(n): shifts the tail left.
    bool removeAt(std::size_t index)
    {
        if (index >= size_)
            return false;
        for (std::size_t i = index + 1; i < size_; ++i)
            data_[i - 1] = std::move(data_[i]);
        --size_;
        return true;
    }

    T& at(std::size_t index)
    {
        if (index >= size_)
            throw std::out_of_range("DynamicArray index out of range");
        return data_[index];
    }

    const T& at(std::size_t index) const
    {
        if (index >= size_)
            throw std::out_of_range("DynamicArray index out of range");
        return data_[index];
    }

    T& operator[](std::size_t index) { return data_[index]; }
    const T& operator[](std::size_t index) const { return data_[index]; }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    void clear() { size_ = 0; }

    T* begin() { return data_; }
    T* end() { return data_ + size_; }
    const T* begin() const { return data_; }
    const T* end() const { return data_ + size_; }
};

} // namespace nexa::dsa
