#pragma once

// NEXA DSA #6 - Sparse Matrix (sorted triplet / coordinate form)
// Memory-efficient incident x resource allocation matrix: only non-zero
// cells are stored, so space is O(k) for k non-zero entries instead of
// O(incidents x resources).
// get O(log k) (binary search), add O(k) (ordered insert), row/col sum O(k).

#include "dsa/DynamicArray.hpp"

#include <cstddef>

namespace nexa::dsa
{

class SparseMatrix
{
public:
    struct Entry
    {
        unsigned long long row = 0;
        unsigned long long col = 0;
        long long value = 0;
    };

private:
    DynamicArray<Entry> entries_;

    static bool before(const Entry& e, unsigned long long row, unsigned long long col)
    {
        return e.row < row || (e.row == row && e.col < col);
    }

    // First index whose (row, col) >= (row, col).
    std::size_t lowerBound(unsigned long long row, unsigned long long col) const
    {
        std::size_t lo = 0, hi = entries_.size();
        while (lo < hi)
        {
            const std::size_t mid = lo + (hi - lo) / 2;
            if (before(entries_[mid], row, col)) lo = mid + 1; else hi = mid;
        }
        return lo;
    }

public:
    // Accumulates value into (row, col); removes the cell if it becomes 0.
    void add(unsigned long long row, unsigned long long col, long long value)
    {
        if (value == 0)
            return;

        const std::size_t index = lowerBound(row, col);
        if (index < entries_.size() && entries_[index].row == row && entries_[index].col == col)
        {
            entries_[index].value += value;
            if (entries_[index].value == 0)
                entries_.removeAt(index);
            return;
        }

        entries_.pushBack(Entry{});
        for (std::size_t i = entries_.size() - 1; i > index; --i)
            entries_[i] = entries_[i - 1];
        entries_[index] = Entry{row, col, value};
    }

    long long get(unsigned long long row, unsigned long long col) const
    {
        const std::size_t index = lowerBound(row, col);
        if (index < entries_.size() && entries_[index].row == row && entries_[index].col == col)
            return entries_[index].value;
        return 0;
    }

    long long rowSum(unsigned long long row) const
    {
        long long sum = 0;
        for (std::size_t i = lowerBound(row, 0); i < entries_.size() && entries_[i].row == row; ++i)
            sum += entries_[i].value;
        return sum;
    }

    long long columnSum(unsigned long long col) const
    {
        long long sum = 0;
        for (const Entry& e : entries_)
            if (e.col == col)
                sum += e.value;
        return sum;
    }

    SparseMatrix transpose() const
    {
        SparseMatrix result;
        for (const Entry& e : entries_)
            result.add(e.col, e.row, e.value);
        return result;
    }

    std::size_t nonZeroCount() const { return entries_.size(); }
    const DynamicArray<Entry>& entries() const { return entries_; }
};

} // namespace nexa::dsa
