#include "services/DsaIntelligenceService.hpp"

#include <exception>
#include <vector>

namespace nexa
{

namespace
{

using drogon::orm::Result;
using drogon::orm::Row;
using intel::Snapshot;

std::string text(const Row& row, const char* column)
{
    return row[column].isNull() ? std::string() : row[column].as<std::string>();
}

long long epoch(const Row& row, const char* column)
{
    return row[column].isNull() ? 0LL : row[column].as<long long>();
}

void applyClock(const Result& r, Snapshot& s)
{
    s.now = r.empty() ? 0LL : epoch(r[0], "now_epoch");
}

void applyIncidents(const Result& r, Snapshot& s)
{
    for (const auto& row : r)
    {
        intel::IncidentRecord i;
        i.id = row["incident_id"].as<unsigned long long>();
        i.code = text(row, "incident_code");
        i.title = text(row, "title");
        i.description = text(row, "description");
        i.type = text(row, "incident_type");
        i.severity = text(row, "severity");
        i.status = text(row, "status");
        i.hasLocation = !row["latitude"].isNull() && !row["longitude"].isNull();
        if (i.hasLocation)
        {
            i.lat = row["latitude"].as<double>();
            i.lng = row["longitude"].as<double>();
        }
        i.reportedAt = epoch(row, "reported_epoch");
        s.incidents.push_back(std::move(i));
    }
}

void applyResources(const Result& r, Snapshot& s)
{
    for (const auto& row : r)
    {
        intel::ResourceRecord x;
        x.id = row["resource_id"].as<unsigned long long>();
        x.code = text(row, "resource_code");
        x.name = text(row, "resource_name");
        x.type = text(row, "resource_type");
        x.status = text(row, "status");
        x.address = text(row, "address");
        x.totalUnits = row["total_units"].as<unsigned int>();
        x.availableUnits = row["available_units"].as<unsigned int>();
        x.hasLocation = !row["latitude"].isNull() && !row["longitude"].isNull();
        if (x.hasLocation)
        {
            x.lat = row["latitude"].as<double>();
            x.lng = row["longitude"].as<double>();
        }
        s.resources.push_back(std::move(x));
    }
}

void applyRequests(const Result& r, Snapshot& s)
{
    for (const auto& row : r)
    {
        intel::RequestRecord q;
        q.id = row["request_id"].as<unsigned long long>();
        q.incidentId = row["incident_id"].as<unsigned long long>();
        q.type = text(row, "request_type");
        q.description = text(row, "description");
        q.priority = text(row, "priority");
        q.status = text(row, "status");
        q.quantityRequired = row["quantity_required"].as<unsigned int>();
        q.allocatedUnits = row["allocated_units"].as<unsigned int>();
        q.requestedAt = epoch(row, "requested_epoch");
        s.requests.push_back(std::move(q));
    }
}

void applyAllocations(const Result& r, Snapshot& s)
{
    for (const auto& row : r)
    {
        intel::AllocationRecord a;
        a.id = row["allocation_id"].as<unsigned long long>();
        a.incidentId = row["incident_id"].as<unsigned long long>();
        a.resourceId = row["resource_id"].as<unsigned long long>();
        a.requestId = row["request_id"].isNull() ? 0ULL : row["request_id"].as<unsigned long long>();
        a.units = row["allocated_units"].as<unsigned int>();
        a.status = text(row, "status");
        a.assignedAt = epoch(row, "assigned_epoch");
        a.releasedAt = epoch(row, "released_epoch");
        s.allocations.push_back(std::move(a));
    }
}

void applyEvents(const Result& r, Snapshot& s)
{
    for (const auto& row : r)
    {
        intel::EventRecord e;
        e.id = row["event_id"].as<unsigned long long>();
        e.incidentId = row["incident_id"].as<unsigned long long>();
        e.type = text(row, "event_type");
        e.description = text(row, "description");
        e.time = epoch(row, "event_epoch");
        s.events.push_back(std::move(e));
    }
}

void applyNodes(const Result& r, Snapshot& s)
{
    for (const auto& row : r)
    {
        intel::NodeRecord n;
        n.id = row["node_id"].as<unsigned long long>();
        n.code = text(row, "node_code");
        n.name = text(row, "node_name");
        n.type = text(row, "node_type");
        n.lat = row["latitude"].as<double>();
        n.lng = row["longitude"].as<double>();
        s.nodes.push_back(std::move(n));
    }
}

void applyEdges(const Result& r, Snapshot& s)
{
    for (const auto& row : r)
    {
        intel::EdgeRecord e;
        e.id = row["edge_id"].as<unsigned long long>();
        e.source = row["source_node_id"].as<unsigned long long>();
        e.destination = row["destination_node_id"].as<unsigned long long>();
        e.distanceKm = row["distance_km"].as<double>();
        e.blocked = row["is_blocked"].as<bool>();
        s.edges.push_back(std::move(e));
    }
}

struct Step
{
    const char* sql;
    void (*apply)(const Result&, Snapshot&);
};

// Read-only queries, executed one after another on the same client.
const std::vector<Step>& steps()
{
    static const std::vector<Step> list = {
        {"SELECT UNIX_TIMESTAMP() AS now_epoch", &applyClock},

        {"SELECT incident_id, incident_code, title, description, incident_type, "
         "severity, status, latitude, longitude, "
         "UNIX_TIMESTAMP(reported_at) AS reported_epoch "
         "FROM incidents ORDER BY incident_id",
         &applyIncidents},

        {"SELECT resource_id, resource_code, resource_name, resource_type, status, "
         "total_units, available_units, latitude, longitude, address "
         "FROM resources ORDER BY resource_id",
         &applyResources},

        {"SELECT er.request_id, er.incident_id, er.request_type, er.description, "
         "er.priority, er.status, er.quantity_required, "
         "UNIX_TIMESTAMP(er.requested_at) AS requested_epoch, "
         "COALESCE(a.allocated_units, 0) AS allocated_units "
         "FROM emergency_requests er "
         "LEFT JOIN (SELECT request_id, SUM(allocated_units) AS allocated_units "
         "FROM resource_allocations "
         "WHERE status IN ('ASSIGNED', 'DISPATCHED', 'ON_SITE') "
         "GROUP BY request_id) a ON a.request_id = er.request_id "
         "ORDER BY er.request_id",
         &applyRequests},

        {"SELECT allocation_id, incident_id, resource_id, request_id, allocated_units, status, "
         "UNIX_TIMESTAMP(assigned_at) AS assigned_epoch, "
         "UNIX_TIMESTAMP(released_at) AS released_epoch "
         "FROM resource_allocations ORDER BY allocation_id",
         &applyAllocations},

        {"SELECT event_id, incident_id, event_type, description, "
         "UNIX_TIMESTAMP(event_time) AS event_epoch "
         "FROM incident_events ORDER BY event_id",
         &applyEvents},

        {"SELECT node_id, node_code, node_name, latitude, longitude, node_type "
         "FROM network_nodes ORDER BY node_id",
         &applyNodes},

        {"SELECT edge_id, source_node_id, destination_node_id, distance_km, is_blocked "
         "FROM network_edges ORDER BY edge_id",
         &applyEdges},
    };
    return list;
}

void runStep(std::shared_ptr<drogon::orm::DbClient> db,
             std::shared_ptr<Snapshot> snapshot,
             std::size_t index,
             DsaIntelligenceService::ReadyCallback onReady,
             DsaIntelligenceService::ErrorCallback onError,
             DsaIntelligenceService::FailureCallback onFailure)
{
    if (index >= steps().size())
    {
        onReady(snapshot);
        return;
    }

    const Step& step = steps()[index];
    db->execSqlAsync(
        step.sql,

        [db, snapshot, index, onReady, onError, onFailure](const Result& result)
        {
            try
            {
                steps()[index].apply(result, *snapshot);
            }
            catch (const std::exception& e)
            {
                LOG_ERROR << "NEXA snapshot step " << index << " failed: " << e.what();
                onFailure("Failed to read database snapshot");
                return;
            }
            runStep(db, snapshot, index + 1, onReady, onError, onFailure);
        },

        [onError](const drogon::orm::DrogonDbException& e)
        {
            onError(e);
        });
}

} // namespace

void DsaIntelligenceService::loadSnapshot(const std::shared_ptr<drogon::orm::DbClient>& db,
                                          ReadyCallback onReady,
                                          ErrorCallback onError,
                                          FailureCallback onFailure)
{
    if (!db)
    {
        onFailure("Database connection is unavailable");
        return;
    }
    runStep(db, std::make_shared<Snapshot>(), 0, std::move(onReady), std::move(onError), std::move(onFailure));
}

} // namespace nexa
