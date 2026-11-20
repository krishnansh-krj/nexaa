#pragma once

// NEXA DSA #16 - Red-Black Tree (advanced comparative index)
// Balanced BST with colour invariants (CLRS): height <= 2 log2(n + 1).
// insert / find O(log n). Used side by side with the BST and AVL indexes
// to compare tree heights on the same incident keys.

#include <cstddef>

namespace nexa::dsa
{

template <typename Key, typename Value>
class RedBlackTree
{
private:
    enum class Colour { Red, Black };

    struct Node
    {
        Key key;
        Value value;
        Colour colour = Colour::Red;
        Node* left = nullptr;
        Node* right = nullptr;
        Node* parent = nullptr;
    };

    Node* root_ = nullptr;
    std::size_t size_ = 0;

    void rotateLeft(Node* x)
    {
        Node* y = x->right;
        x->right = y->left;
        if (y->left) y->left->parent = x;
        y->parent = x->parent;
        if (!x->parent) root_ = y;
        else if (x == x->parent->left) x->parent->left = y;
        else x->parent->right = y;
        y->left = x;
        x->parent = y;
    }

    void rotateRight(Node* x)
    {
        Node* y = x->left;
        x->left = y->right;
        if (y->right) y->right->parent = x;
        y->parent = x->parent;
        if (!x->parent) root_ = y;
        else if (x == x->parent->right) x->parent->right = y;
        else x->parent->left = y;
        y->right = x;
        x->parent = y;
    }

    void fixInsert(Node* z)
    {
        while (z->parent && z->parent->colour == Colour::Red)
        {
            Node* grand = z->parent->parent;
            if (z->parent == grand->left)
            {
                Node* uncle = grand->right;
                if (uncle && uncle->colour == Colour::Red)
                {
                    z->parent->colour = Colour::Black;
                    uncle->colour = Colour::Black;
                    grand->colour = Colour::Red;
                    z = grand;
                }
                else
                {
                    if (z == z->parent->right)
                    {
                        z = z->parent;
                        rotateLeft(z);
                    }
                    z->parent->colour = Colour::Black;
                    grand->colour = Colour::Red;
                    rotateRight(grand);
                }
            }
            else
            {
                Node* uncle = grand->left;
                if (uncle && uncle->colour == Colour::Red)
                {
                    z->parent->colour = Colour::Black;
                    uncle->colour = Colour::Black;
                    grand->colour = Colour::Red;
                    z = grand;
                }
                else
                {
                    if (z == z->parent->left)
                    {
                        z = z->parent;
                        rotateRight(z);
                    }
                    z->parent->colour = Colour::Black;
                    grand->colour = Colour::Red;
                    rotateLeft(grand);
                }
            }
        }
        root_->colour = Colour::Black;
    }

    static void destroy(Node* n)
    {
        if (!n) return;
        destroy(n->left);
        destroy(n->right);
        delete n;
    }

    static int heightOf(const Node* n)
    {
        if (!n) return 0;
        const int l = heightOf(n->left), r = heightOf(n->right);
        return 1 + (l > r ? l : r);
    }

    // Returns black-height, or -1 if any invariant is broken.
    static int check(const Node* n)
    {
        if (!n) return 1;
        if (n->colour == Colour::Red &&
            ((n->left && n->left->colour == Colour::Red) ||
             (n->right && n->right->colour == Colour::Red)))
            return -1;
        if (n->left && !(n->left->key < n->key)) return -1;
        if (n->right && !(n->key < n->right->key)) return -1;
        const int l = check(n->left), r = check(n->right);
        if (l < 0 || r < 0 || l != r) return -1;
        return l + (n->colour == Colour::Black ? 1 : 0);
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
    RedBlackTree() = default;
    ~RedBlackTree() { destroy(root_); }

    RedBlackTree(const RedBlackTree&) = delete;
    RedBlackTree& operator=(const RedBlackTree&) = delete;

    bool insert(const Key& key, const Value& value)
    {
        Node* parent = nullptr;
        Node* cur = root_;
        while (cur)
        {
            parent = cur;
            if (key < cur->key) cur = cur->left;
            else if (cur->key < key) cur = cur->right;
            else { cur->value = value; return false; }
        }
        Node* z = new Node{key, value};
        z->parent = parent;
        if (!parent) root_ = z;
        else if (key < parent->key) parent->left = z;
        else parent->right = z;
        ++size_;
        fixInsert(z);
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

    template <typename Visitor>
    void inorder(Visitor visit) const { inorder(root_, visit); }

    bool isValid() const
    {
        if (root_ && root_->colour != Colour::Black) return false;
        return check(root_) > 0;
    }

    int height() const { return heightOf(root_); }
    std::size_t size() const { return size_; }
};

} // namespace nexa::dsa
