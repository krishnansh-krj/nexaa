#include "algorithms/DFS.hpp"

namespace nexa
{
bool DFS::traverse(const Graph& graph, int start,
                   int* traversal, int& count)
{
    count = 0;
    if (!graph.isValidVertex(start) || traversal == nullptr)
        return false;

    const int vertices = graph.getMaxVertices();
    bool* visited = new bool[vertices + 1]{};
    int* stack = new int[vertices + 1];
    int* neighbours = new int[vertices + 1];

    int top = -1;
    stack[++top] = start;
    visited[start] = true;

    while (top >= 0)
    {
        const int current = stack[top--];
        traversal[count++] = current;
        int neighbourCount = 0;

        AdjNode* edge = graph.getAdjacent(current);
        while (edge != nullptr)
        {
            const int next = edge->destination;
            if (!edge->blocked && !visited[next])
            {
                visited[next] = true;
                neighbours[neighbourCount++] = next;
            }
            edge = edge->next;
        }

        for (int i = neighbourCount - 1; i >= 0; --i)
            stack[++top] = neighbours[i];
    }

    delete[] neighbours;
    delete[] stack;
    delete[] visited;
    return true;
}
}
