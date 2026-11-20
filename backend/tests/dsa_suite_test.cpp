// NEXA - verification suite for the synopsis data structures (include/dsa).
// Every structure is checked against a brute-force reference on random data.

#include "dsa/AVLTree.hpp"
#include "dsa/AdjacencyMatrix.hpp"
#include "dsa/BinarySearchTree.hpp"
#include "dsa/DisjointSet.hpp"
#include "dsa/DynamicArray.hpp"
#include "dsa/Geo.hpp"
#include "dsa/HashTable.hpp"
#include "dsa/IntervalTree.hpp"
#include "dsa/KDTree.hpp"
#include "dsa/KaryTree.hpp"
#include "dsa/LinkedList.hpp"
#include "dsa/MinimumSpanningTree.hpp"
#include "dsa/MultiLinkedList.hpp"
#include "dsa/QuadTree.hpp"
#include "dsa/Queue.hpp"
#include "dsa/RangeTree.hpp"
#include "dsa/RedBlackTree.hpp"
#include "dsa/Searching.hpp"
#include "dsa/SegmentTree.hpp"
#include "dsa/Sorting.hpp"
#include "dsa/SparseMatrix.hpp"
#include "dsa/Stack.hpp"
#include "dsa/SuffixArray.hpp"
#include "dsa/Trie.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

using namespace nexa::dsa;

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                                 \
    do                                                                              \
    {                                                                               \
        ++g_checks;                                                                 \
        if (!(cond))                                                                \
        {                                                                           \
            ++g_failures;                                                           \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << "  " #cond "\n"; \
        }                                                                           \
    } while (0)

static std::mt19937_64 rng(20261118ULL);

static void testLinear()
{
    DynamicArray<int> arr;
    for (int i = 0; i < 100; ++i) arr.pushBack(i * 2);
    CHECK(arr.size() == 100 && arr[50] == 100);
    CHECK(arr.removeAt(0) && arr[0] == 2 && arr.size() == 99);
    DynamicArray<int> copy(arr);
    CHECK(copy.size() == 99 && copy[98] == 198);

    Stack<int> st;
    for (int i = 1; i <= 5; ++i) st.push(i);
    int top = 0;
    CHECK(*st.peek() == 5);
    CHECK(st.pop(top) && top == 5 && st.size() == 4);

    Queue<int> q;
    for (int i = 0; i < 20; ++i) q.enqueue(i);
    int front = -1;
    for (int i = 0; i < 10; ++i) q.dequeue(front);
    for (int i = 20; i < 40; ++i) q.enqueue(i); // forces wrap + grow
    std::vector<int> order;
    while (q.dequeue(front)) order.push_back(front);
    bool fifo = order.size() == 30;
    for (std::size_t i = 0; fifo && i < order.size(); ++i) fifo = order[i] == static_cast<int>(i) + 10;
    CHECK(fifo);

    LinkedList<int> list;
    auto less = [](int a, int b) { return a < b; };
    for (int v : {5, 1, 4, 2, 3, 3}) list.insertOrdered(v, less);
    std::vector<int> seen;
    list.forEach([&](int v) { seen.push_back(v); });
    CHECK((seen == std::vector<int>{1, 2, 3, 3, 4, 5}));
    CHECK(list.removeFirst([](int v) { return v == 3; }) && list.size() == 5);
    std::vector<int> rev;
    list.forEachReverse([&](int v) { rev.push_back(v); });
    CHECK((rev == std::vector<int>{5, 4, 3, 2, 1}));
}

static void testRelationships()
{
    MultiLinkedList<int> mll;
    mll.link(1, 10, 2);
    mll.link(1, 20, 1);
    mll.link(2, 10, 3);
    CHECK(mll.cellCount() == 3 && mll.rowCount() == 2 && mll.columnCount() == 2);
    int unitsIncident1 = 0;
    mll.forEachInRow(1, [&](const auto& c) { unitsIncident1 += c.payload; });
    CHECK(unitsIncident1 == 3);
    int incidentsOnResource10 = 0;
    mll.forEachInColumn(10, [&](const auto&) { ++incidentsOnResource10; });
    CHECK(incidentsOnResource10 == 2 && mll.columnDegree(20) == 1);

    SparseMatrix m;
    std::map<std::pair<unsigned long long, unsigned long long>, long long> ref;
    for (int i = 0; i < 500; ++i)
    {
        const unsigned long long r = rng() % 30, c = rng() % 30;
        const long long v = static_cast<long long>(rng() % 7) - 3;
        m.add(r, c, v);
        ref[{r, c}] += v;
    }
    bool same = true;
    std::size_t nonZero = 0;
    for (const auto& [key, value] : ref)
    {
        if (value != 0) ++nonZero;
        same = same && m.get(key.first, key.second) == value;
    }
    CHECK(same && m.nonZeroCount() == nonZero);
    long long row5 = 0;
    for (const auto& [key, value] : ref) if (key.first == 5) row5 += value;
    CHECK(m.rowSum(5) == row5);
    CHECK(m.transpose().get(3, 7) == m.get(7, 3));
}

