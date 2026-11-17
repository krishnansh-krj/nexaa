
#pragma once

#include <cstddef>

namespace nexa
{

struct AdjNode
{
    int destination;
    int edgeId;
    double weight;
    bool blocked;
    AdjNode* next;

    AdjNode(int dest, int id, double w, bool isBlocked)
        : destination(dest),
          edgeId(id),
          weight(w),
          blocked(isBlocked),
          next(nullptr)
    {}
};

class Graph
{
private:
    int maxVertices;
    AdjNode** adjacencyList;

public:
    explicit Graph(int vertices)
        : maxVertices(vertices)
    {
        adjacencyList = new AdjNode*[maxVertices + 1];

        for (int i = 0; i <= maxVertices; ++i)
            adjacencyList[i] = nullptr;
    }

    ~Graph()
    {
        for (int i = 0; i <= maxVertices; ++i)
        {
            AdjNode* current = adjacencyList[i];

            while (current != nullptr)
            {
                AdjNode* temp = current;
                current = current->next;
                delete temp;
            }
        }

        delete[] adjacencyList;
    }

    Graph(const Graph&) = delete;
    Graph& operator=(const Graph&) = delete;

    bool isValidVertex(int vertex) const
    {
        return vertex >= 1 && vertex <= maxVertices;
    }

    bool addEdge(
        int source,
        int destination,
        int edgeId,
        double weight,
        bool blocked = false)
    {
        if (!isValidVertex(source) ||
            !isValidVertex(destination) ||
            weight <= 0)
            return false;

        AdjNode* node = new AdjNode(
            destination, edgeId, weight, blocked);

        node->next = adjacencyList[source];
        adjacencyList[source] = node;

        return true;
    }

    bool setEdgeBlocked(int edgeId, bool blocked)
    {
        bool found = false;

        for (int i = 1; i <= maxVertices; ++i)
        {
            AdjNode* current = adjacencyList[i];

            while (current != nullptr)
            {
                if (current->edgeId == edgeId)
                {
                    current->blocked = blocked;
                    found = true;
                }

                current = current->next;
            }
        }

        return found;
    }

    AdjNode* getAdjacent(int vertex) const
    {
        if (!isValidVertex(vertex))
            return nullptr;

        return adjacencyList[vertex];
    }

    int getMaxVertices() const
    {
        return maxVertices;
    }
};

}
