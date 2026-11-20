#pragma once

// NEXA DSA #20 - Minimum Spanning Tree / Forest
//   kruskal : sort edges (merge sort) + Disjoint Set     O(E log E)
//   prim    : adjacency matrix, array-based selection    O(V^2)
// Both return a spanning FOREST if the network is disconnected; the two
// totals must agree, which the API reports as a self-check.
// Vertices are 0-based indices.

#include "dsa/AdjacencyMatrix.hpp"
#include "dsa/DisjointSet.hpp"
#include "dsa/Sorting.hpp"

#include <cstddef>
#include <vector>

namespace nexa::dsa
{

struct WeightedEdge
{
    std::size_t u = 0;
    std::size_t v = 0;
    double weight = 0.0;
    unsigned long long id = 0; // original edge ID (for reporting)
};

struct SpanningForest
{
    std::vector<WeightedEdge> edges;
    double totalWeight = 0.0;
    std::size_t components = 0;
};

inline SpanningForest kruskalMST(std::size_t vertexCount, std::vector<WeightedEdge> edges)
{
    SpanningForest result;
    if (!edges.empty())
        mergeSort(edges.data(), edges.size(),
                  [](const WeightedEdge& a, const WeightedEdge& b) { return a.weight < b.weight; });

    DisjointSet sets(vertexCount);
    for (const WeightedEdge& e : edges)
    {
        if (e.u >= vertexCount || e.v >= vertexCount)
            continue;
        if (sets.unite(e.u, e.v))
        {
            result.edges.push_back(e);
            result.totalWeight += e.weight;
            if (result.edges.size() + 1 == vertexCount)
                break;
        }
    }
    result.components = sets.components();
    return result;
}

inline SpanningForest primMST(const AdjacencyMatrix& graph)
{
    SpanningForest result;
    const std::size_t n = graph.vertices();
    if (n == 0)
        return result;

    bool* inTree = new bool[n]{};
    double* best = new double[n];
    std::size_t* from = new std::size_t[n];
    for (std::size_t i = 0; i < n; ++i)
    {
        best[i] = AdjacencyMatrix::kNoEdge;
        from[i] = n;
    }

    for (std::size_t added = 0; added < n; ++added)
    {
        // Pick the cheapest frontier vertex; start a new tree if none reachable.
        std::size_t pick = n;
        for (std::size_t v = 0; v < n; ++v)
            if (!inTree[v] && (pick == n || best[v] < best[pick]))
                pick = v;

        if (best[pick] == AdjacencyMatrix::kNoEdge)
            ++result.components; // root of a new component
        else
        {
            result.edges.push_back(WeightedEdge{from[pick], pick, best[pick], 0});
            result.totalWeight += best[pick];
        }
        inTree[pick] = true;

        for (std::size_t v = 0; v < n; ++v)
            if (!inTree[v] && graph.weight(pick, v) < best[v])
            {
                best[v] = graph.weight(pick, v);
                from[v] = pick;
            }
    }

    delete[] from;
    delete[] best;
    delete[] inTree;
    return result;
}

} // namespace nexa::dsa