static void testSearchAndSort()
{
    for (int round = 0; round < 30; ++round)
    {
        const std::size_t n = 1 + rng() % 400;
        std::vector<long long> v(n);
        for (auto& x : v) x = static_cast<long long>(rng() % 1000);
        std::vector<long long> expected = v;
        std::sort(expected.begin(), expected.end());

        auto less = [](long long a, long long b) { return a < b; };
        std::vector<long long> a = v; mergeSort(a.data(), a.size(), less);
        std::vector<long long> b = v; quickSort(b.data(), b.size(), less);
        std::vector<long long> c = v; countingSort(c.data(), c.size(), 999, [](long long x) { return static_cast<std::size_t>(x); });
        std::vector<long long> d = v; radixSort(d.data(), d.size(), [](long long x) { return static_cast<unsigned long long>(x); });
        std::vector<long long> e = v; bucketSort(e.data(), e.size(), 10, [](long long x) { return static_cast<double>(x); });
        CHECK(a == expected);
        CHECK(b == expected);
        CHECK(c == expected);
        CHECK(d == expected);
        CHECK(e == expected);

        const long long probe = expected[rng() % n];
        const std::ptrdiff_t hit = binarySearch(expected.data(), n, probe, [](long long x) { return x; });
        CHECK(hit >= 0 && expected[static_cast<std::size_t>(hit)] == probe);
        CHECK(binarySearch(expected.data(), n, 5000LL, [](long long x) { return x; }) == kNotFound);
        CHECK(linearSearch(v.data(), n, [&](long long x) { return x == probe; }) >= 0);

        const std::size_t k = rng() % n;
        std::vector<long long> s = v;
        CHECK(quickSelect(s.data(), n, k, less) == expected[k]);

        std::vector<double> dv;
        for (long long x : v) dv.push_back(static_cast<double>(x));
        const double med = median(dv.data(), dv.size());
        const double ref = n % 2 ? static_cast<double>(expected[n / 2])
                                 : (expected[n / 2 - 1] + expected[n / 2]) / 2.0;
        CHECK(std::fabs(med - ref) < 1e-9);
    }

    // Stability: merge, counting, radix, bucket keep equal keys in input order.
    struct Rec { int key; int seq; };
    std::vector<Rec> recs;
    for (int i = 0; i < 200; ++i) recs.push_back({static_cast<int>(rng() % 5), i});
    auto stable = [](const std::vector<Rec>& r)
    {
        for (std::size_t i = 1; i < r.size(); ++i)
            if (r[i - 1].key > r[i].key || (r[i - 1].key == r[i].key && r[i - 1].seq > r[i].seq)) return false;
        return true;
    };
    std::vector<Rec> r1 = recs; mergeSort(r1.data(), r1.size(), [](const Rec& x, const Rec& y) { return x.key < y.key; });
    std::vector<Rec> r2 = recs; countingSort(r2.data(), r2.size(), 4, [](const Rec& x) { return static_cast<std::size_t>(x.key); });
    std::vector<Rec> r3 = recs; radixSort(r3.data(), r3.size(), [](const Rec& x) { return static_cast<unsigned long long>(x.key); });
    std::vector<Rec> r4 = recs; bucketSort(r4.data(), r4.size(), 5, [](const Rec& x) { return static_cast<double>(x.key); });
    CHECK(stable(r1) && stable(r2) && stable(r3) && stable(r4));

    // Large sorted input must not degrade quick sort.
    std::vector<int> big(200000);
    for (std::size_t i = 0; i < big.size(); ++i) big[i] = static_cast<int>(i);
    quickSort(big.data(), big.size(), [](int x, int y) { return x < y; });
    CHECK(std::is_sorted(big.begin(), big.end()));
}

