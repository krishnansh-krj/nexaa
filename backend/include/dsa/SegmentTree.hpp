#pragma once

// NEXA DSA #26 - Segment Tree (sum + max)
// Range-based operational analytics, e.g. active units per hour bucket:
// total unit-hours and peak load over any bucket range.
// build O(n), point update O(log n), range sum / max O(log n).

#include <cstddef>
#include <vector>

namespace nexa::dsa
{

class SegmentTree
{
private:
    std::size_t n_ = 0;
    std::vector<long long> sum_;
    std::vector<long long> max_;

    void build(std::size_t node, std::size_t lo, std::size_t hi, const std::vector<long long>& values)
    {
        if (lo == hi)
        {
            sum_[node] = max_[node] = values[lo];
            return;
        }
        const std::size_t mid = (lo + hi) / 2;
        build(2 * node, lo, mid, values);
        build(2 * node + 1, mid + 1, hi, values);
        pull(node);
    }

    void pull(std::size_t node)
    {
        sum_[node] = sum_[2 * node] + sum_[2 * node + 1];
        max_[node] = max_[2 * node] > max_[2 * node + 1] ? max_[2 * node] : max_[2 * node + 1];
    }

    void update(std::size_t node, std::size_t lo, std::size_t hi, std::size_t index, long long value)
    {
        if (lo == hi)
        {
            sum_[node] = max_[node] = value;
            return;
        }
        const std::size_t mid = (lo + hi) / 2;
        if (index <= mid) update(2 * node, lo, mid, index, value);
        else update(2 * node + 1, mid + 1, hi, index, value);
        pull(node);
    }

    long long querySum(std::size_t node, std::size_t lo, std::size_t hi, std::size_t l, std::size_t r) const
    {
        if (r < lo || hi < l) return 0;
        if (l <= lo && hi <= r) return sum_[node];
        const std::size_t mid = (lo + hi) / 2;
        return querySum(2 * node, lo, mid, l, r) + querySum(2 * node + 1, mid + 1, hi, l, r);
    }

    long long queryMax(std::size_t node, std::size_t lo, std::size_t hi, std::size_t l, std::size_t r) const
    {
        if (r < lo || hi < l) return 0;
        if (l <= lo && hi <= r) return max_[node];
        const std::size_t mid = (lo + hi) / 2;
        const long long a = queryMax(2 * node, lo, mid, l, r);
        const long long b = queryMax(2 * node + 1, mid + 1, hi, l, r);
        return a > b ? a : b;
    }

public:
    SegmentTree() = default;

    explicit SegmentTree(const std::vector<long long>& values) { build(values); }

    // Values are treated as non-negative for max (empty ranges return 0).
    void build(const std::vector<long long>& values)
    {
        n_ = values.size();
        sum_.assign(n_ == 0 ? 1 : 4 * n_, 0);
        max_.assign(n_ == 0 ? 1 : 4 * n_, 0);
        if (n_ > 0)
            build(1, 0, n_ - 1, values);
    }

    void set(std::size_t index, long long value)
    {
        if (index < n_)
            update(1, 0, n_ - 1, index, value);
    }

    // Inclusive [l, r]; out-of-range bounds are clamped.
    long long rangeSum(std::size_t l, std::size_t r) const
    {
        if (n_ == 0 || l > r || l >= n_) return 0;
        return querySum(1, 0, n_ - 1, l, r >= n_ ? n_ - 1 : r);
    }

    long long rangeMax(std::size_t l, std::size_t r) const
    {
        if (n_ == 0 || l > r || l >= n_) return 0;
        return queryMax(1, 0, n_ - 1, l, r >= n_ ? n_ - 1 : r);
    }

    std::size_t size() const { return n_; }
};

} // namespace nexa::dsa
