#pragma once

// NEXA DSA - Disjoint Set Union (union by rank + path compression)
// Support structure for Kruskal's MST; near O(1) amortised per operation.

#include <cstddef>

namespace nexa::dsa
{

class DisjointSet
{
private:
    std::size_t* parent_;
    unsigned char* rank_;
    std::size_t n_;
    std::size_t components_;

public:
    explicit DisjointSet(std::size_t n)
        : parent_(new std::size_t[n > 0 ? n : 1]), rank_(new unsigned char[n > 0 ? n : 1]{}),
          n_(n), components_(n)
    {
        for (std::size_t i = 0; i < n; ++i)
            parent_[i] = i;
    }

    ~DisjointSet()
    {
        delete[] parent_;
        delete[] rank_;
    }

    DisjointSet(const DisjointSet&) = delete;
    DisjointSet& operator=(const DisjointSet&) = delete;

    std::size_t find(std::size_t x)
    {
        std::size_t root = x;
        while (parent_[root] != root)
            root = parent_[root];
        while (parent_[x] != root)
        {
            const std::size_t next = parent_[x];
            parent_[x] = root;
            x = next;
        }
        return root;
    }

    bool unite(std::size_t a, std::size_t b)
    {
        a = find(a);
        b = find(b);
        if (a == b)
            return false;
        if (rank_[a] < rank_[b]) { std::size_t t = a; a = b; b = t; }
        parent_[b] = a;
        if (rank_[a] == rank_[b]) ++rank_[a];
        --components_;
        return true;
    }

    std::size_t components() const { return components_; }
    std::size_t size() const { return n_; }
};

} // namespace nexa::dsa
