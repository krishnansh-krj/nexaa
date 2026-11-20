#pragma once

// NEXA DSA #10, #11, #12 - Merge, Quick, Counting, Radix and Bucket Sort
//   mergeSort    : stable ranking of operational records      O(n log n), O(n) extra
//   quickSort    : in-place ordering                          O(n log n) avg, O(log n) stack
//   countingSort : bounded severity / priority ranks          O(n + k)
//   radixSort    : integer IDs (LSD, base 256)                O(8 (n + 256))
//   bucketSort   : normalised distance / score bands          O(n + b) avg

#include <cstddef>
#include <utility>

namespace nexa::dsa
{

namespace detail
{
template <typename T, typename Less>
void mergeRange(T* data, T* buffer, std::size_t lo, std::size_t mid, std::size_t hi, Less& less)
{
    std::size_t i = lo, j = mid, k = lo;
    while (i < mid && j < hi)
        buffer[k++] = less(data[j], data[i]) ? data[j++] : data[i++];
    while (i < mid) buffer[k++] = data[i++];
    while (j < hi) buffer[k++] = data[j++];
    for (std::size_t x = lo; x < hi; ++x)
        data[x] = buffer[x];
}

template <typename T, typename Less>
void mergeSortRange(T* data, T* buffer, std::size_t lo, std::size_t hi, Less& less)
{
    if (hi - lo < 2)
        return;
    const std::size_t mid = lo + (hi - lo) / 2;
    mergeSortRange(data, buffer, lo, mid, less);
    mergeSortRange(data, buffer, mid, hi, less);
    if (!less(data[mid], data[mid - 1]))
        return; // already ordered
    mergeRange(data, buffer, lo, mid, hi, less);
}

template <typename T, typename Less>
void insertionSort(T* data, std::size_t lo, std::size_t hi, Less& less)
{
    for (std::size_t i = lo + 1; i < hi; ++i)
    {
        T value = std::move(data[i]);
        std::size_t j = i;
        while (j > lo && less(value, data[j - 1]))
        {
            data[j] = std::move(data[j - 1]);
            --j;
        }
        data[j] = std::move(value);
    }
}
} // namespace detail

template <typename T, typename Less>
void mergeSort(T* data, std::size_t count, Less less)
{
    if (count < 2)
        return;
    T* buffer = new T[count];
    detail::mergeSortRange(data, buffer, 0, count, less);
    delete[] buffer;
}

template <typename T, typename Less>
void quickSort(T* data, std::size_t count, Less less)
{
    // Iterative on the larger side, recursive on the smaller: O(log n) stack.
    std::size_t lo = 0, hi = count; // half-open
    while (hi - lo > 16)
    {
        const std::size_t mid = lo + (hi - lo) / 2;
        const std::size_t last = hi - 1;
        if (less(data[mid], data[lo])) std::swap(data[mid], data[lo]);
        if (less(data[last], data[lo])) std::swap(data[last], data[lo]);
        if (less(data[last], data[mid])) std::swap(data[last], data[mid]);
        const T pivot = data[mid];

        std::size_t i = lo, j = last;
        while (true)
        {
            while (less(data[i], pivot)) ++i;
            while (less(pivot, data[j])) --j;
            if (i >= j) break;
            std::swap(data[i], data[j]);
            ++i;
            --j;
        }
        // [lo, j] <= pivot <= [j+1, hi)
        const std::size_t split = j + 1;
        if (split - lo < hi - split)
        {
            quickSort(data + lo, split - lo, less);
            lo = split;
        }
        else
        {
            quickSort(data + split, hi - split, less);
            hi = split;
        }
    }
    detail::insertionSort(data, lo, hi, less);
}

// Stable. keyOf(element) must return an integer in [0, maxKey].
template <typename T, typename KeyOf>
void countingSort(T* data, std::size_t count, std::size_t maxKey, KeyOf keyOf)
{
    if (count < 2)
        return;
    std::size_t* counts = new std::size_t[maxKey + 2]{};
    for (std::size_t i = 0; i < count; ++i)
        ++counts[static_cast<std::size_t>(keyOf(data[i])) + 1];
    for (std::size_t k = 1; k <= maxKey + 1; ++k)
        counts[k] += counts[k - 1];

    T* output = new T[count];
    for (std::size_t i = 0; i < count; ++i)
        output[counts[static_cast<std::size_t>(keyOf(data[i]))]++] = data[i];
    for (std::size_t i = 0; i < count; ++i)
        data[i] = output[i];

    delete[] output;
    delete[] counts;
}

// Stable LSD radix sort on an unsigned 64-bit key, one byte per pass.
template <typename T, typename KeyOf>
void radixSort(T* data, std::size_t count, KeyOf keyOf)
{
    if (count < 2)
        return;

    unsigned long long maxKey = 0;
    for (std::size_t i = 0; i < count; ++i)
    {
        const unsigned long long key = keyOf(data[i]);
        if (key > maxKey) maxKey = key;
    }

    T* output = new T[count];
    for (int shift = 0; shift < 64 && (maxKey >> shift) != 0; shift += 8)
    {
        std::size_t counts[257] = {};
        for (std::size_t i = 0; i < count; ++i)
            ++counts[((keyOf(data[i]) >> shift) & 0xFFu) + 1];
        for (int b = 1; b <= 256; ++b)
            counts[b] += counts[b - 1];
        for (std::size_t i = 0; i < count; ++i)
            output[counts[(keyOf(data[i]) >> shift) & 0xFFu]++] = data[i];
        for (std::size_t i = 0; i < count; ++i)
            data[i] = output[i];
    }
    delete[] output;
}

// Bucket index of `value` when [minValue, maxValue] is split into
// `bucketCount` equal-width bands.
inline std::size_t bucketOf(double value, double minValue, double maxValue, std::size_t bucketCount)
{
    if (bucketCount == 0 || maxValue <= minValue)
        return 0;
    const double ratio = (value - minValue) / (maxValue - minValue);
    std::size_t index = static_cast<std::size_t>(ratio * static_cast<double>(bucketCount));
    if (index >= bucketCount) index = bucketCount - 1;
    return index;
}

// Stable bucket sort by a floating-point key. Optionally reports each
// element's bucket in `bucketOut` (parallel to the sorted output).
template <typename T, typename KeyOf>
void bucketSort(T* data, std::size_t count, std::size_t bucketCount, KeyOf keyOf,
                std::size_t* bucketOut = nullptr)
{
    if (count == 0)
        return;
    if (bucketCount == 0)
        bucketCount = count;

    double minValue = keyOf(data[0]), maxValue = minValue;
    for (std::size_t i = 1; i < count; ++i)
    {
        const double key = keyOf(data[i]);
        if (key < minValue) minValue = key;
        if (key > maxValue) maxValue = key;
    }

    // Two passes: size the buckets, then place (keeps it stable and flat).
    std::size_t* starts = new std::size_t[bucketCount + 1]{};
    std::size_t* bucketIds = new std::size_t[count];
    for (std::size_t i = 0; i < count; ++i)
    {
        bucketIds[i] = bucketOf(keyOf(data[i]), minValue, maxValue, bucketCount);
        ++starts[bucketIds[i] + 1];
    }
    for (std::size_t b = 1; b <= bucketCount; ++b)
        starts[b] += starts[b - 1];

    T* output = new T[count];
    std::size_t* outputBucket = new std::size_t[count];
    std::size_t* cursor = new std::size_t[bucketCount];
    for (std::size_t b = 0; b < bucketCount; ++b)
        cursor[b] = starts[b];
    for (std::size_t i = 0; i < count; ++i)
    {
        const std::size_t position = cursor[bucketIds[i]]++;
        output[position] = data[i];
        outputBucket[position] = bucketIds[i];
    }

    auto less = [&keyOf](const T& a, const T& b) { return keyOf(a) < keyOf(b); };
    for (std::size_t b = 0; b < bucketCount; ++b)
        detail::insertionSort(output, starts[b], starts[b + 1], less);

    for (std::size_t i = 0; i < count; ++i)
    {
        data[i] = output[i];
        if (bucketOut) bucketOut[i] = outputBucket[i];
    }

    delete[] cursor;
    delete[] outputBucket;
    delete[] output;
    delete[] bucketIds;
    delete[] starts;
}

} // namespace nexa::dsa
