
#pragma once

#include "../data_structures/Graph.h"

namespace nexa
{

class BFS
{
public:
    static bool traverse(
        const Graph& graph,
        int start,
        int* traversal,
        int& count)
    {
        count = 0;

        if (!graph.isValidVertex(start) || traversal == nullptr)
            return false;

        int vertices = graph.getMaxVertices();

        bool* visited = new bool[vertices + 1]{};
        int* queue = new int[vertices + 1];

        int front = 0;
        int rear = 0;

        visited[start] = true;
        queue[rear++] = start;

        while (front < rear)
        {
            int current = queue[front++];
            traversal[count++] = current;

            AdjNode* edge = graph.getAdjacent(current);

            while (edge != nullptr)
            {
                int next = edge->destination;

                if (!edge->blocked && !visited[next])
                {
                    visited[next] = true;
                    queue[rear++] = next;
                }

                edge = edge->next;
            }
        }

        delete[] visited;
        delete[] queue;

        return true;
    }
};

}
