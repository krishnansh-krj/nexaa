
#pragma once

#include <utility>

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

    void swapNodes(int a, int b)
    {
        std::swap(heap[a], heap[b]);

        position[heap[a].vertex] = a;
        position[heap[b].vertex] = b;
    }

    void heapifyUp(int index)
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

    void heapifyDown(int index)
    {
        while (true)
        {
            int smallest = index;
            int left = 2 * index;
            int right = 2 * index + 1;

            if (left <= size &&
                heap[left].distance < heap[smallest].distance)
                smallest = left;

            if (right <= size &&
                heap[right].distance < heap[smallest].distance)
                smallest = right;

            if (smallest == index)
                break;

            swapNodes(index, smallest);
            index = smallest;
        }
    }

public:
    explicit MinHeap(int maxVertices)
        : capacity(maxVertices), size(0)
    {
        heap = new HeapNode[capacity + 1];

        position = new int[capacity + 1];

        for (int i = 0; i <= capacity; ++i)
            position[i] = -1;
    }

    ~MinHeap()
    {
        delete[] heap;
        delete[] position;
    }

    MinHeap(const MinHeap&) = delete;
    MinHeap& operator=(const MinHeap&) = delete;

    bool isEmpty() const
    {
        return size == 0;
    }

    bool insertOrDecrease(int vertex, double distance)
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

    bool extractMin(HeapNode& result)
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
};

}