static void testTrees()
{
    KaryTree tree("ALL");
    auto* natural = tree.addChild(tree.root(), "NATURAL");
    auto* hydro = tree.addChild(natural, "HYDROLOGICAL");
    auto* flood = tree.addChild(hydro, "FLOOD");
    tree.addChild(tree.root(), "HUMAN_MADE");
    flood->members.push_back(2);
    CHECK(tree.find("FLOOD") == flood && KaryTree::depthOf(flood) == 3);
    CHECK((KaryTree::pathTo(flood) == std::vector<std::string>{"ALL", "NATURAL", "HYDROLOGICAL", "FLOOD"}));
    CHECK(KaryTree::totalMembers(tree.root()) == 1 && tree.root()->childCount == 2 && tree.size() == 5);

    BinarySearchTree<long long, int> bst;
    AVLTree<long long, int> avl;
    RedBlackTree<long long, int> rb;
    std::set<long long> ref;
    for (int i = 0; i < 3000; ++i)
    {
        const long long k = static_cast<long long>(rng() % 5000);
        bst.insert(k, static_cast<int>(k));
        avl.insert(k, static_cast<int>(k));
        rb.insert(k, static_cast<int>(k));
        ref.insert(k);
    }
    CHECK(bst.size() == ref.size() && avl.size() == ref.size() && rb.size() == ref.size());
    CHECK(avl.isValid() && rb.isValid());
    bool found = true;
    for (long long k : ref) found = found && bst.find(k) && avl.find(k) && rb.find(k) && *avl.find(k) == k;
    CHECK(found && !avl.find(99999) && !rb.find(-1));

    int removed = 0;
    for (long long k = 0; k < 5000; k += 3)
    {
        const bool inRef = ref.erase(k) > 0;
        const bool a = avl.remove(k), b = bst.remove(k);
        if (a != inRef || b != inRef) ++removed;
    }
    CHECK(removed == 0 && avl.isValid() && avl.size() == ref.size() && bst.size() == ref.size());
    std::vector<long long> inorder;
    avl.inorder([&](long long k, int) { inorder.push_back(k); });
    CHECK(inorder == std::vector<long long>(ref.begin(), ref.end()));
    std::vector<long long> inBst;
    bst.inorder([&](long long k, int) { inBst.push_back(k); });
    CHECK(inBst == inorder);

    std::vector<long long> ranged;
    avl.rangeQuery(1000, 1200, [&](long long k, int) { ranged.push_back(k); });
    std::vector<long long> expectRange(ref.lower_bound(1000), ref.upper_bound(1200));
    CHECK(ranged == expectRange);

    // Sorted insertion: BST degenerates, AVL / RB stay logarithmic.
    BinarySearchTree<int, int> bstSorted;
    AVLTree<int, int> avlSorted;
    RedBlackTree<int, int> rbSorted;
    for (int i = 0; i < 1023; ++i) { bstSorted.insert(i, i); avlSorted.insert(i, i); rbSorted.insert(i, i); }
    CHECK(bstSorted.height() == 1023);
    CHECK(avlSorted.height() <= 11 && rbSorted.height() <= 20 && rbSorted.isValid());

    ChainedHashTable<std::string, int> chained(4);
    LinearProbingHashTable<unsigned long long, int> probing(4);
    for (int i = 0; i < 2000; ++i)
    {
        chained.put("NEXA-RES-" + std::to_string(i), i);
        probing.put(static_cast<unsigned long long>(i) * 7919ULL, i);
    }
    CHECK(chained.size() == 2000 && probing.size() == 2000 && chained.loadFactor() <= 0.75);
    CHECK(chained.get("NEXA-RES-1234") && *chained.get("NEXA-RES-1234") == 1234 && !chained.get("missing"));
    CHECK(probing.get(42ULL * 7919ULL) && *probing.get(42ULL * 7919ULL) == 42);
    CHECK(probing.remove(42ULL * 7919ULL) && !probing.get(42ULL * 7919ULL) && probing.size() == 1999);
    CHECK(probing.get(43ULL * 7919ULL) != nullptr); // probe chain survives tombstone
    probing.put(42ULL * 7919ULL, 4242);
    CHECK(*probing.get(42ULL * 7919ULL) == 4242);
    CHECK(chained.remove("NEXA-RES-7") && chained.size() == 1999);
}

