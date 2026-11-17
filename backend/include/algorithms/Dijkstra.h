
#pragma once

#include "../data_structures/Graph.h"
#include "../data_structures/MinHeap.h"

#include <limits>

namespace nexa
{

class Dijkstra
{
public:

    static bool shortestPath(
        const Graph& graph,
        int source,
        int destination,
        double& shortestDistance,
        int* path,
        int& pathLength)
    {
        const int vertices = graph.getMaxVertices();

        pathLength = 0;
        shortestDistance = std::numeric_limits<double>::infinity();

        if (!graph.isValidVertex(source) ||
            !graph.isValidVertex(destination) ||
            path == nullptr)
            return false;

        double* distance = new double[vertices + 1];
        int* previous = new int[vertices + 1];

        for (int i = 0; i <= vertices; ++i)
        {
            distance[i] = std::numeric_limits<double>::infinity();
            previous[i] = -1;
        }

        MinHeap heap(vertices);

        distance[source] = 0;
        heap.insertOrDecrease(source, 0);

        HeapNode current;

        while (heap.extractMin(current))
        {
            int u = current.vertex;

            if (u == destination)
                break;

            AdjNode* edge = graph.getAdjacent(u);

            while (edge != nullptr)
            {
                if (!edge->blocked)
                {
                    int v = edge->destination;

                    double newDistance =
                        distance[u] + edge->weight;

                    if (newDistance < distance[v])
                    {
                        distance[v] = newDistance;
                        previous[v] = u;

                        heap.insertOrDecrease(v, newDistance);
                    }
                }

                edge = edge->next;
            }
        }

        bool found = distance[destination] !=
            std::numeric_limits<double>::infinity();

        if (found)
        {
            shortestDistance = distance[destination];

            int* reversePath = new int[vertices + 1];
            int count = 0;

            int currentVertex = destination;

            while (currentVertex != -1 &&
                   count <= vertices)
            {
                reversePath[count++] = currentVertex;

                if (currentVertex == source)
                    break;

                currentVertex = previous[currentVertex];
            }

            if (count > 0 && reversePath[count - 1] == source)
            {
                pathLength = count;

                for (int i = 0; i < count; ++i)
                    path[i] = reversePath[count - i - 1];
            }
            else
            {
                found = false;
                pathLength = 0;
            }

            delete[] reversePath;
        }

        delete[] distance;
        delete[] previous;

        return found;
    }
};

}
