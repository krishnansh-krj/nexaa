
#include <iostream>

#include "data_structures/Graph.h"
#include "algorithms/BFS.h"
#include "algorithms/DFS.h"
#include "algorithms/Dijkstra.h"

int main()
{
    nexa::Graph graph(4);

    // Directed weighted network
    graph.addEdge(1, 2, 1, 4.0);
    graph.addEdge(1, 3, 2, 2.0);
    graph.addEdge(2, 3, 3, 1.0);
    graph.addEdge(2, 4, 4, 5.0);
    graph.addEdge(3, 4, 5, 3.0);

    int traversal[5];
    int count = 0;

    std::cout << "BFS: ";

    nexa::BFS::traverse(graph, 1, traversal, count);

    for (int i = 0; i < count; ++i)
        std::cout << traversal[i] << " ";

    std::cout << "\nDFS: ";

    nexa::DFS::traverse(graph, 1, traversal, count);

    for (int i = 0; i < count; ++i)
        std::cout << traversal[i] << " ";

    double distance;
    int path[5];
    int pathLength;

    bool found = nexa::Dijkstra::shortestPath(
        graph, 1, 4, distance, path, pathLength);

    if (found)
    {
        std::cout << "\nShortest distance: " << distance;
        std::cout << "\nShortest path: ";

        for (int i = 0; i < pathLength; ++i)
            std::cout << path[i] << " ";
    }

    // Block edge 2 (1 -> 3) and calculate again.
    graph.setEdgeBlocked(2, true);

    found = nexa::Dijkstra::shortestPath(
        graph, 1, 4, distance, path, pathLength);

    if (found)
        std::cout << "\nDistance after blocking edge 2: "
                  << distance;

    std::cout << "\n";

    return 0;
}
