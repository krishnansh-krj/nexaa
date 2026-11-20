#pragma once

// NEXA DSA #4 - Doubly Linked List
// Incident event timeline and sequential operational history.
// pushBack / pushFront O(1), search O(n), ordered insert O(n).

#include <cstddef>

namespace nexa::dsa
{

template <typename T>
class LinkedList
{
public:
    struct Node
    {
        T value;
        Node* prev;
        Node* next;
    };

private:
    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    std::size_t size_ = 0;

public:
    LinkedList() = default;
    ~LinkedList() { clear(); }

    LinkedList(const LinkedList&) = delete;
    LinkedList& operator=(const LinkedList&) = delete;

    void pushBack(const T& value)
    {
        Node* node = new Node{value, tail_, nullptr};
        if (tail_) tail_->next = node; else head_ = node;
        tail_ = node;
        ++size_;
    }

    void pushFront(const T& value)
    {
        Node* node = new Node{value, nullptr, head_};
        if (head_) head_->prev = node; else tail_ = node;
        head_ = node;
        ++size_;
    }

    // Keeps the list ordered by `less` (stable: equal keys keep arrival order).
    template <typename Less>
    void insertOrdered(const T& value, Less less)
    {
        Node* cursor = tail_;
        while (cursor != nullptr && less(value, cursor->value))
            cursor = cursor->prev;

        if (cursor == nullptr)
        {
            pushFront(value);
            return;
        }

        Node* node = new Node{value, cursor, cursor->next};
        if (cursor->next) cursor->next->prev = node; else tail_ = node;
        cursor->next = node;
        ++size_;
    }

    template <typename Predicate>
    const T* find(Predicate match) const
    {
        for (Node* n = head_; n != nullptr; n = n->next)
            if (match(n->value))
                return &n->value;
        return nullptr;
    }

    template <typename Predicate>
    bool removeFirst(Predicate match)
    {
        for (Node* n = head_; n != nullptr; n = n->next)
        {
            if (!match(n->value))
                continue;
            if (n->prev) n->prev->next = n->next; else head_ = n->next;
            if (n->next) n->next->prev = n->prev; else tail_ = n->prev;
            delete n;
            --size_;
            return true;
        }
        return false;
    }

    template <typename Visitor>
    void forEach(Visitor visit) const
    {
        for (Node* n = head_; n != nullptr; n = n->next)
            visit(n->value);
    }

    template <typename Visitor>
    void forEachReverse(Visitor visit) const
    {
        for (Node* n = tail_; n != nullptr; n = n->prev)
            visit(n->value);
    }

    const Node* head() const { return head_; }
    const Node* tail() const { return tail_; }
    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    void clear()
    {
        while (head_ != nullptr)
        {
            Node* next = head_->next;
            delete head_;
            head_ = next;
        }
        tail_ = nullptr;
        size_ = 0;
    }
};

} // namespace nexa::dsa
