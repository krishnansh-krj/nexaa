#pragma once

// NEXA DSA #2 - Stack (singly linked, LIFO)
// Recent operational action history; top = most recent action.
// push / pop / peek O(1).

#include <cstddef>

namespace nexa::dsa
{

template <typename T>
class Stack
{
private:
    struct Node
    {
        T value;
        Node* next;
    };

    Node* top_ = nullptr;
    std::size_t size_ = 0;

public:
    Stack() = default;
    ~Stack() { clear(); }

    Stack(const Stack&) = delete;
    Stack& operator=(const Stack&) = delete;

    void push(const T& value)
    {
        top_ = new Node{value, top_};
        ++size_;
    }

    bool pop(T& output)
    {
        if (top_ == nullptr)
            return false;
        Node* old = top_;
        output = old->value;
        top_ = old->next;
        delete old;
        --size_;
        return true;
    }

    const T* peek() const { return top_ ? &top_->value : nullptr; }

    bool empty() const { return top_ == nullptr; }
    std::size_t size() const { return size_; }

    void clear()
    {
        while (top_ != nullptr)
        {
            Node* next = top_->next;
            delete top_;
            top_ = next;
        }
        size_ = 0;
    }

    // Visit top -> bottom without modifying the stack.
    template <typename Visitor>
    void forEachFromTop(Visitor visit) const
    {
        for (Node* n = top_; n != nullptr; n = n->next)
            visit(n->value);
    }
};

} // namespace nexa::dsa
