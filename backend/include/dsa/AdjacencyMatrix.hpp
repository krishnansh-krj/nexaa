#pragma once

// NEXA DSA #17b - Graph as an Adjacency Matrix
// Complements the adjacency-list Graph (data_structures/Graph.hpp):
// O(1) edge lookup, O(V^2) space; used by Prim's MST on dense networks.
// Vertices are 0-based. Weight 0 / "infinity" means no edge.

#include <cstddef>
#include <limits>

namespace nexa::dsa
{

class AdjacencyMatrix
{
private:
    std::size_t n_;
    double* cells_;

public:
    static constexpr double kNoEdge = std::numeric_limits<double>::infinity();

    explicit AdjacencyMatrix(std::size_t vertices)
        : n_(vertices), cells_(new double[vertices * vertices > 0 ? vertices * vertices : 1])
    {
        for (std::size_t i = 0; i < n_ * n_; ++i)
            cells_[i] = kNoEdge;
    }

    ~AdjacencyMatrix() { delete[] cells_; }

    AdjacencyMatrix(const AdjacencyMatrix&) = delete;
    AdjacencyMatrix& operator=(const AdjacencyMatrix&) = delete;

    // Undirected; keeps the lighter of parallel edges.
    bool setUndirected(std::size_t u, std::size_t v, double weight)
    {
        if (u >= n_ || v >= n_ || u == v || !(weight > 0))
            return false;
        if (weight < cells_[u * n_ + v])
        {
            cells_[u * n_ + v] = weight;
            cells_[v * n_ + u] = weight;
        }
        return true;
    }

    double weight(std::size_t u, std::size_t v) const { return cells_[u * n_ + v]; }
    bool hasEdge(std::size_t u, std::size_t v) const { return cells_[u * n_ + v] != kNoEdge; }
    std::size_t vertices() const { return n_; }
};

} // namespace nexa::dsa
