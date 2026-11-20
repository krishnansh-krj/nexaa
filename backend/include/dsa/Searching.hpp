#pragma once

// NEXA DSA #7, #8, #9 - Linear Search, Binary Search, Median Search
//   linearSearch : unsorted incident/resource collections      O(n)
//   binarySearch : sorted record arrays (by key)                O(log n)
//   quickSelect  : k-th smallest / median (Hoare selection)     O(n) avg
//   median       : median of response-time / distance data      O(n) avg

#include <cstddef>
#include <utility>

namespace nexa::dsa
{

constexpr std::ptrdiff_t kNotFound = -1;

template <typename T, typename Predicate>
std::ptrdiff_t linearSearch(const T* data, std::size_t count, Predicate match)
{
    for (std::size_t i = 0; i < count; ++i)
        if (match(data[i]))
            return static_cast<std::ptrdiff_t>(i);
    return kNotFound;
}

// `data` must be sorted ascending by keyOf(element).
template <typename T, typename Key, typename KeyOf>
std::ptrdiff_t binarySearch(const T* data, std::size_t count, const Key& key, KeyOf keyOf)
{
    std::size_t lo = 0, hi = count;
    while (lo < hi)
    {
        const std::size_t mid = lo + (hi - lo) / 2;
        const auto& midKey = keyOf(data[mid]);
        if (midKey == key)
            return static_cast<std::ptrdiff_t>(mid);
        if (midKey < key) lo = mid + 1; else hi = mid;
    }
    return kNotFound;
}

// First index whose key is >= key (count if none).
template <typename T, typename Key, typename KeyOf>
std::size_t lowerBound(const T* data, std::size_t count, const Key& key, KeyOf keyOf)
{
    std::size_t lo = 0, hi = count;
    while (lo < hi)
    {
        const std::size_t mid = lo + (hi - lo) / 2;
        if (keyOf(data[mid]) < key) lo = mid + 1; else hi = mid;
    }
    return lo;
}

// Rearranges data so that data[k] is the k-th smallest (0-based) element.
template <typename T, typename Less>
const T& quickSelect(T* data, std::size_t count, std::size_t k, Less less)
{
    std::size_t lo = 0, hi = count - 1;
    while (lo < hi)
    {
        // Median-of-three pivot keeps already-sorted input efficient.
        const std::size_t mid = lo + (hi - lo) / 2;
        if (less(data[mid], data[lo])) std::swap(data[mid], data[lo]);
        if (less(data[hi], data[lo])) std::swap(data[hi], data[lo]);
        if (less(data[hi], data[mid])) std::swap(data[hi], data[mid]);
        const T pivot = data[mid];

        std::size_t i = lo, j = hi;
        while (i <= j)
        {
            while (less(data[i], pivot)) ++i;
            while (less(pivot, data[j])) --j;
            if (i <= j)
            {
                std::swap(data[i], data[j]);
                ++i;
                if (j == 0) break;
                --j;
            }
        }

        if (k <= j) hi = j;
        else if (k >= i) lo = i;
        else break;
    }
    return data[k];
}

// Median of a numeric sample (copies the input; average of the two middles
// for an even count). Returns 0 for an empty sample.
inline double median(const double* values, std::size_t count)
{
    if (count == 0)
        return 0.0;

    double* work = new double[count];
    for (std::size_t i = 0; i < count; ++i)
        work[i] = values[i];

    auto less = [](double a, double b) { return a < b; };
    const double upper = quickSelect(work, count, count / 2, less);
    double result = upper;
    if (count % 2 == 0)
    {
        // Lower middle is the max of the left partition.
        double lowerMiddle = work[0];
        for (std::size_t i = 1; i < count / 2; ++i)
            if (work[i] > lowerMiddle)
                lowerMiddle = work[i];
        result = (lowerMiddle + upper) / 2.0;
    }

    delete[] work;
    return result;
}

} // namespace nexa::dsa
