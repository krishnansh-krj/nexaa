#include "algorithms/BFS.hpp"

namespace nexa
{
bool BFS::traverse(const Graph& graph, int start,
                   int* traversal, int& count)
{
    count = 0;
    if (!graph.isValidVertex(start) || traversal == nullptr)
        return false;

    const int vertices = graph.getMaxVertices();
    bool* visited = new bool[vertices + 1]{};
    int* queue = new int[vertices + 1];

    int front = 0, rear = 0;
    visited[start] = true;
    queue[rear++] = start;

    while (front < rear)
    {
        const int current = queue[front++];
        traversal[count++] = current;

        AdjNode* edge = graph.getAdjacent(current);
        while (edge != nullptr)
        {
            const int next = edge->destination;
            if (!edge->blocked && !visited[next])
            {
                visited[next] = true;
                queue[rear++] = next;
            }
            edge = edge->next;
        }
    }

    delete[] queue;
    delete[] visited;
    return true;
}
}
