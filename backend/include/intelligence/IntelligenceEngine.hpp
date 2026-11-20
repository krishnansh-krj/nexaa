#pragma once

// NEXA Intelligence Engine
// Pure C++ layer (no Drogon, no database) that builds the synopsis data
// structures from an in-memory snapshot of nexa_db and answers operational
// queries. DsaIntelligenceService loads the snapshot; this engine does the
// work; the result is plain JSON ready for the frontend.

#include <json/json.h>

#include <stdexcept>
#include <string>
#include <vector>

namespace nexa::intel
{

struct IncidentRecord
{
    unsigned long long id = 0;
    std::string code, title, description, type, severity, status;
    bool hasLocation = false;
    double lat = 0.0, lng = 0.0;
    long long reportedAt = 0; // epoch seconds
};

struct ResourceRecord
{
    unsigned long long id = 0;
    std::string code, name, type, status, address;
    unsigned int totalUnits = 0, availableUnits = 0;
    bool hasLocation = false;
    double lat = 0.0, lng = 0.0;
};

struct RequestRecord
{
    unsigned long long id = 0, incidentId = 0;
    std::string type, description, priority, status;
    unsigned int quantityRequired = 0, allocatedUnits = 0; // active allocated units
    long long requestedAt = 0;
};

struct AllocationRecord
{
    unsigned long long id = 0, incidentId = 0, resourceId = 0, requestId = 0; // requestId 0 = none
    unsigned int units = 0;
    std::string status;
    long long assignedAt = 0, releasedAt = 0; // releasedAt 0 = still open
    bool active() const { return status == "ASSIGNED" || status == "DISPATCHED" || status == "ON_SITE"; }
};

struct EventRecord
{
    unsigned long long id = 0, incidentId = 0;
    std::string type, description;
    long long time = 0;
};

struct NodeRecord
{
    unsigned long long id = 0;
    std::string code, name, type;
    double lat = 0.0, lng = 0.0;
};

struct EdgeRecord
{
    unsigned long long id = 0, source = 0, destination = 0;
    double distanceKm = 0.0;
    bool blocked = false;
};

struct Snapshot
{
    long long now = 0; // database clock, epoch seconds
    std::vector<IncidentRecord> incidents;
    std::vector<ResourceRecord> resources;
    std::vector<RequestRecord> requests;
    std::vector<AllocationRecord> allocations;
    std::vector<EventRecord> events;
    std::vector<NodeRecord> nodes;
    std::vector<EdgeRecord> edges;
};

// Thrown for invalid input / missing records; routes map it to an HTTP error.
class EngineError : public std::runtime_error
{
public:
    EngineError(const std::string& message, const std::string& code, int httpStatus)
        : std::runtime_error(message), code_(code), status_(httpStatus) {}
    const std::string& code() const { return code_; }
    int httpStatus() const { return status_; }

private:
    std::string code_;
    int status_;
};

class IntelligenceEngine
{
public:
    // Static registry of all synopsis structures and where each is used.
    static Json::Value catalogue();

    // KD Tree (+ hash-table incident lookup). type may be empty for any type.
    static Json::Value nearestResources(const Snapshot& s, double lat, double lng, std::size_t k,
                                        const std::string& type, bool onlyAvailable);
    static Json::Value nearestResourcesToIncident(const Snapshot& s, unsigned long long incidentId,
                                                  std::size_t k, const std::string& type, bool onlyAvailable);

    // Quad Tree + Range Tree over a lat/lng rectangle.
    static Json::Value regionQuery(const Snapshot& s, double minLat, double maxLat, double minLng, double maxLng);

    // Trie prefix search over codes, titles and names.
    static Json::Value prefixSearch(const Snapshot& s, const std::string& prefix, std::size_t limit);

    // Suffix Array pattern search over incident / request / event descriptions.
    static Json::Value patternSearch(const Snapshot& s, const std::string& pattern, std::size_t limit);

    // Full matching workflow: hash lookup, linear filter, distance, bucket bands,
    // merge / quick sort ranking, median distance.
    static Json::Value rankResourcesForRequest(const Snapshot& s, unsigned long long requestId,
                                               const std::string& algorithm);

    // Counting sort (severity) + radix sort (id) + BST/AVL/RB index comparison,
    // AVL range query, and code lookup via chained / probing hash, binary and linear search.
    static Json::Value incidentIndex(const Snapshot& s, unsigned long long fromId, unsigned long long toId,
                                     const std::string& code);

    // Doubly linked timeline of one incident's events and allocations.
    static Json::Value incidentTimeline(const Snapshot& s, unsigned long long incidentId);

    // Stack of operational actions; top = most recent.
    static Json::Value operationsHistory(const Snapshot& s, std::size_t limit);

    // FIFO queue order vs priority-queue order for open requests.
    static Json::Value requestQueue(const Snapshot& s);

    // Multi-linked list + sparse matrix of incident <-> resource deployments.
    static Json::Value deploymentMatrix(const Snapshot& s, bool includeClosed);

    // Interval tree + segment tree availability analytics over a time window.
    static Json::Value availability(const Snapshot& s, long long from, long long to,
                                    unsigned long long resourceId, long long bucketSeconds);

    // Kruskal (DSU) and Prim (adjacency matrix) over unblocked road edges.
    static Json::Value networkMst(const Snapshot& s);

    // K-ary disaster classification tree with incidents filed under leaves.
    static Json::Value classification(const Snapshot& s);
};

} // namespace nexa::intel
