#pragma once

// NEXA DSA #13 - K-ary Tree (first-child / next-sibling representation)
// Hierarchical disaster classification:
//   ALL -> NATURAL -> HYDROLOGICAL -> FLOOD ...
// addChild O(1) after locating the parent, find O(n), path-to-root O(depth).

#include <cstddef>
#include <string>
#include <vector>

namespace nexa::dsa
{

class KaryTree
{
public:
    struct Node
    {
        std::string label;
        Node* parent = nullptr;
        Node* firstChild = nullptr;
        Node* nextSibling = nullptr;
        std::size_t childCount = 0;
        std::vector<unsigned long long> members{}; // e.g. incident IDs filed here
    };

private:
    Node* root_;
    std::size_t size_ = 1;

    static void destroy(Node* node)
    {
        while (node != nullptr)
        {
            destroy(node->firstChild);
            Node* next = node->nextSibling;
            delete node;
            node = next;
        }
    }

    static Node* findFrom(Node* node, const std::string& label)
    {
        for (; node != nullptr; node = node->nextSibling)
        {
            if (node->label == label)
                return node;
            if (Node* hit = findFrom(node->firstChild, label))
                return hit;
        }
        return nullptr;
    }

    static std::size_t subtreeMembers(const Node* node)
    {
        std::size_t total = node->members.size();
        for (const Node* c = node->firstChild; c != nullptr; c = c->nextSibling)
            total += subtreeMembers(c);
        return total;
    }

public:
    explicit KaryTree(const std::string& rootLabel)
        : root_(new Node)
    {
        root_->label = rootLabel;
    }

    ~KaryTree() { destroy(root_); }

    KaryTree(const KaryTree&) = delete;
    KaryTree& operator=(const KaryTree&) = delete;

    Node* root() const { return root_; }

    Node* find(const std::string& label) const
    {
        if (root_->label == label)
            return root_;
        return findFrom(root_->firstChild, label);
    }

    // Appends the child at the end of the sibling list (keeps insertion order).
    Node* addChild(Node* parent, const std::string& label)
    {
        if (parent == nullptr)
            return nullptr;
        Node* child = new Node;
        child->label = label;
        child->parent = parent;
        if (parent->firstChild == nullptr)
        {
            parent->firstChild = child;
        }
        else
        {
            Node* last = parent->firstChild;
            while (last->nextSibling != nullptr)
                last = last->nextSibling;
            last->nextSibling = child;
        }
        ++parent->childCount;
        ++size_;
        return child;
    }

    // Labels from the root down to `node`.
    static std::vector<std::string> pathTo(const Node* node)
    {
        std::vector<std::string> reversed;
        for (; node != nullptr; node = node->parent)
            reversed.push_back(node->label);
        return std::vector<std::string>(reversed.rbegin(), reversed.rend());
    }

    static std::size_t depthOf(const Node* node)
    {
        std::size_t depth = 0;
        for (; node != nullptr && node->parent != nullptr; node = node->parent)
            ++depth;
        return depth;
    }

    static std::size_t totalMembers(const Node* node) { return node ? subtreeMembers(node) : 0; }

    std::size_t size() const { return size_; }
};

} // namespace nexa::dsa