static void testGraphs()
{
    for (int round = 0; round < 20; ++round)
    {
        const std::size_t n = 2 + rng() % 40;
        std::vector<WeightedEdge> edges;
        AdjacencyMatrix matrix(n);
        for (std::size_t i = 0; i < n * 2; ++i)
        {
            const std::size_t u = rng() % n, v = rng() % n;
            if (u == v) continue;
            const double w = 1.0 + static_cast<double>(rng() % 100);
            edges.push_back({u, v, w, i});
            matrix.setUndirected(u, v, w);
        }
        const SpanningForest k = kruskalMST(n, edges);
        const SpanningForest p = primMST(matrix);
        CHECK(std::fabs(k.totalWeight - p.totalWeight) < 1e-9);
        CHECK(k.components == p.components);
        CHECK(k.edges.size() + k.components == n);
    }
    DisjointSet ds(5);
    ds.unite(0, 1);
    ds.unite(3, 4);
    CHECK(ds.components() == 3 && ds.find(1) == ds.find(0) && ds.find(2) != ds.find(3));
}

static void testSpatial()
{
    std::vector<GeoPoint> pts;
    std::vector<unsigned long long> ids;
    for (int i = 0; i < 1500; ++i)
    {
        pts.push_back({28.40 + static_cast<double>(rng() % 100000) / 200000.0,
                       77.00 + static_cast<double>(rng() % 100000) / 200000.0});
        ids.push_back(static_cast<unsigned long long>(i) + 1);
    }

    KDTree kd;
    kd.build(pts);
    for (int q = 0; q < 50; ++q)
    {
        const double lat = 28.40 + static_cast<double>(rng() % 1000) / 2000.0;
        const double lng = 77.00 + static_cast<double>(rng() % 1000) / 2000.0;
        const auto hits = kd.kNearest(lat, lng, 5);
        // Brute force in the same projection.
        std::vector<std::pair<double, std::size_t>> brute;
        double latSum = 0;
        for (const auto& p : pts) latSum += p.lat;
        const double c = std::cos(latSum / pts.size() * 3.14159265358979323846 / 180.0);
        for (std::size_t i = 0; i < pts.size(); ++i)
        {
            const double dx = (pts[i].lng - lng) * 111.320 * c, dy = (pts[i].lat - lat) * 110.574;
            brute.push_back({std::sqrt(dx * dx + dy * dy), i});
        }
        std::sort(brute.begin(), brute.end());
        bool ok = hits.size() == 5;
        for (std::size_t i = 0; ok && i < 5; ++i) ok = std::fabs(hits[i].planarKm - brute[i].first) < 1e-9;
        CHECK(ok);
    }
    CHECK(kd.size() == pts.size());

    const GeoBox world{28.0, 29.0, 76.5, 77.5};
    QuadTree quad(world, 4);
    RangeTree range;
    range.build(pts, ids);
    for (std::size_t i = 0; i < pts.size(); ++i) quad.insert(pts[i].lat, pts[i].lng, ids[i]);
    CHECK(quad.size() == pts.size() && quad.depth() > 1);

    for (int q = 0; q < 40; ++q)
    {
        const double a = 28.40 + static_cast<double>(rng() % 1000) / 2000.0;
        const double b = 77.00 + static_cast<double>(rng() % 1000) / 2000.0;
        const GeoBox box{a, a + 0.08, b, b + 0.12};
        std::set<unsigned long long> expect;
        for (std::size_t i = 0; i < pts.size(); ++i)
            if (box.contains(pts[i].lat, pts[i].lng)) expect.insert(ids[i]);
        auto fromQuad = quad.queryRange(box);
        auto fromRange = range.query(box);
        CHECK(std::set<unsigned long long>(fromQuad.begin(), fromQuad.end()) == expect && fromQuad.size() == expect.size());
        CHECK(std::set<unsigned long long>(fromRange.begin(), fromRange.end()) == expect && fromRange.size() == expect.size());
    }
    std::size_t counts[4];
    quad.quadrantCounts(counts);
    CHECK(counts[0] + counts[1] + counts[2] + counts[3] == pts.size());

    const double d = haversineKm(28.6139, 77.2090, 19.0760, 72.8777); // Delhi -> Mumbai
    CHECK(d > 1140 && d < 1160);
}

