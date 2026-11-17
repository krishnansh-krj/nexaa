#pragma once

namespace nexa
{

struct HeapNode
{
    int vertex;
    double distance;
};

class MinHeap
{
private:
    HeapNode* heap;
    int* position;
    int capacity;
    int size;

    void swapNodes(int a, int b);
    void heapifyUp(int index);
    void heapifyDown(int index);

public:
    explicit MinHeap(int maxVertices);
    ~MinHeap();

    MinHeap(const MinHeap&) = delete;
    MinHeap& operator=(const MinHeap&) = delete;

    bool isEmpty() const;
    bool insertOrDecrease(int vertex, double distance);
    bool extractMin(HeapNode& result);
};

}
