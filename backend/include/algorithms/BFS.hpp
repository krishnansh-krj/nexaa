#pragma once

#include "data_structures/Graph.hpp"

namespace nexa
{
class BFS
{
public:
    static bool traverse(const Graph& graph, int start,
                         int* traversal, int& count);
};
}
