#pragma once

// NEXA DSA #14a - Binary Search Tree (unbalanced)
// Baseline index used to contrast with the AVL / Red-Black trees.
// insert / find / remove O(h): O(log n) average, O(n) worst (sorted input).

#include <cstddef>

namespace nexa::dsa
{

template <typename Key, typename Value>
class BinarySearchTree
{
private:
    struct Node
    {
        Key key;
        Value value;
        Node* left = nullptr;
        Node* right = nullptr;
    };

    Node* root_ = nullptr;
    std::size_t size_ = 0;

    static void destroy(Node* n)
    {
        if (!n) return;
        destroy(n->left);
        destroy(n->right);
        delete n;
    }

    static std::size_t heightOf(const Node* n)
    {
        if (!n) return 0;
        const std::size_t l = heightOf(n->left), r = heightOf(n->right);
        return 1 + (l > r ? l : r);
    }

    template <typename Visitor>
    static void inorder(const Node* n, Visitor& visit)
    {
        if (!n) return;
        inorder(n->left, visit);
        visit(n->key, n->value);
        inorder(n->right, visit);
    }

public:
    BinarySearchTree() = default;
    ~BinarySearchTree() { destroy(root_); }

    BinarySearchTree(const BinarySearchTree&) = delete;
    BinarySearchTree& operator=(const BinarySearchTree&) = delete;

    // Iterative to avoid deep recursion on degenerate (sorted) input.
    bool insert(const Key& key, const Value& value)
    {
        Node** link = &root_;
        while (*link != nullptr)
        {
            if (key < (*link)->key) link = &(*link)->left;
            else if ((*link)->key < key) link = &(*link)->right;
            else { (*link)->value = value; return false; }
        }
        *link = new Node{key, value};
        ++size_;
        return true;
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

    bool remove(const Key& key)
    {
        Node** link = &root_;
        while (*link && ((*link)->key < key || key < (*link)->key))
            link = key < (*link)->key ? &(*link)->left : &(*link)->right;
        if (*link == nullptr)
            return false;

        Node* target = *link;
        if (target->left && target->right)
        {
            // Replace with in-order successor.
            Node** succLink = &target->right;
            while ((*succLink)->left)
                succLink = &(*succLink)->left;
            Node* succ = *succLink;
            target->key = succ->key;
            target->value = succ->value;
            *succLink = succ->right;
            delete succ;
        }
        else
        {
            *link = target->left ? target->left : target->right;
            delete target;
        }
        --size_;
        return true;
    }

    template <typename Visitor>
    void inorder(Visitor visit) const { inorder(root_, visit); }

    std::size_t height() const { return heightOf(root_); }
    std::size_t size() const { return size_; }
};

} // namespace nexa::dsa
