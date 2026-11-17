#pragma once

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
        : destination(dest), edgeId(id), weight(w),
          blocked(isBlocked), next(nullptr)
    {}
};

class Graph
{
private:
    int maxVertices;
    AdjNode** adjacencyList;

public:
    explicit Graph(int vertices);
    ~Graph();

    Graph(const Graph&) = delete;
    Graph& operator=(const Graph&) = delete;

    bool isValidVertex(int vertex) const;
    bool addEdge(int source, int destination, int edgeId,
                 double weight, bool blocked = false);
    bool setEdgeBlocked(int edgeId, bool blocked);
    AdjNode* getAdjacent(int vertex) const;
    int getMaxVertices() const;
};

}
