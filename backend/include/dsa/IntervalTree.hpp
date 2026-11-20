#pragma once

// NEXA DSA #25 - Interval Tree (augmented, centred on start points)
// Resource-availability analysis: which allocations were active during a
// time window [from, to]? Each node stores the max end-point of its subtree
// so whole subtrees are skipped. Build O(n log n), overlap query O(log n + k).

#include "dsa/Sorting.hpp"

#include <cstddef>
#include <vector>

namespace nexa::dsa
{

class IntervalTree
{
public:
    struct Interval
    {
        long long start = 0;   // inclusive
        long long end = 0;     // inclusive
        unsigned long long id = 0;
    };

private:
    struct Node
    {
        Interval interval;
        long long maxEnd = 0;
        int left = -1, right = -1;
    };

    std::vector<Node> nodes_;
    int root_ = -1;

    int build(Interval* sorted, std::size_t count)
    {
        if (count == 0)
            return -1;
        const std::size_t mid = count / 2;
        const int id = static_cast<int>(nodes_.size());
        nodes_.push_back(Node{sorted[mid], sorted[mid].end, -1, -1});
        const int left = build(sorted, mid);
        const int right = build(sorted + mid + 1, count - mid - 1);
        nodes_[id].left = left;
        nodes_[id].right = right;
        if (left >= 0 && nodes_[left].maxEnd > nodes_[id].maxEnd) nodes_[id].maxEnd = nodes_[left].maxEnd;
        if (right >= 0 && nodes_[right].maxEnd > nodes_[id].maxEnd) nodes_[id].maxEnd = nodes_[right].maxEnd;
        return id;
    }

    void overlaps(int id, long long from, long long to, std::vector<Interval>& out, std::size_t& visited) const
    {
        if (id < 0 || nodes_[id].maxEnd < from)
            return; // nothing in this subtree ends late enough
        ++visited;
        const Node& n = nodes_[id];
        overlaps(n.left, from, to, out, visited);
        if (n.interval.start <= to && n.interval.end >= from)
            out.push_back(n.interval);
        if (n.interval.start <= to) // right subtree starts are >= this start
            overlaps(n.right, from, to, out, visited);
    }

public:
    void build(std::vector<Interval> intervals)
    {
        nodes_.clear();
        root_ = -1;
        if (intervals.empty())
            return;
        mergeSort(intervals.data(), intervals.size(),
                  [](const Interval& a, const Interval& b) { return a.start < b.start; });
        nodes_.reserve(intervals.size());
        root_ = build(intervals.data(), intervals.size());
    }

    std::vector<Interval> overlapping(long long from, long long to, std::size_t* visitedNodes = nullptr) const
    {
        std::vector<Interval> out;
        std::size_t visited = 0;
        overlaps(root_, from, to, out, visited);
        if (visitedNodes) *visitedNodes = visited;
        return out;
    }

    std::vector<Interval> stabbing(long long instant) const { return overlapping(instant, instant); }

    std::size_t size() const { return nodes_.size(); }
};

} // namespace nexa::dsa
