#pragma once

// NEXA DSA #23 - Point-Region Quad Tree
// Spatial partitioning for regional incident / resource queries.
// Each leaf holds up to `capacity` points, then splits into NW/NE/SW/SE.
// insert O(depth), range query O(log n + k) average.

#include "dsa/Geo.hpp"

#include <cstddef>
#include <vector>

namespace nexa::dsa
{

class QuadTree
{
private:
    struct Item
    {
        double lat, lng;
        unsigned long long id;
    };

    struct Node
    {
        GeoBox bounds;
        std::vector<Item> items;
        Node* child[4] = {nullptr, nullptr, nullptr, nullptr}; // NW, NE, SW, SE
        int depth = 0;
        bool leaf() const { return child[0] == nullptr; }
    };

    Node* root_;
    std::size_t capacity_;
    int maxDepth_;
    std::size_t size_ = 0;
    std::size_t nodeCount_ = 1;

    static void destroy(Node* n)
    {
        if (!n) return;
        for (Node* c : n->child) destroy(c);
        delete n;
    }

    static int quadrantOf(const Node* n, double lat, double lng)
    {
        const double midLat = (n->bounds.minLat + n->bounds.maxLat) / 2;
        const double midLng = (n->bounds.minLng + n->bounds.maxLng) / 2;
        const bool north = lat >= midLat;
        const bool east = lng >= midLng;
        return north ? (east ? 1 : 0) : (east ? 3 : 2);
    }

    void split(Node* n)
    {
        const GeoBox& b = n->bounds;
        const double midLat = (b.minLat + b.maxLat) / 2;
        const double midLng = (b.minLng + b.maxLng) / 2;
        const GeoBox boxes[4] = {
            {midLat, b.maxLat, b.minLng, midLng}, // NW
            {midLat, b.maxLat, midLng, b.maxLng}, // NE
            {b.minLat, midLat, b.minLng, midLng}, // SW
            {b.minLat, midLat, midLng, b.maxLng}, // SE
        };
        for (int q = 0; q < 4; ++q)
        {
            n->child[q] = new Node;
            n->child[q]->bounds = boxes[q];
            n->child[q]->depth = n->depth + 1;
        }
        nodeCount_ += 4;
        for (const Item& it : n->items)
            n->child[quadrantOf(n, it.lat, it.lng)]->items.push_back(it);
        n->items.clear();
    }

    void insert(Node* n, const Item& item)
    {
        while (!n->leaf())
            n = n->child[quadrantOf(n, item.lat, item.lng)];
        n->items.push_back(item);
        if (n->items.size() > capacity_ && n->depth < maxDepth_)
        {
            split(n);
            // Re-check the child that received the most points.
            for (Node* c : n->child)
                if (c->items.size() > capacity_ && c->depth < maxDepth_)
                {
                    std::vector<Item> moved;
                    moved.swap(c->items);
                    for (const Item& it : moved)
                        insert(c, it);
                }
        }
    }

    static void query(const Node* n, const GeoBox& box, std::vector<unsigned long long>& out,
                      std::size_t& visited)
    {
        if (!n || !n->bounds.intersects(box))
            return;
        ++visited;
        for (const Item& it : n->items)
            if (box.contains(it.lat, it.lng))
                out.push_back(it.id);
        for (const Node* c : n->child)
            query(c, box, out, visited);
    }

    static int depthOf(const Node* n)
    {
        if (!n || n->leaf()) return 0;
        int best = 0;
        for (const Node* c : n->child)
        {
            const int d = depthOf(c);
            if (d > best) best = d;
        }
        return best + 1;
    }

public:
    QuadTree(const GeoBox& bounds, std::size_t capacity = 4, int maxDepth = 16)
        : root_(new Node), capacity_(capacity == 0 ? 1 : capacity), maxDepth_(maxDepth)
    {
        root_->bounds = bounds;
    }

    ~QuadTree() { destroy(root_); }

    QuadTree(const QuadTree&) = delete;
    QuadTree& operator=(const QuadTree&) = delete;

    bool insert(double lat, double lng, unsigned long long id)
    {
        if (!root_->bounds.contains(lat, lng))
            return false;
        insert(root_, Item{lat, lng, id});
        ++size_;
        return true;
    }

    std::vector<unsigned long long> queryRange(const GeoBox& box, std::size_t* visitedNodes = nullptr) const
    {
        std::vector<unsigned long long> out;
        std::size_t visited = 0;
        query(root_, box, out, visited);
        if (visitedNodes) *visitedNodes = visited;
        return out;
    }

    // Number of points in each top-level quadrant (NW, NE, SW, SE).
    void quadrantCounts(std::size_t counts[4]) const
    {
        for (int q = 0; q < 4; ++q)
        {
            counts[q] = 0;
            if (root_->leaf()) continue;
            counts[q] = queryCount(root_->child[q]);
        }
        if (root_->leaf())
            for (const Item& it : root_->items)
                ++counts[quadrantOf(root_, it.lat, it.lng)];
    }

    static std::size_t queryCount(const Node* n)
    {
        if (!n) return 0;
        std::size_t total = n->items.size();
        for (const Node* c : n->child) total += queryCount(c);
        return total;
    }

    std::size_t size() const { return size_; }
    std::size_t nodeCount() const { return nodeCount_; }
    int depth() const { return depthOf(root_); }
};

} // namespace nexa::dsa
