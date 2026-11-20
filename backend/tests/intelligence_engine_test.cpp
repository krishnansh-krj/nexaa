// NEXA - integration test for the Intelligence Engine on a snapshot that
// mirrors the current nexa_db contents (seed data + API activity).

#include "intelligence/IntelligenceEngine.hpp"

#include <cmath>
#include <iostream>
#include <string>

using namespace nexa::intel;

static int g_failures = 0, g_checks = 0;
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

static Snapshot makeSnapshot()
{
    Snapshot s;
    s.now = 1794960000; // fixed clock
    const long long t0 = s.now - 6 * 3600;

    s.incidents = {
        {1, "NEXA-INC-001", "Urban Building Fire", "Fire reported in a multi-storey residential building.",
         "FIRE", "CRITICAL", "ACTIVE", true, 28.6139, 77.2090, t0},
        {2, "NEXA-INC-002", "Urban Flooding", "Heavy rainfall causing waterlogging and evacuation requirements.",
         "FLOOD", "HIGH", "VERIFIED", true, 28.6304, 77.2177, t0 + 600},
    };
    s.resources = {
        {1, "NEXA-RES-001", "Ambulance Unit A", "AMBULANCE", "AVAILABLE", "Emergency Base A", 3, 3, true, 28.6150, 77.2100},
        {2, "NEXA-RES-002", "Fire Response Truck A", "FIRE_TRUCK", "DEPLOYED", "Fire Station A", 2, 0, true, 28.6170, 77.2110},
        {3, "NEXA-RES-003", "Medical Response Team A", "MEDICAL_TEAM", "AVAILABLE", "Medical Centre A", 5, 4, true, 28.6200, 77.2140},
        {4, "NEXA-RES-004", "Rescue Team A", "RESCUE_TEAM", "AVAILABLE", "Rescue Base A", 4, 2, true, 28.6250, 77.2160},
        {5, "NEXA-RES-005", "Ambulance Unit B", "AMBULANCE", "AVAILABLE", "Emergency Base B", 5, 5, true, 28.6400, 77.2300},
    };
    s.requests = {
        {1, 1, "FIRE_SUPPRESSION", "Immediate fire suppression support required.", "CRITICAL", "FULFILLED", 2, 2, t0 + 60},
        {2, 1, "MEDICAL_ASSISTANCE", "Medical team required for possible casualties.", "HIGH", "FULFILLED", 1, 1, t0 + 120},
        {3, 2, "RESCUE_SUPPORT", "Rescue personnel required for flood evacuation.", "HIGH", "FULFILLED", 2, 2, t0 + 700},
        {4, 1, "AMBULANCE", "Ambulance for injured residents.", "HIGH", "PENDING", 1, 0, t0 + 3600},
    };
    s.allocations = {
        {1, 1, 2, 1, 1, "DISPATCHED", t0 + 300, 0},
        {2, 1, 3, 2, 1, "ASSIGNED", t0 + 400, 0},
        {3, 2, 4, 3, 2, "ASSIGNED", t0 + 900, 0},
        {4, 1, 2, 1, 1, "ASSIGNED", t0 + 4000, 0},
        {5, 1, 5, 4, 1, "COMPLETED", t0 + 4200, t0 + 5400},
    };
    s.events = {
        {1, 1, "INCIDENT_REPORTED", "Building fire reported to the central authority.", t0 + 10},
        {2, 1, "RESOURCE_DISPATCHED", "Fire response truck dispatched to incident location.", t0 + 320},
        {3, 2, "INCIDENT_VERIFIED", "Flooding incident verified by the response authority.", t0 + 650},
        {6, 1, "RESOURCE_ALLOCATED", "Ambulance unit allocated.", t0 + 4200},
        {7, 1, "RESOURCE_RELEASED", "Ambulance unit released.", t0 + 5400},
    };
    s.nodes = {
        {1, "NODE-001", "Central Junction", "JUNCTION", 28.6139, 77.2090},
        {2, "NODE-002", "Fire Station Junction", "STATION", 28.6170, 77.2110},
        {3, "NODE-003", "Medical Centre Junction", "HOSPITAL", 28.6200, 77.2140},
        {4, "NODE-004", "Flood Response Junction", "JUNCTION", 28.6250, 77.2160},
    };
    s.edges = {
        {1, 1, 2, 1.5, false}, {2, 2, 3, 2.0, false}, {3, 3, 4, 2.5, false},
        {4, 1, 3, 3.0, false}, {5, 2, 4, 4.0, false},
    };
    return s;
}

