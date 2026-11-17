#include "data_structures/MinHeap.hpp"

#include <utility>

namespace nexa
{

MinHeap::MinHeap(int maxVertices)
    : heap(nullptr), position(nullptr),
      capacity(maxVertices > 0 ? maxVertices : 0), size(0)
{
    heap = new HeapNode[capacity + 1];
    position = new int[capacity + 1];
    for (int i = 0; i <= capacity; ++i)
        position[i] = -1;
}

MinHeap::~MinHeap()
{
    delete[] heap;
    delete[] position;
}

void MinHeap::swapNodes(int a, int b)
{
    std::swap(heap[a], heap[b]);
    position[heap[a].vertex] = a;
    position[heap[b].vertex] = b;
}

void MinHeap::heapifyUp(int index)
{
    while (index > 1)
    {
        int parent = index / 2;
        if (heap[parent].distance <= heap[index].distance)
            break;
        swapNodes(parent, index);
        index = parent;
    }
}

void MinHeap::heapifyDown(int index)
{
    while (true)
    {
        int smallest = index;
        int left = 2 * index;
        int right = 2 * index + 1;

        if (left <= size && heap[left].distance < heap[smallest].distance)
            smallest = left;
        if (right <= size && heap[right].distance < heap[smallest].distance)
            smallest = right;
        if (smallest == index)
            break;

        swapNodes(index, smallest);
        index = smallest;
    }
}

bool MinHeap::isEmpty() const
{
    return size == 0;
}

bool MinHeap::insertOrDecrease(int vertex, double distance)
{
    if (vertex < 1 || vertex > capacity)
        return false;

    int index = position[vertex];
    if (index == -2)
        return false;

    if (index == -1)
    {
        if (size >= capacity)
            return false;
        ++size;
        index = size;
        heap[index] = {vertex, distance};
        position[vertex] = index;
    }
    else
    {
        if (distance >= heap[index].distance)
            return true;
        heap[index].distance = distance;
    }

    heapifyUp(index);
    return true;
}

bool MinHeap::extractMin(HeapNode& result)
{
    if (isEmpty())
        return false;

    result = heap[1];
    position[result.vertex] = -2;

    if (size == 1)
    {
        size = 0;
        return true;
    }

    heap[1] = heap[size];
    position[heap[1].vertex] = 1;
    --size;
    heapifyDown(1);
    return true;
}

}
