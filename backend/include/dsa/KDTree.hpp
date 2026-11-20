#pragma once

// NEXA DSA #22 - 2-D KD Tree
// Nearest-resource spatial search. Points are projected to a local
// kilometre grid (equirectangular around the data's mean latitude) so the
// split-plane pruning is metric; callers report exact haversine distances.
// build O(n log n) (median split via quick-select), nearest O(log n) avg.

#include "dsa/Geo.hpp"
#include "dsa/Searching.hpp"

#include <cmath>
#include <cstddef>
#include <vector>

namespace nexa::dsa
{

class KDTree
{
public:
    struct Hit
    {
        std::size_t index = 0;   // index into the original input
        double planarKm = 0.0;   // projected distance used for ranking
    };

private:
    struct Point
    {
        double x = 0.0, y = 0.0;
        std::size_t index = 0;
    };

    struct Node
    {
        Point point;
        int axis = 0;
        int left = -1, right = -1;
    };

    std::vector<Node> nodes_;
    int root_ = -1;
    double cosRef_ = 1.0;
    std::size_t visited_ = 0;

    static constexpr double kKmPerDegLat = 110.574;
    static constexpr double kKmPerDegLng = 111.320;

    void project(double lat, double lng, double& x, double& y) const
    {
        x = lng * kKmPerDegLng * cosRef_;
        y = lat * kKmPerDegLat;
    }

    int build(Point* pts, std::size_t count, int depth)
    {
        if (count == 0)
            return -1;
        const int axis = depth % 2;
        const std::size_t mid = count / 2;
        quickSelect(pts, count, mid, [axis](const Point& a, const Point& b)
                    { return axis == 0 ? a.x < b.x : a.y < b.y; });

        const int id = static_cast<int>(nodes_.size());
        nodes_.push_back(Node{pts[mid], axis, -1, -1});
        const int left = build(pts, mid, depth + 1);
        const int right = build(pts + mid + 1, count - mid - 1, depth + 1);
        nodes_[id].left = left;
        nodes_[id].right = right;
        return id;
    }

    // Keeps `best` sorted ascending, size <= k.
    void search(int id, double x, double y, std::size_t k, std::vector<Hit>& best)
    {
        if (id < 0)
            return;
        ++visited_;
        const Node& node = nodes_[id];
        const double dx = node.point.x - x, dy = node.point.y - y;
        const double dist = std::sqrt(dx * dx + dy * dy);

        if (best.size() < k || dist < best.back().planarKm)
        {
            Hit hit{node.point.index, dist};
            std::size_t pos = best.size();
            best.push_back(hit);
            while (pos > 0 && best[pos - 1].planarKm > dist)
            {
                best[pos] = best[pos - 1];
                --pos;
            }
            best[pos] = hit;
            if (best.size() > k)
                best.pop_back();
        }

        const double diff = node.axis == 0 ? x - node.point.x : y - node.point.y;
        const int nearSide = diff < 0 ? node.left : node.right;
        const int farSide = diff < 0 ? node.right : node.left;
        search(nearSide, x, y, k, best);
        if (best.size() < k || std::fabs(diff) < best.back().planarKm)
            search(farSide, x, y, k, best);
    }

public:
    KDTree() = default;

    void build(const std::vector<GeoPoint>& points)
    {
        nodes_.clear();
        root_ = -1;
        if (points.empty())
            return;

        double latSum = 0.0;
        for (const GeoPoint& p : points)
            latSum += p.lat;
        cosRef_ = std::cos((latSum / static_cast<double>(points.size())) * 3.14159265358979323846 / 180.0);

        std::vector<Point> pts(points.size());
        for (std::size_t i = 0; i < points.size(); ++i)
        {
            project(points[i].lat, points[i].lng, pts[i].x, pts[i].y);
            pts[i].index = i;
        }
        nodes_.reserve(points.size());
        root_ = build(pts.data(), pts.size(), 0);
    }

    std::vector<Hit> kNearest(double lat, double lng, std::size_t k)
    {
        std::vector<Hit> best;
        visited_ = 0;
        if (k == 0 || root_ < 0)
            return best;
        double x = 0, y = 0;
        project(lat, lng, x, y);
        search(root_, x, y, k, best);
        return best;
    }

    std::size_t size() const { return nodes_.size(); }
    std::size_t lastVisited() const { return visited_; }
};

} // namespace nexa::dsa