static void testRangeAnalytics()
{
    std::vector<IntervalTree::Interval> intervals;
    for (int i = 0; i < 800; ++i)
    {
        const long long s = static_cast<long long>(rng() % 10000);
        intervals.push_back({s, s + static_cast<long long>(rng() % 500), static_cast<unsigned long long>(i)});
    }
    IntervalTree it;
    it.build(intervals);
    for (int q = 0; q < 50; ++q)
    {
        const long long from = static_cast<long long>(rng() % 10000), to = from + static_cast<long long>(rng() % 800);
        std::set<unsigned long long> expect;
        for (const auto& iv : intervals) if (iv.start <= to && iv.end >= from) expect.insert(iv.id);
        std::set<unsigned long long> got;
        for (const auto& iv : it.overlapping(from, to)) got.insert(iv.id);
        CHECK(got == expect);
    }

    std::vector<long long> values(257);
    for (auto& v : values) v = static_cast<long long>(rng() % 50);
    SegmentTree seg(values);
    for (int q = 0; q < 200; ++q)
    {
        if (q % 5 == 0)
        {
            const std::size_t i = rng() % values.size();
            values[i] = static_cast<long long>(rng() % 50);
            seg.set(i, values[i]);
        }
        std::size_t l = rng() % values.size(), r = rng() % values.size();
        if (l > r) std::swap(l, r);
        long long sum = 0, mx = 0;
        for (std::size_t i = l; i <= r; ++i) { sum += values[i]; mx = std::max(mx, values[i]); }
        CHECK(seg.rangeSum(l, r) == sum && seg.rangeMax(l, r) == mx);
    }
}

static void testText()
{
    Trie trie;
    trie.insertPhrase("Warehouse Fire Sector 62", 1);
    trie.insertPhrase("Flood in Yamuna Bank", 2);
    trie.insert("NEXA-INC-001", 1);
    trie.insert("NEXA-INC-002", 2);
    trie.insert("NEXA-RES-005", 105);
    auto fl = trie.withPrefix("fl");
    CHECK(!fl.empty() && fl[0].id == 2);
    auto inc = trie.withPrefix("NEXA-INC");
    CHECK(inc.size() == 2 && inc[0].word == "nexa-inc-001");
    CHECK(trie.contains("FIRE") && !trie.contains("fir") && trie.withPrefix("zzz").empty());
    CHECK(trie.withPrefix("nexa", 2).size() == 2);

    std::vector<std::string> docs = {
        "Fire reported at warehouse; fire spreading to the second floor.",
        "Flood water rising near the bank. Rescue boats requested.",
        "Ambulance dispatched to the warehouse fire."};
    SuffixArray sa;
    sa.build(docs);
    auto fire = sa.find("FIRE");
    CHECK(fire.size() == 3);
    std::size_t warehouse = 0;
    for (const auto& o : sa.find("warehouse")) { (void)o; ++warehouse; }
    CHECK(warehouse == 2);
    bool ok = true;
    for (const auto& o : sa.find("rescue"))
        ok = ok && o.document == 1 && docs[1].substr(o.offset, 6) == "Rescue";
    CHECK(ok && sa.find("rescue").size() == 1 && sa.find("tsunami").empty());

    // Cross-check against naive search on random text.
    std::string alphabet = "abc ";
    std::vector<std::string> randomDocs(5);
    for (auto& d : randomDocs) for (int i = 0; i < 300; ++i) d.push_back(alphabet[rng() % alphabet.size()]);
    SuffixArray rsa;
    rsa.build(randomDocs);
    for (const std::string pattern : {"ab", "cab", "a a", "bbb"})
    {
        std::size_t naive = 0;
        for (const auto& d : randomDocs)
            for (std::size_t p = d.find(pattern); p != std::string::npos; p = d.find(pattern, p + 1)) ++naive;
        CHECK(rsa.find(pattern, 100000).size() == naive);
    }
    CHECK(!rsa.longestRepeated().empty());
}

int main()
{
    testLinear();
    testRelationships();
    testSearchAndSort();
    testTrees();
    testGraphs();
    testSpatial();
    testRangeAnalytics();
    testText();

    std::cout << "NEXA DSA suite: " << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
