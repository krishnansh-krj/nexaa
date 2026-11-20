#include "controllers/DsaRoutes.hpp"

#include "intelligence/IntelligenceEngine.hpp"
#include "services/DsaIntelligenceService.hpp"
#include "utilities/ApiResponse.hpp"

#include <drogon/drogon.h>

#include <exception>
#include <functional>
#include <string>
#include <utility>

namespace nexa
{

namespace
{

using HttpCallback = std::function<void(const drogon::HttpResponsePtr&)>;
using intel::IntelligenceEngine;
using intel::Snapshot;

// ---------- query-parameter parsing ----------

struct BadParameter
{
    std::string name;
};

bool hasParam(const drogon::HttpRequestPtr& req, const std::string& name)
{
    return !req->getParameter(name).empty();
}

unsigned long long unsignedParam(const drogon::HttpRequestPtr& req, const std::string& name,
                                 unsigned long long fallback)
{
    const std::string value = req->getParameter(name);
    if (value.empty())
        return fallback;
    for (char c : value)
        if (c < '0' || c > '9')
            throw BadParameter{name};
    try
    {
        return std::stoull(value);
    }
    catch (...)
    {
        throw BadParameter{name};
    }
}

long long signedParam(const drogon::HttpRequestPtr& req, const std::string& name, long long fallback)
{
    const std::string value = req->getParameter(name);
    if (value.empty())
        return fallback;
    try
    {
        std::size_t used = 0;
        const long long parsed = std::stoll(value, &used);
        if (used != value.size())
            throw BadParameter{name};
        return parsed;
    }
    catch (const BadParameter&)
    {
        throw;
    }
    catch (...)
    {
        throw BadParameter{name};
    }
}

double doubleParam(const drogon::HttpRequestPtr& req, const std::string& name)
{
    const std::string value = req->getParameter(name);
    if (value.empty())
        throw BadParameter{name};
    try
    {
        std::size_t used = 0;
        const double parsed = std::stod(value, &used);
        if (used != value.size())
            throw BadParameter{name};
        return parsed;
    }
    catch (const BadParameter&)
    {
        throw;
    }
    catch (...)
    {
        throw BadParameter{name};
    }
}

bool boolParam(const drogon::HttpRequestPtr& req, const std::string& name, bool fallback)
{
    const std::string value = req->getParameter(name);
    if (value.empty())
        return fallback;
    if (value == "true" || value == "1" || value == "yes")
        return true;
    if (value == "false" || value == "0" || value == "no")
        return false;
    throw BadParameter{name};
}

// ---------- response helpers ----------

void badParameter(const HttpCallback& callback, const std::string& name)
{
    callback(ApiResponse::error("Invalid or missing query parameter: " + name,
                                drogon::k400BadRequest, "INVALID_PARAMETER"));
}

// Loads the snapshot, runs `compute` on it, and maps engine errors to HTTP.
template <typename Compute>
void respond(const HttpCallback& callback, Compute compute, const std::string& message)
{
    DsaIntelligenceService::loadSnapshot(
        drogon::app().getDbClient(),

        [callback, compute, message](DsaIntelligenceService::SnapshotPtr snapshot)
        {
            try
            {
                callback(ApiResponse::success(compute(*snapshot), message));
            }
            catch (const intel::EngineError& e)
            {
                callback(ApiResponse::error(e.what(),
                                            static_cast<drogon::HttpStatusCode>(e.httpStatus()),
                                            e.code()));
            }
            catch (const std::exception& e)
            {
                LOG_ERROR << "NEXA DSA engine error: " << e.what();
                callback(ApiResponse::error("Data-structure processing failed",
                                            drogon::k500InternalServerError, "DSA_ENGINE_ERROR"));
            }
        },

        [callback](const drogon::orm::DrogonDbException& e)
        {
            LOG_ERROR << "NEXA Database Error: " << e.base().what();
            callback(ApiResponse::error("A database operation failed",
                                        drogon::k500InternalServerError, "DATABASE_ERROR"));
        },

        [callback](const std::string& failure)
        {
            callback(ApiResponse::error(failure, drogon::k500InternalServerError, "SNAPSHOT_FAILED"));
        });
}

// Registers a GET route whose handler parses parameters into a compute
// function; BadParameter becomes a 400 before any database work.
template <typename Builder>
void getRoute(const std::string& path, const std::string& message, Builder build)
{
    drogon::app().registerHandler(
        path,
        [build, message](const drogon::HttpRequestPtr& req, HttpCallback&& callback)
        {
            try
            {
                respond(callback, build(req), message);
            }
            catch (const BadParameter& bad)
            {
                badParameter(callback, bad.name);
            }
        },
        {drogon::Get});
}

} // namespace

void DsaRoutes::registerRoutes()
{
    // GET /api/dsa/catalogue - every synopsis structure and where it is used.
    drogon::app().registerHandler(
        "/api/dsa/catalogue",
        [](const drogon::HttpRequestPtr&, HttpCallback&& callback)
        {
            callback(ApiResponse::success(IntelligenceEngine::catalogue(),
                                          "NEXA data-structure catalogue"));
        },
        {drogon::Get});

    // GET /api/dsa/spatial/nearest?incident_id=1  or  ?lat=..&lng=..
    //     [&k=5][&type=AMBULANCE][&only_available=true]
    getRoute("/api/dsa/spatial/nearest", "Nearest resources (KD Tree)",
             [](const drogon::HttpRequestPtr& req)
             {
                 const std::size_t k = static_cast<std::size_t>(unsignedParam(req, "k", 5));
                 const std::string type = req->getParameter("type");
                 const bool onlyAvailable = boolParam(req, "only_available", true);
                 if (hasParam(req, "incident_id"))
                 {
                     const unsigned long long incidentId = unsignedParam(req, "incident_id", 0);
                     if (incidentId == 0) throw BadParameter{"incident_id"};
                     return std::function<Json::Value(const Snapshot&)>(
                         [=](const Snapshot& s)
                         { return IntelligenceEngine::nearestResourcesToIncident(s, incidentId, k, type, onlyAvailable); });
                 }
                 const double lat = doubleParam(req, "lat");
                 const double lng = doubleParam(req, "lng");
                 return std::function<Json::Value(const Snapshot&)>(
                     [=](const Snapshot& s)
                     { return IntelligenceEngine::nearestResources(s, lat, lng, k, type, onlyAvailable); });
             });

    // GET /api/dsa/spatial/region?min_lat=&max_lat=&min_lng=&max_lng=
    getRoute("/api/dsa/spatial/region", "Regional query (Quad Tree + Range Tree)",
             [](const drogon::HttpRequestPtr& req)
             {
                 const double minLat = doubleParam(req, "min_lat");
                 const double maxLat = doubleParam(req, "max_lat");
                 const double minLng = doubleParam(req, "min_lng");
                 const double maxLng = doubleParam(req, "max_lng");
                 return [=](const Snapshot& s)
                 { return IntelligenceEngine::regionQuery(s, minLat, maxLat, minLng, maxLng); };
             });

    // GET /api/dsa/search/prefix?q=nexa-inc[&limit=20]
    getRoute("/api/dsa/search/prefix", "Prefix search (Trie)",
             [](const drogon::HttpRequestPtr& req)
             {
                 const std::string q = req->getParameter("q");
                 const std::size_t limit = static_cast<std::size_t>(unsignedParam(req, "limit", 20));
                 return [=](const Snapshot& s) { return IntelligenceEngine::prefixSearch(s, q, limit); };
             });

    // GET /api/dsa/search/pattern?q=fire[&limit=50]
    getRoute("/api/dsa/search/pattern", "Pattern search (Suffix Array)",
             [](const drogon::HttpRequestPtr& req)
             {
                 const std::string q = req->getParameter("q");
                 const std::size_t limit = static_cast<std::size_t>(unsignedParam(req, "limit", 50));
                 return [=](const Snapshot& s) { return IntelligenceEngine::patternSearch(s, q, limit); };
             });

    // GET /api/dsa/resources/rank?request_id=4[&algorithm=merge|quick]
    getRoute("/api/dsa/resources/rank", "Ranked resources for request",
             [](const drogon::HttpRequestPtr& req)
             {
                 const unsigned long long requestId = unsignedParam(req, "request_id", 0);
                 if (requestId == 0) throw BadParameter{"request_id"};
                 const std::string algorithm = req->getParameter("algorithm");
                 return [=](const Snapshot& s)
                 { return IntelligenceEngine::rankResourcesForRequest(s, requestId, algorithm); };
             });

    // GET /api/dsa/incidents/index[?from_id=&to_id=&code=NEXA-INC-001]
    getRoute("/api/dsa/incidents/index", "Incident index (sorts, trees, hashing)",
             [](const drogon::HttpRequestPtr& req)
             {
                 const unsigned long long fromId = unsignedParam(req, "from_id", 0);
                 const unsigned long long toId = unsignedParam(req, "to_id", 0);
                 const std::string code = req->getParameter("code");
                 return [=](const Snapshot& s) { return IntelligenceEngine::incidentIndex(s, fromId, toId, code); };
             });

    // GET /api/dsa/incidents/timeline?incident_id=1
    getRoute("/api/dsa/incidents/timeline", "Incident timeline (Linked List)",
             [](const drogon::HttpRequestPtr& req)
             {
                 const unsigned long long incidentId = unsignedParam(req, "incident_id", 0);
                 if (incidentId == 0) throw BadParameter{"incident_id"};
                 return [=](const Snapshot& s) { return IntelligenceEngine::incidentTimeline(s, incidentId); };
             });

    // GET /api/dsa/incidents/classification
    getRoute("/api/dsa/incidents/classification", "Disaster classification (K-ary Tree)",
             [](const drogon::HttpRequestPtr&)
             { return [](const Snapshot& s) { return IntelligenceEngine::classification(s); }; });

    // GET /api/dsa/operations/history[?limit=20]
    getRoute("/api/dsa/operations/history", "Operational history (Stack)",
             [](const drogon::HttpRequestPtr& req)
             {
                 const std::size_t limit = static_cast<std::size_t>(unsignedParam(req, "limit", 20));
                 return [=](const Snapshot& s) { return IntelligenceEngine::operationsHistory(s, limit); };
             });

    // GET /api/dsa/requests/queue
    getRoute("/api/dsa/requests/queue", "Request queue vs priority order",
             [](const drogon::HttpRequestPtr&)
             { return [](const Snapshot& s) { return IntelligenceEngine::requestQueue(s); }; });

    // GET /api/dsa/allocations/matrix[?include_closed=false]
    getRoute("/api/dsa/allocations/matrix", "Deployment relationships (Multi-Linked List + Sparse Matrix)",
             [](const drogon::HttpRequestPtr& req)
             {
                 const bool includeClosed = boolParam(req, "include_closed", false);
                 return [=](const Snapshot& s) { return IntelligenceEngine::deploymentMatrix(s, includeClosed); };
             });

    // GET /api/dsa/analytics/availability[?from=epoch&to=epoch&resource_id=&bucket_seconds=3600]
    getRoute("/api/dsa/analytics/availability", "Availability analytics (Interval Tree + Segment Tree)",
             [](const drogon::HttpRequestPtr& req)
             {
                 const long long from = signedParam(req, "from", 0);
                 const long long to = signedParam(req, "to", 0);
                 const unsigned long long resourceId = unsignedParam(req, "resource_id", 0);
                 const long long bucket = signedParam(req, "bucket_seconds", 3600);
                 return [=](const Snapshot& s)
                 { return IntelligenceEngine::availability(s, from, to, resourceId, bucket); };
             });

    // GET /api/dsa/network/mst
    getRoute("/api/dsa/network/mst", "Minimum spanning tree (Kruskal + Prim)",
             [](const drogon::HttpRequestPtr&)
             { return [](const Snapshot& s) { return IntelligenceEngine::networkMst(s); }; });

    LOG_INFO << "NEXA DSA routes registered (/api/dsa/*).";
}

} // namespace nexa
