#pragma once

// NEXA DSA #14b - AVL Tree (height-balanced BST)
// Incident / resource indexing with guaranteed O(log n) search,
// insert and delete, plus ordered range queries O(log n + k).

#include <cstddef>

namespace nexa::dsa
{

template <typename Key, typename Value>
class AVLTree
{
private:
    struct Node
    {
        Key key;
        Value value;
        Node* left = nullptr;
        Node* right = nullptr;
        int height = 1;
    };

    Node* root_ = nullptr;
    std::size_t size_ = 0;
    std::size_t rotations_ = 0;

    static int h(const Node* n) { return n ? n->height : 0; }
    static int balance(const Node* n) { return n ? h(n->left) - h(n->right) : 0; }
    static void update(Node* n)
    {
        const int l = h(n->left), r = h(n->right);
        n->height = 1 + (l > r ? l : r);
    }

    Node* rotateRight(Node* y)
    {
        Node* x = y->left;
        y->left = x->right;
        x->right = y;
        update(y);
        update(x);
        ++rotations_;
        return x;
    }

    Node* rotateLeft(Node* x)
    {
        Node* y = x->right;
        x->right = y->left;
        y->left = x;
        update(x);
        update(y);
        ++rotations_;
        return y;
    }

    Node* rebalance(Node* n)
    {
        update(n);
        const int b = balance(n);
        if (b > 1)
        {
            if (balance(n->left) < 0)
                n->left = rotateLeft(n->left);   // LR
            return rotateRight(n);               // LL
        }
        if (b < -1)
        {
            if (balance(n->right) > 0)
                n->right = rotateRight(n->right); // RL
            return rotateLeft(n);                 // RR
        }
        return n;
    }

    Node* insert(Node* n, const Key& key, const Value& value, bool& inserted)
    {
        if (!n)
        {
            inserted = true;
            return new Node{key, value};
        }
        if (key < n->key) n->left = insert(n->left, key, value, inserted);
        else if (n->key < key) n->right = insert(n->right, key, value, inserted);
        else { n->value = value; inserted = false; return n; }
        return rebalance(n);
    }

    Node* removeMin(Node* n, Node*& minNode)
    {
        if (!n->left)
        {
            minNode = n;
            return n->right;
        }
        n->left = removeMin(n->left, minNode);
        return rebalance(n);
    }

    Node* remove(Node* n, const Key& key, bool& removed)
    {
        if (!n) return nullptr;
        if (key < n->key) n->left = remove(n->left, key, removed);
        else if (n->key < key) n->right = remove(n->right, key, removed);
        else
        {
            removed = true;
            Node* left = n->left;
            Node* right = n->right;
            delete n;
            if (!right) return left;
            Node* successor = nullptr;
            Node* rest = removeMin(right, successor);
            successor->right = rest;
            successor->left = left;
            return rebalance(successor);
        }
        return rebalance(n);
    }

    static void destroy(Node* n)
    {
        if (!n) return;
        destroy(n->left);
        destroy(n->right);
        delete n;
    }

    template <typename Visitor>
    static void inorder(const Node* n, Visitor& visit)
    {
        if (!n) return;
        inorder(n->left, visit);
        visit(n->key, n->value);
        inorder(n->right, visit);
    }

    template <typename Visitor>
    static void range(const Node* n, const Key& lo, const Key& hi, Visitor& visit)
    {
        if (!n) return;
        if (lo < n->key) range(n->left, lo, hi, visit);
        if (!(n->key < lo) && !(hi < n->key)) visit(n->key, n->value);
        if (n->key < hi) range(n->right, lo, hi, visit);
    }

    static bool valid(const Node* n, int& height)
    {
        if (!n) { height = 0; return true; }
        int lh = 0, rh = 0;
        if (!valid(n->left, lh) || !valid(n->right, rh)) return false;
        if (n->left && !(n->left->key < n->key)) return false;
        if (n->right && !(n->key < n->right->key)) return false;
        const int diff = lh - rh;
        if (diff > 1 || diff < -1) return false;
        height = 1 + (lh > rh ? lh : rh);
        return height == n->height;
    }

public:
    AVLTree() = default;
    ~AVLTree() { destroy(root_); }

    AVLTree(const AVLTree&) = delete;
    AVLTree& operator=(const AVLTree&) = delete;

    bool insert(const Key& key, const Value& value)
    {
        bool inserted = false;
        root_ = insert(root_, key, value, inserted);
        if (inserted) ++size_;
        return inserted;
    }

    bool remove(const Key& key)
    {
        bool removed = false;
        root_ = remove(root_, key, removed);
        if (removed) --size_;
        return removed;
    }

    const Value* find(const Key& key) const
    {
        const Node* n = root_;
        while (n)
        {
            if (key < n->key) n = n->left;
            else if (n->key < key) n = n->right;
            else return &n->value;
        }
        return nullptr;
    }

    template <typename Visitor>
    void inorder(Visitor visit) const { inorder(root_, visit); }

    // Visits every key in [lo, hi] in ascending order.
    template <typename Visitor>
    void rangeQuery(const Key& lo, const Key& hi, Visitor visit) const { range(root_, lo, hi, visit); }

    bool isValid() const { int height = 0; return valid(root_, height); }
    int height() const { return h(root_); }
    std::size_t size() const { return size_; }
    std::size_t rotations() const { return rotations_; }
};

} // namespace nexa::dsa
