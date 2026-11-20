#include "intelligence/IntelligenceEngine.hpp"

#include "algorithms/RequestPrioritizer.hpp"
#include "dsa/AVLTree.hpp"
#include "dsa/AdjacencyMatrix.hpp"
#include "dsa/BinarySearchTree.hpp"
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
#include "utilities/TypeMapping.hpp"

#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <string>
#include <vector>

namespace nexa::intel
{

namespace
{

Json::Value uid(unsigned long long v) { return Json::Value(static_cast<Json::UInt64>(v)); }
Json::Value i64(long long v) { return Json::Value(static_cast<Json::Int64>(v)); }

double round3(double v) { return std::round(v * 1000.0) / 1000.0; }

// ---------- Hash-table indexes (DSA #21) ----------

void indexIncidents(const Snapshot& s, dsa::ChainedHashTable<unsigned long long, std::size_t>& table)
{
    for (std::size_t i = 0; i < s.incidents.size(); ++i)
        table.put(s.incidents[i].id, i);
}

void indexResources(const Snapshot& s, dsa::ChainedHashTable<unsigned long long, std::size_t>& table)
{
    for (std::size_t i = 0; i < s.resources.size(); ++i)
        table.put(s.resources[i].id, i);
}

const IncidentRecord& requireIncident(const Snapshot& s, unsigned long long incidentId)
{
    dsa::ChainedHashTable<unsigned long long, std::size_t> table;
    indexIncidents(s, table);
    const std::size_t* index = table.get(incidentId);
    if (!index)
        throw EngineError("Incident not found", "INCIDENT_NOT_FOUND", 404);
    return s.incidents[*index];
}

Json::Value resourceSummary(const ResourceRecord& r)
{
    Json::Value item;
    item["resource_id"] = uid(r.id);
    item["resource_code"] = r.code;
    item["resource_name"] = r.name;
    item["resource_type"] = r.type;
    item["status"] = r.status;
    item["available_units"] = r.availableUnits;
    item["total_units"] = r.totalUnits;
    item["latitude"] = r.hasLocation ? Json::Value(r.lat) : Json::Value(Json::nullValue);
    item["longitude"] = r.hasLocation ? Json::Value(r.lng) : Json::Value(Json::nullValue);
    return item;
}

Json::Value incidentSummary(const IncidentRecord& i)
{
    Json::Value item;
    item["incident_id"] = uid(i.id);
    item["incident_code"] = i.code;
    item["title"] = i.title;
    item["incident_type"] = i.type;
    item["severity"] = i.severity;
    item["status"] = i.status;
    item["latitude"] = i.hasLocation ? Json::Value(i.lat) : Json::Value(Json::nullValue);
    item["longitude"] = i.hasLocation ? Json::Value(i.lng) : Json::Value(Json::nullValue);
    return item;
}

Json::Value structuresUsed(std::initializer_list<const char*> names)
{
    Json::Value list(Json::arrayValue);
    for (const char* n : names)
        list.append(n);
    return list;
}

} // namespace

// =====================================================================
// CATALOGUE
// =====================================================================

Json::Value IntelligenceEngine::catalogue()
{
    struct Entry { int no; const char* name; const char* file; const char* application; const char* endpoint; const char* complexity; };
    static const Entry entries[] = {
        {1, "Array (dynamic)", "dsa/DynamicArray.hpp", "Resource registry and record management", "/api/dsa/resources/rank", "access O(1), append O(1) amortised"},
        {2, "Stack", "dsa/Stack.hpp", "Recent operational action history", "/api/dsa/operations/history", "push/pop O(1)"},
        {3, "Queue", "dsa/Queue.hpp", "First-come emergency request processing", "/api/dsa/requests/queue", "enqueue/dequeue O(1)"},
        {4, "Linked List", "dsa/LinkedList.hpp", "Incident event timeline", "/api/dsa/incidents/timeline", "insert O(1)/O(n) ordered, search O(n)"},
        {5, "Multi-Linked List", "dsa/MultiLinkedList.hpp", "Incident <-> resource deployment relationships", "/api/dsa/allocations/matrix", "link O(1) after header lookup"},
        {6, "Sparse Matrix", "dsa/SparseMatrix.hpp", "Incident x resource allocation matrix", "/api/dsa/allocations/matrix", "space O(k), get O(log k)"},
        {7, "Linear Search", "dsa/Searching.hpp", "Filtering unsorted resource / incident collections", "/api/dsa/resources/rank", "O(n)"},
        {8, "Binary Search", "dsa/Searching.hpp", "Lookup in code-sorted incident array", "/api/dsa/incidents/index", "O(log n)"},
        {9, "Median Search (quick-select)", "dsa/Searching.hpp", "Median distance / allocation duration", "/api/dsa/resources/rank", "O(n) average"},
        {10, "Merge Sort / Quick Sort", "dsa/Sorting.hpp", "Ranking candidate resources", "/api/dsa/resources/rank", "O(n log n)"},
        {11, "Radix / Counting Sort", "dsa/Sorting.hpp", "Incident IDs and bounded severity ranks", "/api/dsa/incidents/index", "O(n + k)"},
        {12, "Bucket Sort", "dsa/Sorting.hpp", "Distance bands for candidate resources", "/api/dsa/resources/rank", "O(n + b) average"},
        {13, "K-ary Tree", "dsa/KaryTree.hpp", "Hierarchical disaster classification", "/api/dsa/incidents/classification", "insert O(1), find O(n)"},
        {14, "BST / AVL Tree", "dsa/BinarySearchTree.hpp, dsa/AVLTree.hpp", "Incident ID index and range query", "/api/dsa/incidents/index", "AVL O(log n)"},
        {15, "Binary Heap / Priority Queue", "data_structures/MinHeap.hpp, data_structures/PriorityQueue.hpp", "Dijkstra frontier; severity-based request prioritisation", "/api/requests/prioritized, /api/network/route", "O(log n)"},
        {16, "Red-Black Tree", "dsa/RedBlackTree.hpp", "Comparative balanced index (vs BST / AVL)", "/api/dsa/incidents/index", "O(log n)"},
        {17, "Graph (Adjacency List / Matrix)", "data_structures/Graph.hpp, dsa/AdjacencyMatrix.hpp", "Road network", "/api/network/*, /api/dsa/network/mst", "list O(V+E), matrix O(V^2)"},
        {18, "BFS / DFS", "algorithms/BFS.hpp, algorithms/DFS.hpp", "Reachability and connected-area exploration", "/api/network/traverse/bfs, /api/network/traverse/dfs", "O(V+E)"},
        {19, "Shortest Path (Dijkstra)", "algorithms/Dijkstra.hpp", "Resource-to-incident routing", "/api/network/route", "O((V+E) log V)"},
        {20, "Minimum Spanning Tree (Kruskal + Prim)", "dsa/MinimumSpanningTree.hpp, dsa/DisjointSet.hpp", "Minimum-cost network connectivity", "/api/dsa/network/mst", "Kruskal O(E log E), Prim O(V^2)"},
        {21, "Hash Table (chaining + linear probing)", "dsa/HashTable.hpp", "ID / code lookup", "/api/dsa/incidents/index, all /api/dsa endpoints", "O(1) average"},
        {22, "KD Tree", "dsa/KDTree.hpp", "Nearest-resource search", "/api/dsa/spatial/nearest", "O(log n) average"},
        {23, "Quad Tree", "dsa/QuadTree.hpp", "Regional incident / resource partitioning", "/api/dsa/spatial/region", "O(log n + k) average"},
        {24, "Range Tree", "dsa/RangeTree.hpp", "Rectangle range query", "/api/dsa/spatial/region", "O(log^2 n + k)"},
        {25, "Interval Tree", "dsa/IntervalTree.hpp", "Allocations active in a time window", "/api/dsa/analytics/availability", "O(log n + k)"},
        {26, "Segment Tree", "dsa/SegmentTree.hpp", "Load per time bucket: total and peak", "/api/dsa/analytics/availability", "O(log n)"},
        {27, "Trie", "dsa/Trie.hpp", "Prefix search over codes, titles, names", "/api/dsa/search/prefix", "O(L)"},
        {28, "Suffix Array", "dsa/SuffixArray.hpp", "Pattern search in descriptions", "/api/dsa/search/pattern", "build O(n log n), search O(m log n)"},
    };

    Json::Value list(Json::arrayValue);
    for (const Entry& e : entries)
    {
        Json::Value item;
        item["no"] = e.no;
        item["structure"] = e.name;
        item["source"] = e.file;
        item["application"] = e.application;
        item["endpoint"] = e.endpoint;
        item["complexity"] = e.complexity;
        list.append(item);
    }

    Json::Value data;
    data["structure_count"] = static_cast<int>(sizeof(entries) / sizeof(entries[0]));
    data["structures"] = list;
    return data;
}

// =====================================================================
// SPATIAL: KD TREE
// =====================================================================

Json::Value IntelligenceEngine::nearestResources(const Snapshot& s, double lat, double lng, std::size_t k,
                                                 const std::string& type, bool onlyAvailable)
{
    if (lat < -90 || lat > 90 || lng < -180 || lng > 180)
        throw EngineError("Latitude/longitude out of range", "INVALID_COORDINATES", 400);
    if (k == 0) k = 5;
    if (k > 50) k = 50;

    const std::string wanted = normalizeType(type);

    // Linear filter into the candidate array; KD tree over the survivors.
    std::vector<std::size_t> candidates;
    std::vector<dsa::GeoPoint> points;
    for (std::size_t i = 0; i < s.resources.size(); ++i)
    {
        const ResourceRecord& r = s.resources[i];
        if (!r.hasLocation) continue;
        if (!wanted.empty() && normalizeType(r.type) != wanted) continue;
        if (onlyAvailable && (r.status != "AVAILABLE" || r.availableUnits == 0)) continue;
        candidates.push_back(i);
        points.push_back({r.lat, r.lng});
    }

    dsa::KDTree tree;
    tree.build(points);
    const auto hits = tree.kNearest(lat, lng, k);

    Json::Value results(Json::arrayValue);
    for (const auto& hit : hits)
    {
        const ResourceRecord& r = s.resources[candidates[hit.index]];
        Json::Value item = resourceSummary(r);
        item["distance_km"] = round3(dsa::haversineKm(lat, lng, r.lat, r.lng));
        results.append(item);
    }

    Json::Value data;
    data["origin"]["latitude"] = lat;
    data["origin"]["longitude"] = lng;
    data["filter"]["resource_type"] = wanted.empty() ? Json::Value(Json::nullValue) : Json::Value(wanted);
    data["filter"]["only_available"] = onlyAvailable;
    data["k"] = static_cast<Json::UInt64>(k);
    data["candidate_count"] = static_cast<Json::UInt64>(candidates.size());
    data["kd_nodes_visited"] = static_cast<Json::UInt64>(tree.lastVisited());
    data["results"] = results;
    data["structures_used"] = structuresUsed({"Linear Search", "KD Tree", "Haversine distance"});
    return data;
}

Json::Value IntelligenceEngine::nearestResourcesToIncident(const Snapshot& s, unsigned long long incidentId,
                                                           std::size_t k, const std::string& type, bool onlyAvailable)
{
    const IncidentRecord& incident = requireIncident(s, incidentId);
    if (!incident.hasLocation)
        throw EngineError("Incident has no recorded location", "INCIDENT_WITHOUT_LOCATION", 422);

    Json::Value data = nearestResources(s, incident.lat, incident.lng, k, type, onlyAvailable);
    data["incident"] = incidentSummary(incident);
    data["structures_used"].append("Hash Table (incident lookup)");
    return data;
}

// =====================================================================
// SPATIAL: QUAD TREE + RANGE TREE
// =====================================================================

Json::Value IntelligenceEngine::regionQuery(const Snapshot& s, double minLat, double maxLat, double minLng, double maxLng)
{
    if (minLat > maxLat || minLng > maxLng)
        throw EngineError("min values must not exceed max values", "INVALID_REGION", 400);

    const dsa::GeoBox world{-90.0, 90.0, -180.0, 180.0};
    const dsa::GeoBox box{minLat, maxLat, minLng, maxLng};

    dsa::QuadTree incidentQuad(world, 4, 24);
    dsa::QuadTree resourceQuad(world, 4, 24);
    std::vector<dsa::GeoPoint> resourcePoints, incidentPoints;
    std::vector<unsigned long long> resourceIds, incidentIds;

    for (const IncidentRecord& i : s.incidents)
        if (i.hasLocation)
        {
            incidentQuad.insert(i.lat, i.lng, i.id);
            incidentPoints.push_back({i.lat, i.lng});
            incidentIds.push_back(i.id);
        }
    for (const ResourceRecord& r : s.resources)
        if (r.hasLocation)
        {
            resourceQuad.insert(r.lat, r.lng, r.id);
            resourcePoints.push_back({r.lat, r.lng});
            resourceIds.push_back(r.id);
        }

    dsa::RangeTree incidentRange, resourceRange;
    incidentRange.build(incidentPoints, incidentIds);
    resourceRange.build(resourcePoints, resourceIds);

    std::size_t quadVisited = 0, rangeCanonical = 0;
    std::vector<unsigned long long> incidentHits = incidentQuad.queryRange(box, &quadVisited);
    std::vector<unsigned long long> resourceHits = resourceRange.query(box, &rangeCanonical);

    // Cross-check: the other structure must return the same sets.
    std::vector<unsigned long long> incidentCheck = incidentRange.query(box);
    std::vector<unsigned long long> resourceCheck = resourceQuad.queryRange(box);
    auto sortIds = [](std::vector<unsigned long long>& v)
    { dsa::radixSort(v.data(), v.size(), [](unsigned long long x) { return x; }); };
    sortIds(incidentHits); sortIds(incidentCheck); sortIds(resourceHits); sortIds(resourceCheck);

    dsa::ChainedHashTable<unsigned long long, std::size_t> incidentTable, resourceTable;
    indexIncidents(s, incidentTable);
    indexResources(s, resourceTable);

    Json::Value incidents(Json::arrayValue), resources(Json::arrayValue);
    for (unsigned long long id : incidentHits)
        incidents.append(incidentSummary(s.incidents[*incidentTable.get(id)]));
    for (unsigned long long id : resourceHits)
        resources.append(resourceSummary(s.resources[*resourceTable.get(id)]));

    std::size_t quadrants[4];
    resourceQuad.quadrantCounts(quadrants);

    Json::Value data;
    data["region"]["min_lat"] = minLat;
    data["region"]["max_lat"] = maxLat;
    data["region"]["min_lng"] = minLng;
    data["region"]["max_lng"] = maxLng;
    data["incident_count"] = static_cast<Json::UInt64>(incidentHits.size());
    data["resource_count"] = static_cast<Json::UInt64>(resourceHits.size());
    data["incidents"] = incidents;
    data["resources"] = resources;
    data["quad_tree"]["incident_nodes"] = static_cast<Json::UInt64>(incidentQuad.nodeCount());
    data["quad_tree"]["incident_nodes_visited"] = static_cast<Json::UInt64>(quadVisited);
    data["quad_tree"]["resource_depth"] = resourceQuad.depth();
    data["range_tree"]["resource_canonical_nodes"] = static_cast<Json::UInt64>(rangeCanonical);
    data["cross_check_passed"] = incidentHits == incidentCheck && resourceHits == resourceCheck;
    data["structures_used"] = structuresUsed({"Quad Tree", "Range Tree", "Radix Sort", "Hash Table"});
    return data;
}

// =====================================================================
// TEXT: TRIE + SUFFIX ARRAY
// =====================================================================

Json::Value IntelligenceEngine::prefixSearch(const Snapshot& s, const std::string& prefix, std::size_t limit)
{
    if (prefix.empty())
        throw EngineError("Query parameter q is required", "MISSING_QUERY", 400);
    if (limit == 0 || limit > 100) limit = 20;

    // Entity keys: incidents as id, resources as id + 2^62 (keeps one trie).
    constexpr unsigned long long kResourceTag = 1ULL << 62;
    dsa::Trie trie;
    for (const IncidentRecord& i : s.incidents)
    {
        trie.insert(i.code, i.id);
        trie.insertPhrase(i.title, i.id);
        trie.insert(i.type, i.id);
    }
    for (const ResourceRecord& r : s.resources)
    {
        trie.insert(r.code, r.id | kResourceTag);
        trie.insertPhrase(r.name, r.id | kResourceTag);
        trie.insert(r.type, r.id | kResourceTag);
    }

    dsa::ChainedHashTable<unsigned long long, std::size_t> incidentTable, resourceTable;
    indexIncidents(s, incidentTable);
    indexResources(s, resourceTable);

    // Several keys may lead to the same entity; de-duplicate with a hash set.
    dsa::ChainedHashTable<unsigned long long, bool> seen;
    Json::Value results(Json::arrayValue);
    for (const auto& match : trie.withPrefix(prefix, 1000))
    {
        if (results.size() >= limit) break;
        if (!seen.put(match.id, true)) continue;

        Json::Value item;
        item["matched_key"] = match.word;
        if (match.id & kResourceTag)
        {
            const ResourceRecord& r = s.resources[*resourceTable.get(match.id & ~kResourceTag)];
            item["entity"] = "RESOURCE";
            item["id"] = uid(r.id);
            item["code"] = r.code;
            item["label"] = r.name;
            item["type"] = r.type;
        }
        else
        {
            const IncidentRecord& i = s.incidents[*incidentTable.get(match.id)];
            item["entity"] = "INCIDENT";
            item["id"] = uid(i.id);
            item["code"] = i.code;
            item["label"] = i.title;
            item["type"] = i.type;
        }
        results.append(item);
    }

    Json::Value data;
    data["query"] = prefix;
    data["result_count"] = results.size();
    data["results"] = results;
    data["trie"]["keys"] = static_cast<Json::UInt64>(trie.wordCount());
    data["trie"]["nodes"] = static_cast<Json::UInt64>(trie.nodeCount());
    data["structures_used"] = structuresUsed({"Trie", "Hash Table"});
    return data;
}

Json::Value IntelligenceEngine::patternSearch(const Snapshot& s, const std::string& pattern, std::size_t limit)
{
    if (pattern.size() < 2)
        throw EngineError("Query parameter q must have at least 2 characters", "QUERY_TOO_SHORT", 400);
    if (limit == 0 || limit > 200) limit = 50;

    struct Source { const char* kind; unsigned long long id; unsigned long long incidentId; };
    std::vector<std::string> docs;
    std::vector<Source> sources;
    for (const IncidentRecord& i : s.incidents)
    {
        docs.push_back(i.title + " - " + i.description);
        sources.push_back({"INCIDENT", i.id, i.id});
    }
    for (const RequestRecord& r : s.requests)
    {
        docs.push_back(r.description);
        sources.push_back({"REQUEST", r.id, r.incidentId});
    }
    for (const EventRecord& e : s.events)
    {
        docs.push_back(e.description);
        sources.push_back({"EVENT", e.id, e.incidentId});
    }

    dsa::SuffixArray sa;
    sa.build(docs);
    const auto occurrences = sa.find(pattern, 5000);

    Json::Value results(Json::arrayValue);
    // Occurrences come in suffix order; group to one row per document.
    std::vector<std::size_t> docOrder;
    std::vector<std::size_t> hitCount(docs.size(), 0), firstOffset(docs.size(), 0);
    for (const auto& o : occurrences)
    {
        if (hitCount[o.document] == 0 || o.offset < firstOffset[o.document])
            firstOffset[o.document] = o.offset;
        if (hitCount[o.document]++ == 0)
            docOrder.push_back(o.document);
    }
    dsa::mergeSort(docOrder.data(), docOrder.size(), [](std::size_t a, std::size_t b) { return a < b; });

    for (std::size_t d : docOrder)
    {
        if (results.size() >= limit) break;
        const std::string& text = docs[d];
        const std::size_t from = firstOffset[d] > 30 ? firstOffset[d] - 30 : 0;
        Json::Value item;
        item["source"] = sources[d].kind;
        item["id"] = uid(sources[d].id);
        item["incident_id"] = uid(sources[d].incidentId);
        item["occurrences"] = static_cast<Json::UInt64>(hitCount[d]);
        item["snippet"] = text.substr(from, pattern.size() + 60);
        results.append(item);
    }

    Json::Value data;
    data["query"] = pattern;
    data["total_occurrences"] = static_cast<Json::UInt64>(occurrences.size());
    data["matching_documents"] = static_cast<Json::UInt64>(docOrder.size());
    data["results"] = results;
    data["suffix_array"]["text_length"] = static_cast<Json::UInt64>(sa.textLength());
    data["suffix_array"]["documents"] = static_cast<Json::UInt64>(docs.size());
    data["structures_used"] = structuresUsed({"Suffix Array", "LCP (Kasai)", "Merge Sort"});
    return data;
}

// =====================================================================
// RESOURCE SEARCH & RANKING WORKFLOW
// =====================================================================

Json::Value IntelligenceEngine::rankResourcesForRequest(const Snapshot& s, unsigned long long requestId,
                                                        const std::string& algorithmParam)
{
    const std::string algorithm = algorithmParam.empty() ? "MERGE" : normalizeType(algorithmParam);
    if (algorithm != "MERGE" && algorithm != "QUICK")
        throw EngineError("algorithm must be merge or quick", "INVALID_ALGORITHM", 400);

    // 1. Hash-table lookup of the request (chaining) ...
    dsa::ChainedHashTable<unsigned long long, std::size_t> requestTable;
    for (std::size_t i = 0; i < s.requests.size(); ++i)
        requestTable.put(s.requests[i].id, i);
    const std::size_t* reqIndex = requestTable.get(requestId);
    if (!reqIndex)
        throw EngineError("Emergency request not found", "REQUEST_NOT_FOUND", 404);
    const RequestRecord& request = s.requests[*reqIndex];
    const IncidentRecord& incident = requireIncident(s, request.incidentId);

    const auto compatible = compatibleResourceType(request.type);
    if (!compatible)
        throw EngineError("Request type has no compatible resource type", "UNSUPPORTED_REQUEST_TYPE", 422);

    const unsigned int outstanding = request.quantityRequired > request.allocatedUnits
                                         ? request.quantityRequired - request.allocatedUnits : 0;

    // 2. Linear search / filter over the resource registry (dynamic array).
    struct Candidate
    {
        std::size_t index = 0;
        double distanceKm = -1.0; // -1 = unknown location
        unsigned int available = 0;
        std::size_t band = 0;
    };
    dsa::DynamicArray<Candidate> candidates;
    for (std::size_t i = 0; i < s.resources.size(); ++i)
    {
        const ResourceRecord& r = s.resources[i];
        if (!isCompatibleResource(request.type, r.type)) continue;
        if (r.status != "AVAILABLE" || r.availableUnits == 0) continue;
        Candidate c;
        c.index = i;
        c.available = r.availableUnits;
        if (incident.hasLocation && r.hasLocation)
            c.distanceKm = dsa::haversineKm(incident.lat, incident.lng, r.lat, r.lng);
        candidates.pushBack(c);
    }

    // 3. Bucket sort into 5 distance bands (unknown distances go last).
    std::vector<double> distances;
    for (const Candidate& c : candidates)
        if (c.distanceKm >= 0) distances.push_back(c.distanceKm);
    constexpr std::size_t kBands = 5;
    double minD = 0, maxD = 0;
    if (!distances.empty())
    {
        minD = maxD = distances[0];
        for (double d : distances) { if (d < minD) minD = d; if (d > maxD) maxD = d; }
    }
    for (Candidate& c : candidates)
        c.band = c.distanceKm < 0 ? kBands : dsa::bucketOf(c.distanceKm, minD, maxD, kBands);
    dsa::bucketSort(candidates.begin(), candidates.size(), kBands + 1,
                    [](const Candidate& c) { return static_cast<double>(c.band); });

    // 4. Final ranking: band asc, then distance asc, then available units desc.
    auto better = [](const Candidate& a, const Candidate& b)
    {
        if (a.band != b.band) return a.band < b.band;
        if (a.distanceKm != b.distanceKm) return a.distanceKm < b.distanceKm;
        return a.available > b.available;
    };
    if (algorithm == "QUICK")
        dsa::quickSort(candidates.begin(), candidates.size(), better);
    else
        dsa::mergeSort(candidates.begin(), candidates.size(), better);

    // 5. Median distance (quick-select).
    const double medianKm = dsa::median(distances.data(), distances.size());

    Json::Value ranked(Json::arrayValue);
    unsigned int cumulative = 0;
    for (std::size_t rank = 0; rank < candidates.size(); ++rank)
    {
        const Candidate& c = candidates[rank];
        Json::Value item = resourceSummary(s.resources[c.index]);
        item["rank"] = static_cast<Json::UInt64>(rank + 1);
        item["distance_km"] = c.distanceKm < 0 ? Json::Value(Json::nullValue) : Json::Value(round3(c.distanceKm));
        item["distance_band"] = c.band == kBands ? Json::Value("UNKNOWN") : Json::Value(static_cast<Json::UInt64>(c.band + 1));
        const unsigned int take = cumulative >= outstanding ? 0
                                 : (outstanding - cumulative < c.available ? outstanding - cumulative : c.available);
        item["suggested_units"] = take;
        cumulative += take;
        ranked.append(item);
    }

    Json::Value data;
    data["request_id"] = uid(request.id);
    data["incident"] = incidentSummary(incident);
    data["request_type"] = request.type;
    data["required_resource_type"] = *compatible;
    data["outstanding_units"] = outstanding;
    data["candidate_count"] = static_cast<Json::UInt64>(candidates.size());
    data["can_fully_satisfy"] = cumulative >= outstanding;
    data["median_distance_km"] = distances.empty() ? Json::Value(Json::nullValue) : Json::Value(round3(medianKm));
    data["sort_algorithm"] = algorithm == "QUICK" ? "quick" : "merge";
    data["ranked_resources"] = ranked;
    data["structures_used"] = structuresUsed({"Hash Table", "Array", "Linear Search", "Bucket Sort",
                                              algorithm == "QUICK" ? "Quick Sort" : "Merge Sort", "Median Search"});
    return data;
}

// =====================================================================
// INCIDENT INDEXING: SORTS, TREES, HASHING, SEARCHING
// =====================================================================

Json::Value IntelligenceEngine::incidentIndex(const Snapshot& s, unsigned long long fromId, unsigned long long toId,
                                              const std::string& code)
{
    const std::size_t n = s.incidents.size();
    std::vector<std::size_t> order(n);
    for (std::size_t i = 0; i < n; ++i) order[i] = i;

    // Radix sort by ID, then stable counting sort by severity rank (desc) ->
    // severity groups with ascending IDs inside each group.
    dsa::radixSort(order.data(), n, [&](std::size_t i) { return s.incidents[i].id; });
    dsa::countingSort(order.data(), n, 4, [&](std::size_t i)
                      { return static_cast<std::size_t>(4 - RequestPrioritizer::severityRank(s.incidents[i].severity)); });

    Json::Value bySeverity(Json::arrayValue);
    for (std::size_t i : order)
        bySeverity.append(incidentSummary(s.incidents[i]));

    // Three tree indexes on the same keys, in arrival (database) order.
    dsa::BinarySearchTree<unsigned long long, std::size_t> bst;
    dsa::AVLTree<unsigned long long, std::size_t> avl;
    dsa::RedBlackTree<unsigned long long, std::size_t> rb;
    for (std::size_t i = 0; i < n; ++i)
    {
        bst.insert(s.incidents[i].id, i);
        avl.insert(s.incidents[i].id, i);
        rb.insert(s.incidents[i].id, i);
    }

    if (toId == 0) toId = ~0ULL;
    if (fromId > toId)
        throw EngineError("from_id must not exceed to_id", "INVALID_RANGE", 400);
    Json::Value inRange(Json::arrayValue);
    avl.rangeQuery(fromId, toId, [&](unsigned long long, std::size_t index)
                   { inRange.append(incidentSummary(s.incidents[index])); });

    Json::Value data;
    data["ordered_by_severity"] = bySeverity;
    data["id_range"]["from_id"] = uid(fromId);
    data["id_range"]["to_id"] = toId == ~0ULL ? Json::Value(Json::nullValue) : uid(toId);
    data["id_range"]["incidents"] = inRange;
    data["tree_comparison"]["keys"] = static_cast<Json::UInt64>(n);
    data["tree_comparison"]["bst_height"] = static_cast<Json::UInt64>(bst.height());
    data["tree_comparison"]["avl_height"] = avl.height();
    data["tree_comparison"]["avl_rotations"] = static_cast<Json::UInt64>(avl.rotations());
    data["tree_comparison"]["red_black_height"] = rb.height();
    data["tree_comparison"]["avl_valid"] = avl.isValid();
    data["tree_comparison"]["red_black_valid"] = rb.isValid();

    if (!code.empty())
    {
        const std::string key = normalizeType(code);
        dsa::ChainedHashTable<std::string, std::size_t> chained;
        dsa::LinearProbingHashTable<std::string, std::size_t> probing;
        std::vector<std::size_t> byCode(n);
        for (std::size_t i = 0; i < n; ++i)
        {
            chained.put(normalizeType(s.incidents[i].code), i);
            probing.put(normalizeType(s.incidents[i].code), i);
            byCode[i] = i;
        }
        auto codeOf = [&](std::size_t i) { return normalizeType(s.incidents[i].code); };
        dsa::mergeSort(byCode.data(), n, [&](std::size_t a, std::size_t b) { return codeOf(a) < codeOf(b); });

        const std::size_t* viaChained = chained.get(key);
        const std::size_t* viaProbing = probing.get(key);
        const std::ptrdiff_t viaBinary = dsa::binarySearch(byCode.data(), n, key, codeOf);
        const std::ptrdiff_t viaLinear = dsa::linearSearch(s.incidents.data(), n,
                                                           [&](const IncidentRecord& r) { return normalizeType(r.code) == key; });

        const long long a = viaChained ? static_cast<long long>(*viaChained) : -1;
        const long long b = viaProbing ? static_cast<long long>(*viaProbing) : -1;
        const long long c = viaBinary >= 0 ? static_cast<long long>(byCode[static_cast<std::size_t>(viaBinary)]) : -1;
        const long long d = static_cast<long long>(viaLinear);

        Json::Value lookup;
        lookup["code"] = key;
        lookup["found"] = a >= 0;
        lookup["incident"] = a >= 0 ? incidentSummary(s.incidents[static_cast<std::size_t>(a)]) : Json::Value(Json::nullValue);
        lookup["methods_agree"] = a == b && b == c && c == d;
        lookup["hash_chaining_collisions"] = static_cast<Json::UInt64>(chained.collisions());
        lookup["hash_probing_probes"] = static_cast<Json::UInt64>(probing.probes());
        data["code_lookup"] = lookup;
    }

    data["structures_used"] = structuresUsed({"Radix Sort", "Counting Sort", "BST", "AVL Tree", "Red-Black Tree",
                                              "Hash Table (chaining)", "Hash Table (linear probing)", "Binary Search", "Linear Search"});
    return data;
}

// =====================================================================
// LINKED LIST TIMELINE / STACK HISTORY / QUEUE
// =====================================================================

namespace
{
struct TimelineItem
{
    long long time = 0;
    std::string kind, type, description;
    unsigned long long refId = 0;
};
} // namespace

Json::Value IntelligenceEngine::incidentTimeline(const Snapshot& s, unsigned long long incidentId)
{
    const IncidentRecord& incident = requireIncident(s, incidentId);

    dsa::LinkedList<TimelineItem> timeline;
    auto byTime = [](const TimelineItem& a, const TimelineItem& b) { return a.time < b.time; };

    timeline.insertOrdered({incident.reportedAt, "INCIDENT", "REPORTED", incident.title, incident.id}, byTime);
    for (const EventRecord& e : s.events)
        if (e.incidentId == incidentId)
            timeline.insertOrdered({e.time, "EVENT", e.type, e.description, e.id}, byTime);
    for (const RequestRecord& r : s.requests)
        if (r.incidentId == incidentId)
            timeline.insertOrdered({r.requestedAt, "REQUEST", r.type, r.description, r.id}, byTime);
    for (const AllocationRecord& a : s.allocations)
        if (a.incidentId == incidentId)
        {
            timeline.insertOrdered({a.assignedAt, "ALLOCATION", "ASSIGNED",
                                    std::to_string(a.units) + " unit(s) of resource " + std::to_string(a.resourceId), a.id}, byTime);
            if (a.releasedAt > 0)
                timeline.insertOrdered({a.releasedAt, "ALLOCATION", a.status,
                                        "Allocation released (" + a.status + ")", a.id}, byTime);
        }

    Json::Value entries(Json::arrayValue);
    timeline.forEach([&](const TimelineItem& t)
    {
        Json::Value item;
        item["time"] = i64(t.time);
        item["kind"] = t.kind;
        item["type"] = t.type;
        item["description"] = t.description;
        item["reference_id"] = uid(t.refId);
        entries.append(item);
    });

    Json::Value data;
    data["incident"] = incidentSummary(incident);
    data["entry_count"] = static_cast<Json::UInt64>(timeline.size());
    data["first_time"] = timeline.head() ? i64(timeline.head()->value.time) : Json::Value(Json::nullValue);
    data["last_time"] = timeline.tail() ? i64(timeline.tail()->value.time) : Json::Value(Json::nullValue);
    data["timeline"] = entries;
    data["structures_used"] = structuresUsed({"Hash Table", "Doubly Linked List (ordered insert)"});
    return data;
}

Json::Value IntelligenceEngine::operationsHistory(const Snapshot& s, std::size_t limit)
{
    if (limit == 0 || limit > 200) limit = 20;

    std::vector<EventRecord> ordered = s.events;
    dsa::mergeSort(ordered.data(), ordered.size(), [](const EventRecord& a, const EventRecord& b)
                   { return a.time < b.time || (a.time == b.time && a.id < b.id); });

    dsa::Stack<EventRecord> history;
    for (const EventRecord& e : ordered)
        history.push(e);

    Json::Value recent(Json::arrayValue);
    const std::size_t total = history.size();
    EventRecord e;
    while (recent.size() < limit && history.pop(e))
    {
        Json::Value item;
        item["event_id"] = uid(e.id);
        item["incident_id"] = uid(e.incidentId);
        item["event_type"] = e.type;
        item["description"] = e.description;
        item["time"] = i64(e.time);
        if (e.type == "RESOURCE_ALLOCATED")
            item["reverse_action"] = "POST /api/allocations/{id}/release";
        else if (e.type == "RESOURCE_RELEASED")
            item["reverse_action"] = "POST /api/allocations (re-allocate)";
        else
            item["reverse_action"] = Json::Value(Json::nullValue);
        recent.append(item);
    }

    Json::Value data;
    data["stack_size"] = static_cast<Json::UInt64>(total);
    data["returned"] = recent.size();
    data["most_recent_first"] = recent;
    data["structures_used"] = structuresUsed({"Merge Sort", "Stack (LIFO)"});
    return data;
}

Json::Value IntelligenceEngine::requestQueue(const Snapshot& s)
{
    dsa::ChainedHashTable<unsigned long long, std::size_t> incidentTable;
    indexIncidents(s, incidentTable);

    std::vector<RequestRecord> open;
    for (const RequestRecord& r : s.requests)
        if (r.status == "PENDING" || r.status == "PROCESSING")
            open.push_back(r);
    dsa::mergeSort(open.data(), open.size(), [](const RequestRecord& a, const RequestRecord& b)
                   { return a.requestedAt < b.requestedAt || (a.requestedAt == b.requestedAt && a.id < b.id); });

    dsa::Queue<RequestRecord> fifo;
    std::vector<PrioritizedRequest> forHeap;
    unsigned long long order = 0;
    for (const RequestRecord& r : open)
    {
        fifo.enqueue(r);
        PrioritizedRequest p;
        p.requestId = r.id;
        p.incidentId = r.incidentId;
        p.requestType = r.type;
        p.requestPriority = r.priority;
        const std::size_t* inc = incidentTable.get(r.incidentId);
        p.incidentSeverity = inc ? s.incidents[*inc].severity : "";
        p.severityRank = RequestPrioritizer::severityRank(p.incidentSeverity);
        p.priorityRank = RequestPrioritizer::priorityRank(r.priority);
        p.quantityRequired = r.quantityRequired;
        p.allocatedUnits = r.allocatedUnits;
        p.outstandingUnits = r.quantityRequired > r.allocatedUnits ? r.quantityRequired - r.allocatedUnits : 0;
        p.insertionOrder = order++;
        forHeap.push_back(p);
    }

    Json::Value fifoOrder(Json::arrayValue);
    RequestRecord r;
    while (fifo.dequeue(r))
    {
        Json::Value item;
        item["request_id"] = uid(r.id);
        item["incident_id"] = uid(r.incidentId);
        item["request_type"] = r.type;
        item["priority"] = r.priority;
        item["requested_at"] = i64(r.requestedAt);
        fifoOrder.append(item);
    }

    Json::Value priorityOrder(Json::arrayValue);
    for (const PrioritizedRequest& p : RequestPrioritizer::rank(forHeap))
    {
        Json::Value item;
        item["request_id"] = uid(p.requestId);
        item["incident_id"] = uid(p.incidentId);
        item["incident_severity"] = p.incidentSeverity;
        item["priority"] = p.requestPriority;
        item["outstanding_units"] = p.outstandingUnits;
        priorityOrder.append(item);
    }

    Json::Value data;
    data["open_request_count"] = static_cast<Json::UInt64>(open.size());
    data["fifo_order"] = fifoOrder;
    data["priority_order"] = priorityOrder;
    data["structures_used"] = structuresUsed({"Merge Sort", "Queue (FIFO)", "Priority Queue (binary heap)", "Hash Table"});
    return data;
}

// =====================================================================
// MULTI-LINKED LIST + SPARSE MATRIX
// =====================================================================

Json::Value IntelligenceEngine::deploymentMatrix(const Snapshot& s, bool includeClosed)
{
    dsa::MultiLinkedList<const AllocationRecord*> relations;
    dsa::SparseMatrix matrix;
    for (const AllocationRecord& a : s.allocations)
    {
        if (!includeClosed && !a.active()) continue;
        relations.link(a.incidentId, a.resourceId, &a);
        matrix.add(a.incidentId, a.resourceId, static_cast<long long>(a.units));
    }

    Json::Value byIncident(Json::arrayValue);
    relations.forEachRowKey([&](unsigned long long incidentId, std::size_t degree)
    {
        Json::Value row;
        row["incident_id"] = uid(incidentId);
        row["resource_links"] = static_cast<Json::UInt64>(degree);
        row["total_units"] = i64(matrix.rowSum(incidentId));
        Json::Value list(Json::arrayValue);
        relations.forEachInRow(incidentId, [&](const auto& cell)
        {
            Json::Value link;
            link["allocation_id"] = uid(cell.payload->id);
            link["resource_id"] = uid(cell.colKey);
            link["units"] = cell.payload->units;
            link["status"] = cell.payload->status;
            list.append(link);
        });
        row["allocations"] = list;
        byIncident.append(row);
    });

    Json::Value byResource(Json::arrayValue);
    relations.forEachColumnKey([&](unsigned long long resourceId, std::size_t degree)
    {
        Json::Value col;
        col["resource_id"] = uid(resourceId);
        col["incident_links"] = static_cast<Json::UInt64>(degree);
        col["total_units"] = i64(matrix.columnSum(resourceId));
        Json::Value incidents(Json::arrayValue);
        relations.forEachInColumn(resourceId, [&](const auto& cell) { incidents.append(uid(cell.rowKey)); });
        col["incident_ids"] = incidents;
        byResource.append(col);
    });

    Json::Value cells(Json::arrayValue);
    for (const auto& e : matrix.entries())
    {
        Json::Value cell;
        cell["incident_id"] = uid(e.row);
        cell["resource_id"] = uid(e.col);
        cell["units"] = i64(e.value);
        cells.append(cell);
    }

    const double dense = static_cast<double>(s.incidents.size()) * static_cast<double>(s.resources.size());
    Json::Value data;
    data["include_closed"] = includeClosed;
    data["relationship_count"] = static_cast<Json::UInt64>(relations.cellCount());
    data["by_incident"] = byIncident;
    data["by_resource"] = byResource;
    data["sparse_matrix"]["non_zero"] = static_cast<Json::UInt64>(matrix.nonZeroCount());
    data["sparse_matrix"]["dense_cells"] = dense;
    data["sparse_matrix"]["density"] = dense > 0 ? round3(static_cast<double>(matrix.nonZeroCount()) / dense) : 0.0;
    data["sparse_matrix"]["entries"] = cells;
    data["structures_used"] = structuresUsed({"Multi-Linked List", "Sparse Matrix"});
    return data;
}

// =====================================================================
// INTERVAL TREE + SEGMENT TREE AVAILABILITY
// =====================================================================

Json::Value IntelligenceEngine::availability(const Snapshot& s, long long from, long long to,
                                             unsigned long long resourceId, long long bucketSeconds)
{
    if (to <= 0) to = s.now;
    if (from <= 0) from = to - 7LL * 24 * 3600;
    if (from >= to)
        throw EngineError("from must be earlier than to", "INVALID_WINDOW", 400);
    if (bucketSeconds <= 0) bucketSeconds = 3600;
    // Cap the number of buckets so the segment tree stays small.
    if ((to - from) / bucketSeconds > 2000)
        bucketSeconds = (to - from) / 2000 + 1;

    std::vector<dsa::IntervalTree::Interval> intervals;
    dsa::ChainedHashTable<unsigned long long, std::size_t> allocTable;
    std::vector<double> completedMinutes;
    for (std::size_t i = 0; i < s.allocations.size(); ++i)
    {
        const AllocationRecord& a = s.allocations[i];
        if (resourceId != 0 && a.resourceId != resourceId) continue;
        if (a.status == "CANCELLED") continue;
        const long long end = a.releasedAt > 0 ? a.releasedAt : s.now;
        intervals.push_back({a.assignedAt, end < a.assignedAt ? a.assignedAt : end, a.id});
        allocTable.put(a.id, i);
        if (a.releasedAt > 0)
            completedMinutes.push_back(static_cast<double>(a.releasedAt - a.assignedAt) / 60.0);
    }

    dsa::IntervalTree tree;
    tree.build(intervals);
    std::size_t visited = 0;
    const auto overlapping = tree.overlapping(from, to, &visited);

    // Units active in each time bucket -> segment tree.
    const std::size_t bucketCount = static_cast<std::size_t>((to - from + bucketSeconds - 1) / bucketSeconds);
    std::vector<long long> load(bucketCount, 0);
    for (std::size_t b = 0; b < bucketCount; ++b)
    {
        const long long bStart = from + static_cast<long long>(b) * bucketSeconds;
        const long long bEnd = bStart + bucketSeconds - 1;
        for (const auto& iv : tree.overlapping(bStart, bEnd))
            load[b] += s.allocations[*allocTable.get(iv.id)].units;
    }
    dsa::SegmentTree seg(load);

    std::size_t peakBucket = 0;
    const long long peak = seg.rangeMax(0, bucketCount - 1);
    for (std::size_t b = 0; b < bucketCount; ++b)
        if (load[b] == peak) { peakBucket = b; break; }

    Json::Value active(Json::arrayValue);
    for (const auto& iv : overlapping)
    {
        const AllocationRecord& a = s.allocations[*allocTable.get(iv.id)];
        Json::Value item;
        item["allocation_id"] = uid(a.id);
        item["resource_id"] = uid(a.resourceId);
        item["incident_id"] = uid(a.incidentId);
        item["units"] = a.units;
        item["status"] = a.status;
        item["start"] = i64(iv.start);
        item["end"] = a.releasedAt > 0 ? i64(a.releasedAt) : Json::Value(Json::nullValue);
        active.append(item);
    }

    Json::Value series(Json::arrayValue);
    for (std::size_t b = 0; b < bucketCount; ++b)
    {
        Json::Value point;
        point["bucket_start"] = i64(from + static_cast<long long>(b) * bucketSeconds);
        point["active_units"] = i64(load[b]);
        series.append(point);
    }

    Json::Value data;
    data["window"]["from"] = i64(from);
    data["window"]["to"] = i64(to);
    data["window"]["bucket_seconds"] = i64(bucketSeconds);
    data["resource_id"] = resourceId == 0 ? Json::Value(Json::nullValue) : uid(resourceId);
    data["allocations_in_window"] = active;
    data["interval_tree"]["intervals"] = static_cast<Json::UInt64>(tree.size());
    data["interval_tree"]["nodes_visited"] = static_cast<Json::UInt64>(visited);
    data["segment_tree"]["buckets"] = static_cast<Json::UInt64>(bucketCount);
    data["segment_tree"]["peak_active_units"] = i64(peak);
    data["segment_tree"]["peak_bucket_start"] = i64(from + static_cast<long long>(peakBucket) * bucketSeconds);
    data["segment_tree"]["unit_hours"] = round3(static_cast<double>(seg.rangeSum(0, bucketCount - 1)) *
                                                static_cast<double>(bucketSeconds) / 3600.0);
    data["median_completed_allocation_minutes"] = completedMinutes.empty()
        ? Json::Value(Json::nullValue)
        : Json::Value(round3(dsa::median(completedMinutes.data(), completedMinutes.size())));
    data["load_series"] = series;
    data["structures_used"] = structuresUsed({"Interval Tree", "Segment Tree", "Hash Table", "Median Search"});
    return data;
}

// =====================================================================
// MST
// =====================================================================

Json::Value IntelligenceEngine::networkMst(const Snapshot& s)
{
    dsa::ChainedHashTable<unsigned long long, std::size_t> nodeIndex;
    for (std::size_t i = 0; i < s.nodes.size(); ++i)
        nodeIndex.put(s.nodes[i].id, i);

    std::vector<dsa::WeightedEdge> edges;
    dsa::AdjacencyMatrix matrix(s.nodes.size());
    std::size_t blocked = 0;
    for (const EdgeRecord& e : s.edges)
    {
        if (e.blocked) { ++blocked; continue; }
        const std::size_t* u = nodeIndex.get(e.source);
        const std::size_t* v = nodeIndex.get(e.destination);
        if (!u || !v || *u == *v) continue;
        edges.push_back({*u, *v, e.distanceKm, e.id});
        matrix.setUndirected(*u, *v, e.distanceKm);
    }

    const dsa::SpanningForest kruskal = dsa::kruskalMST(s.nodes.size(), edges);
    const dsa::SpanningForest prim = dsa::primMST(matrix);

    Json::Value tree(Json::arrayValue);
    for (const dsa::WeightedEdge& e : kruskal.edges)
    {
        Json::Value item;
        item["edge_id"] = uid(e.id);
        item["from_node_id"] = uid(s.nodes[e.u].id);
        item["from_node_code"] = s.nodes[e.u].code;
        item["to_node_id"] = uid(s.nodes[e.v].id);
        item["to_node_code"] = s.nodes[e.v].code;
        item["distance_km"] = round3(e.weight);
        tree.append(item);
    }

    Json::Value data;
    data["node_count"] = static_cast<Json::UInt64>(s.nodes.size());
    data["usable_edge_count"] = static_cast<Json::UInt64>(edges.size());
    data["blocked_edges_ignored"] = static_cast<Json::UInt64>(blocked);
    data["components"] = static_cast<Json::UInt64>(kruskal.components);
    data["is_connected"] = kruskal.components <= 1;
    data["total_distance_km"] = round3(kruskal.totalWeight);
    data["mst_edges"] = tree;
    data["prim_total_distance_km"] = round3(prim.totalWeight);
    data["kruskal_prim_agree"] = std::fabs(kruskal.totalWeight - prim.totalWeight) < 1e-6 &&
                                 kruskal.components == prim.components;
    data["structures_used"] = structuresUsed({"Hash Table", "Adjacency Matrix", "Disjoint Set", "Merge Sort",
                                              "Kruskal MST", "Prim MST"});
    return data;
}

// =====================================================================
// K-ARY CLASSIFICATION
// =====================================================================

namespace
{
Json::Value classificationJson(const dsa::KaryTree::Node* node)
{
    Json::Value item;
    item["label"] = node->label;
    item["incident_count"] = static_cast<Json::UInt64>(dsa::KaryTree::totalMembers(node));
    Json::Value ids(Json::arrayValue);
    for (unsigned long long id : node->members) ids.append(uid(id));
    item["incident_ids"] = ids;
    Json::Value children(Json::arrayValue);
    for (const dsa::KaryTree::Node* c = node->firstChild; c; c = c->nextSibling)
        children.append(classificationJson(c));
    item["children"] = children;
    return item;
}
} // namespace

Json::Value IntelligenceEngine::classification(const Snapshot& s)
{
    dsa::KaryTree tree("ALL_DISASTERS");
    struct Group { const char* parent; const char* category; std::vector<const char*> leaves; };
    const std::vector<Group> taxonomy = {
        {"NATURAL", "HYDROLOGICAL", {"FLOOD", "FLASH_FLOOD", "TSUNAMI"}},
        {"NATURAL", "GEOLOGICAL", {"EARTHQUAKE", "LANDSLIDE", "VOLCANO"}},
        {"NATURAL", "METEOROLOGICAL", {"CYCLONE", "STORM", "HEATWAVE", "COLD_WAVE"}},
        {"NATURAL", "CLIMATOLOGICAL", {"DROUGHT", "WILDFIRE"}},
        {"NATURAL", "BIOLOGICAL", {"EPIDEMIC", "PANDEMIC"}},
        {"HUMAN_MADE", "FIRE_AND_EXPLOSION", {"FIRE", "EXPLOSION", "GAS_LEAK"}},
        {"HUMAN_MADE", "TRANSPORT", {"ROAD_ACCIDENT", "RAIL_ACCIDENT", "AIR_ACCIDENT"}},
        {"HUMAN_MADE", "INFRASTRUCTURE", {"BUILDING_COLLAPSE", "POWER_OUTAGE"}},
        {"HUMAN_MADE", "HAZMAT", {"CHEMICAL_SPILL", "INDUSTRIAL_ACCIDENT"}},
    };
    for (const Group& g : taxonomy)
    {
        dsa::KaryTree::Node* parent = tree.find(g.parent);
        if (!parent) parent = tree.addChild(tree.root(), g.parent);
        dsa::KaryTree::Node* category = tree.addChild(parent, g.category);
        for (const char* leaf : g.leaves)
            tree.addChild(category, leaf);
    }
    dsa::KaryTree::Node* unclassified = tree.addChild(tree.root(), "UNCLASSIFIED");

    Json::Value placements(Json::arrayValue);
    for (const IncidentRecord& i : s.incidents)
    {
        const std::string type = normalizeType(i.type);
        dsa::KaryTree::Node* leaf = type.empty() ? nullptr : tree.find(type);
        if (!leaf || leaf->firstChild != nullptr)
        {
            leaf = nullptr;
            for (dsa::KaryTree::Node* c = unclassified->firstChild; c; c = c->nextSibling)
                if (c->label == (type.empty() ? "UNKNOWN" : type)) leaf = c;
            if (!leaf) leaf = tree.addChild(unclassified, type.empty() ? "UNKNOWN" : type);
        }
        leaf->members.push_back(i.id);

        Json::Value p;
        p["incident_id"] = uid(i.id);
        p["incident_code"] = i.code;
        Json::Value path(Json::arrayValue);
        for (const std::string& label : dsa::KaryTree::pathTo(leaf)) path.append(label);
        p["path"] = path;
        placements.append(p);
    }

    Json::Value data;
    data["tree_nodes"] = static_cast<Json::UInt64>(tree.size());
    data["placements"] = placements;
    data["tree"] = classificationJson(tree.root());
    data["structures_used"] = structuresUsed({"K-ary Tree (first-child / next-sibling)"});
    return data;
}

} // namespace nexa::intel
