#pragma once

// NEXA DSA #24 - 2-D Range Tree (static, layered by latitude)
// Geographical rectangle queries over resources / incidents.
// Primary balanced tree over latitude; each node stores its subtree's
// points sorted by longitude (merge of children). Query visits O(log n)
// canonical nodes and binary-searches each: O(log^2 n + k).
// Space O(n log n).

#include "dsa/Geo.hpp"
#include "dsa/Searching.hpp"
#include "dsa/Sorting.hpp"

#include <cstddef>
#include <vector>

namespace nexa::dsa
{

class RangeTree
{
private:
    struct Point
    {
        double lat = 0.0, lng = 0.0;
        unsigned long long id = 0;
    };

    struct Node
    {
        std::size_t lo = 0, hi = 0;   // [lo, hi) in byLat_
        double minLat = 0.0, maxLat = 0.0;
        std::vector<Point> byLng;     // associated structure
        int left = -1, right = -1;
    };

    std::vector<Point> byLat_;
    std::vector<Node> nodes_;
    int root_ = -1;

    int build(std::size_t lo, std::size_t hi)
    {
        const int id = static_cast<int>(nodes_.size());
        nodes_.push_back(Node{});
        nodes_[id].lo = lo;
        nodes_[id].hi = hi;
        nodes_[id].minLat = byLat_[lo].lat;
        nodes_[id].maxLat = byLat_[hi - 1].lat;

        if (hi - lo == 1)
        {
            nodes_[id].byLng.push_back(byLat_[lo]);
            return id;
        }
        const std::size_t mid = lo + (hi - lo) / 2;
        const int left = build(lo, mid);
        const int right = build(mid, hi);
        nodes_[id].left = left;
        nodes_[id].right = right;

        // Merge children's longitude-sorted lists.
        const std::vector<Point>& a = nodes_[left].byLng;
        const std::vector<Point>& b = nodes_[right].byLng;
        std::vector<Point> merged;
        merged.reserve(a.size() + b.size());
        std::size_t i = 0, j = 0;
        while (i < a.size() && j < b.size())
            merged.push_back(b[j].lng < a[i].lng ? b[j++] : a[i++]);
        while (i < a.size()) merged.push_back(a[i++]);
        while (j < b.size()) merged.push_back(b[j++]);
        nodes_[id].byLng.swap(merged);
        return id;
    }

    void query(int id, const GeoBox& box, std::vector<unsigned long long>& out, std::size_t& canonical) const
    {
        if (id < 0)
            return;
        const Node& n = nodes_[id];
        if (n.maxLat < box.minLat || n.minLat > box.maxLat)
            return;

        if (n.minLat >= box.minLat && n.maxLat <= box.maxLat)
        {
            // Canonical node: whole latitude span inside -> 1-D search on lng.
            ++canonical;
            std::size_t start = lowerBound(n.byLng.data(), n.byLng.size(), box.minLng,
                                           [](const Point& p) { return p.lng; });
            for (std::size_t k = start; k < n.byLng.size() && n.byLng[k].lng <= box.maxLng; ++k)
                out.push_back(n.byLng[k].id);
            return;
        }
        if (n.left < 0)
        {
            const Point& p = byLat_[n.lo];
            if (box.contains(p.lat, p.lng))
                out.push_back(p.id);
            return;
        }
        query(n.left, box, out, canonical);
        query(n.right, box, out, canonical);
    }

public:
    void build(const std::vector<GeoPoint>& points, const std::vector<unsigned long long>& ids)
    {
        byLat_.clear();
        nodes_.clear();
        root_ = -1;
        const std::size_t n = points.size() < ids.size() ? points.size() : ids.size();
        for (std::size_t i = 0; i < n; ++i)
            byLat_.push_back(Point{points[i].lat, points[i].lng, ids[i]});
        if (byLat_.empty())
            return;
        mergeSort(byLat_.data(), byLat_.size(), [](const Point& a, const Point& b) { return a.lat < b.lat; });
        nodes_.reserve(2 * byLat_.size());
        root_ = build(0, byLat_.size());
    }

    std::vector<unsigned long long> query(const GeoBox& box, std::size_t* canonicalNodes = nullptr) const
    {
        std::vector<unsigned long long> out;
        std::size_t canonical = 0;
        query(root_, box, out, canonical);
        if (canonicalNodes) *canonicalNodes = canonical;
        return out;
    }

    std::size_t size() const { return byLat_.size(); }
};

} // namespace nexa::dsa
