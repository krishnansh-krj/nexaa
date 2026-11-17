#pragma once

#include "data_structures/Graph.hpp"

namespace nexa
{
class Dijkstra
{
public:
    static bool shortestPath(const Graph& graph, int source,
                             int destination, double& shortestDistance,
                             int* path, int& pathLength);
};
}
