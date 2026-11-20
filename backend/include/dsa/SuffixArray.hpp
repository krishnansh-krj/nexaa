#pragma once

// NEXA DSA #28 - Suffix Array (+ Kasai LCP)
// Pattern search inside incident / event descriptions (case-insensitive).
// Documents are concatenated with a separator; each match is mapped back to
// its document. Build: prefix doubling with radix sort O(n log n);
// search: two binary searches O(m log n) + k.

#include "dsa/Sorting.hpp"

#include <cctype>
#include <cstddef>
#include <string>
#include <vector>

namespace nexa::dsa
{

class SuffixArray
{
public:
    struct Occurrence
    {
        std::size_t document = 0; // index of the document passed to build()
        std::size_t offset = 0;   // character offset inside that document
    };

private:
    std::string text_;
    std::vector<std::size_t> sa_;
    std::vector<std::size_t> lcp_;
    std::vector<std::size_t> docStart_;

    static constexpr char kSeparator = '\x01';

    static char fold(char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }

    void buildSuffixArray()
    {
        const std::size_t n = text_.size();
        sa_.assign(n, 0);
        if (n == 0)
            return;

        std::vector<std::size_t> rank(n), tmp(n);
        for (std::size_t i = 0; i < n; ++i)
        {
            sa_[i] = i;
            rank[i] = static_cast<unsigned char>(text_[i]);
        }

        for (std::size_t k = 1;; k <<= 1)
        {
            // Key = (rank[i], rank[i+k] + 1 or 0), sorted with stable radix sort.
            auto second = [&](std::size_t i) -> unsigned long long
            { return i + k < n ? static_cast<unsigned long long>(rank[i + k]) + 1 : 0ULL; };
            auto first = [&](std::size_t i) -> unsigned long long
            { return static_cast<unsigned long long>(rank[i]); };

            radixSort(sa_.data(), n, second);
            radixSort(sa_.data(), n, first);

            tmp[sa_[0]] = 0;
            for (std::size_t i = 1; i < n; ++i)
            {
                const std::size_t a = sa_[i - 1], b = sa_[i];
                const bool same = first(a) == first(b) && second(a) == second(b);
                tmp[b] = tmp[a] + (same ? 0 : 1);
            }
            rank.swap(tmp);
            if (rank[sa_[n - 1]] == n - 1 || k >= n)
                break;
        }
    }

    void buildLcp()
    {
        const std::size_t n = text_.size();
        lcp_.assign(n, 0);
        if (n == 0)
            return;
        std::vector<std::size_t> inverse(n);
        for (std::size_t i = 0; i < n; ++i)
            inverse[sa_[i]] = i;
        std::size_t h = 0;
        for (std::size_t i = 0; i < n; ++i)
        {
            if (inverse[i] == 0) { h = 0; continue; }
            const std::size_t j = sa_[inverse[i] - 1];
            while (i + h < n && j + h < n && text_[i + h] == text_[j + h] && text_[i + h] != kSeparator)
                ++h;
            lcp_[inverse[i]] = h;
            if (h > 0) --h;
        }
    }

    // <0 if suffix < pattern, 0 if pattern is a prefix of suffix, >0 otherwise.
    int compare(std::size_t suffix, const std::string& pattern) const
    {
        for (std::size_t i = 0; i < pattern.size(); ++i)
        {
            if (suffix + i >= text_.size()) return -1;
            const unsigned char a = static_cast<unsigned char>(text_[suffix + i]);
            const unsigned char b = static_cast<unsigned char>(pattern[i]);
            if (a != b) return a < b ? -1 : 1;
        }
        return 0;
    }

public:
    void build(const std::vector<std::string>& documents)
    {
        text_.clear();
        docStart_.clear();
        for (const std::string& doc : documents)
        {
            docStart_.push_back(text_.size());
            for (char c : doc)
                text_.push_back(c == kSeparator ? ' ' : fold(c));
            text_.push_back(kSeparator);
        }
        buildSuffixArray();
        buildLcp();
    }

    std::vector<Occurrence> find(const std::string& rawPattern, std::size_t limit = 200) const
    {
        std::vector<Occurrence> out;
        std::string pattern;
        for (char c : rawPattern) pattern.push_back(fold(c));
        if (pattern.empty() || sa_.empty())
            return out;

        std::size_t lo = 0, hi = sa_.size();
        while (lo < hi)
        {
            const std::size_t mid = lo + (hi - lo) / 2;
            if (compare(sa_[mid], pattern) < 0) lo = mid + 1; else hi = mid;
        }
        const std::size_t begin = lo;
        hi = sa_.size();
        while (lo < hi)
        {
            const std::size_t mid = lo + (hi - lo) / 2;
            if (compare(sa_[mid], pattern) <= 0) lo = mid + 1; else hi = mid;
        }

        for (std::size_t i = begin; i < lo && out.size() < limit; ++i)
        {
            const std::size_t pos = sa_[i];
            // Binary search the owning document.
            std::size_t a = 0, b = docStart_.size();
            while (b - a > 1)
            {
                const std::size_t m = (a + b) / 2;
                if (docStart_[m] <= pos) a = m; else b = m;
            }
            out.push_back(Occurrence{a, pos - docStart_[a]});
        }
        return out;
    }

    // Longest substring that occurs at least twice (within documents).
    std::string longestRepeated() const
    {
        std::size_t best = 0, at = 0;
        for (std::size_t i = 1; i < lcp_.size(); ++i)
            if (lcp_[i] > best) { best = lcp_[i]; at = sa_[i]; }
        return text_.substr(at, best);
    }

    std::size_t textLength() const { return text_.size(); }
    const std::vector<std::size_t>& suffixes() const { return sa_; }
};

} // namespace nexa::dsa
