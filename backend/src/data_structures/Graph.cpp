#include "data_structures/Graph.hpp"

namespace nexa
{

Graph::Graph(int vertices)
    : maxVertices(vertices), adjacencyList(nullptr)
{
    if (maxVertices < 0)
        maxVertices = 0;

    adjacencyList = new AdjNode*[maxVertices + 1]{};
}

Graph::~Graph()
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

bool Graph::isValidVertex(int vertex) const
{
    return vertex >= 1 && vertex <= maxVertices;
}

bool Graph::addEdge(int source, int destination, int edgeId,
                    double weight, bool blocked)
{
    if (!isValidVertex(source) || !isValidVertex(destination) ||
        weight <= 0)
        return false;

    AdjNode* node = new AdjNode(destination, edgeId, weight, blocked);
    node->next = adjacencyList[source];
    adjacencyList[source] = node;
    return true;
}

bool Graph::setEdgeBlocked(int edgeId, bool blocked)
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

AdjNode* Graph::getAdjacent(int vertex) const
{
    if (!isValidVertex(vertex))
        return nullptr;
    return adjacencyList[vertex];
}

int Graph::getMaxVertices() const
{
    return maxVertices;
}

}