template <typename Fn>
static int errorStatus(Fn fn)
{
    try { fn(); }
    catch (const EngineError& e) { return e.httpStatus(); }
    return 0;
}

int main()
{
    const Snapshot s = makeSnapshot();

    const Json::Value cat = IntelligenceEngine::catalogue();
    CHECK(cat["structure_count"].asInt() == 28 && cat["structures"].size() == 28);

    // KD tree: nearest available ambulances to incident 1.
    Json::Value nearest = IntelligenceEngine::nearestResourcesToIncident(s, 1, 3, "ambulance", true);
    CHECK(nearest["results"].size() == 2);
    CHECK(nearest["results"][0]["resource_id"].asUInt64() == 1);
    CHECK(nearest["results"][0]["distance_km"].asDouble() < nearest["results"][1]["distance_km"].asDouble());
    // Deployed fire truck is excluded when only_available=true, included otherwise.
    CHECK(IntelligenceEngine::nearestResourcesToIncident(s, 1, 3, "FIRE_TRUCK", true)["results"].size() == 0);
    CHECK(IntelligenceEngine::nearestResourcesToIncident(s, 1, 3, "FIRE_TRUCK", false)["results"].size() == 1);
    CHECK(errorStatus([&] { IntelligenceEngine::nearestResourcesToIncident(s, 99, 3, "", true); }) == 404);
    CHECK(errorStatus([&] { IntelligenceEngine::nearestResources(s, 120.0, 0.0, 3, "", true); }) == 400);

    // Quad tree + range tree region.
    Json::Value region = IntelligenceEngine::regionQuery(s, 28.610, 28.622, 77.205, 77.215);
    CHECK(region["cross_check_passed"].asBool());
    CHECK(region["incident_count"].asUInt64() == 1 && region["resource_count"].asUInt64() == 3);
    CHECK(errorStatus([&] { IntelligenceEngine::regionQuery(s, 29, 28, 77, 78); }) == 400);

    // Trie prefix search.
    Json::Value prefix = IntelligenceEngine::prefixSearch(s, "NEXA-INC", 10);
    CHECK(prefix["result_count"].asUInt() == 2 && prefix["results"][0]["entity"].asString() == "INCIDENT");
    Json::Value amb = IntelligenceEngine::prefixSearch(s, "amb", 10);
    CHECK(amb["result_count"].asUInt() == 2 && amb["results"][0]["entity"].asString() == "RESOURCE");
    CHECK(IntelligenceEngine::prefixSearch(s, "fire", 10)["result_count"].asUInt() == 2); // incident 1 + truck
    CHECK(errorStatus([&] { IntelligenceEngine::prefixSearch(s, "", 10); }) == 400);

    // Suffix array pattern search.
    Json::Value pattern = IntelligenceEngine::patternSearch(s, "fire", 50);
    CHECK(pattern["total_occurrences"].asUInt64() >= 5);
    CHECK(pattern["results"][0]["source"].asString() == "INCIDENT");
    CHECK(IntelligenceEngine::patternSearch(s, "evacuation", 50)["matching_documents"].asUInt64() == 2);
    CHECK(errorStatus([&] { IntelligenceEngine::patternSearch(s, "a", 50); }) == 400);

    // Ranking workflow for the pending ambulance request.
    Json::Value rank = IntelligenceEngine::rankResourcesForRequest(s, 4, "merge");
    CHECK(rank["candidate_count"].asUInt64() == 2 && rank["can_fully_satisfy"].asBool());
    CHECK(rank["ranked_resources"][0]["resource_id"].asUInt64() == 1);
    CHECK(rank["ranked_resources"][0]["suggested_units"].asUInt() == 1);
    CHECK(rank["ranked_resources"][1]["suggested_units"].asUInt() == 0);
    Json::Value rankQuick = IntelligenceEngine::rankResourcesForRequest(s, 4, "quick");
    CHECK(rankQuick["ranked_resources"] == rank["ranked_resources"]);
    CHECK(errorStatus([&] { IntelligenceEngine::rankResourcesForRequest(s, 4, "bogo"); }) == 400);
    CHECK(errorStatus([&] { IntelligenceEngine::rankResourcesForRequest(s, 42, ""); }) == 404);

    // Indexing: severity order, trees, lookups.
    Json::Value index = IntelligenceEngine::incidentIndex(s, 2, 0, "nexa-inc-002");
    CHECK(index["ordered_by_severity"][0]["severity"].asString() == "CRITICAL");
    CHECK(index["id_range"]["incidents"].size() == 1);
    CHECK(index["code_lookup"]["found"].asBool() && index["code_lookup"]["methods_agree"].asBool());
    CHECK(index["tree_comparison"]["avl_valid"].asBool() && index["tree_comparison"]["red_black_valid"].asBool());
    CHECK(!IntelligenceEngine::incidentIndex(s, 0, 0, "NOPE")["code_lookup"]["found"].asBool());
    CHECK(IntelligenceEngine::incidentIndex(s, 0, 0, "NOPE")["code_lookup"]["methods_agree"].asBool());

    // Linked-list timeline is chronological.
    Json::Value timeline = IntelligenceEngine::incidentTimeline(s, 1);
    bool chronological = true;
    for (Json::ArrayIndex i = 1; i < timeline["timeline"].size(); ++i)
        chronological = chronological && timeline["timeline"][i - 1]["time"].asInt64() <= timeline["timeline"][i]["time"].asInt64();
    CHECK(chronological && timeline["entry_count"].asUInt64() == 1 + 4 + 3 + 5);

    // Stack: most recent first.
    Json::Value history = IntelligenceEngine::operationsHistory(s, 2);
    CHECK(history["stack_size"].asUInt64() == 5 && history["returned"].asUInt() == 2);
    CHECK(history["most_recent_first"][0]["event_id"].asUInt64() == 7);
    CHECK(history["most_recent_first"][1]["reverse_action"].asString().find("release") != std::string::npos);

    // Queue vs priority queue.
    Json::Value queue = IntelligenceEngine::requestQueue(s);
    CHECK(queue["open_request_count"].asUInt64() == 1 && queue["fifo_order"][0]["request_id"].asUInt64() == 4);
    CHECK(queue["priority_order"][0]["incident_severity"].asString() == "CRITICAL");

    // Multi-linked list + sparse matrix.
    Json::Value matrix = IntelligenceEngine::deploymentMatrix(s, false);
    CHECK(matrix["relationship_count"].asUInt64() == 4);
    CHECK(matrix["sparse_matrix"]["non_zero"].asUInt64() == 3);
    bool truckTwoUnits = false;
    for (const auto& e : matrix["sparse_matrix"]["entries"])
        if (e["incident_id"].asUInt64() == 1 && e["resource_id"].asUInt64() == 2) truckTwoUnits = e["units"].asInt64() == 2;
    CHECK(truckTwoUnits);
    CHECK(IntelligenceEngine::deploymentMatrix(s, true)["relationship_count"].asUInt64() == 5);

    // Interval + segment tree availability over the last 6 hours.
    Json::Value avail = IntelligenceEngine::availability(s, s.now - 6 * 3600, s.now, 0, 3600);
    CHECK(avail["allocations_in_window"].size() == 5);
    CHECK(avail["segment_tree"]["buckets"].asUInt64() == 6);
    CHECK(avail["segment_tree"]["peak_active_units"].asInt64() == 6);
    CHECK(std::fabs(avail["median_completed_allocation_minutes"].asDouble() - 20.0) < 1e-9);
    CHECK(IntelligenceEngine::availability(s, s.now - 3600, s.now, 5, 600)["allocations_in_window"].size() == 0);
    CHECK(errorStatus([&] { IntelligenceEngine::availability(s, 100, 50, 0, 60); }) == 400);

    // MST over the seeded road network: 1.5 + 2.0 + 2.5.
    Json::Value mst = IntelligenceEngine::networkMst(s);
    CHECK(mst["is_connected"].asBool() && mst["kruskal_prim_agree"].asBool());
    CHECK(std::fabs(mst["total_distance_km"].asDouble() - 6.0) < 1e-9 && mst["mst_edges"].size() == 3);

    // K-ary classification.
    Json::Value cls = IntelligenceEngine::classification(s);
    CHECK(cls["placements"][0]["path"].size() == 4);
    CHECK(cls["placements"][0]["path"][2].asString() == "FIRE_AND_EXPLOSION");
    CHECK(cls["placements"][1]["path"][1].asString() == "NATURAL");
    CHECK(cls["tree"]["incident_count"].asUInt64() == 2);

    std::cout << "NEXA intelligence engine: " << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
