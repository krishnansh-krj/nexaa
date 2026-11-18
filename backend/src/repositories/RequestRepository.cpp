#include "repositories/RequestRepository.hpp"

#include <utility>

namespace nexa
{
    void RequestRepository::findAll(
        ResultCallback onSuccess,
        ErrorCallback onError) const
{
        execute(
            "SELECT * FROM emergency_requests "
            "ORDER BY requested_at DESC",
            std::move(onSuccess),
            std::move(onError)
        );
    }

    void RequestRepository::findById(
        unsigned long long requestId,
        ResultCallback onSuccess,
        ErrorCallback onError) const
{
        execute(
            "SELECT * FROM emergency_requests WHERE request_id = ?",
            std::move(onSuccess),
            std::move(onError),
            requestId
        );
    }

    void RequestRepository::findByIncident(
        unsigned long long incidentId,
        ResultCallback onSuccess,
        ErrorCallback onError) const
{
        execute(
            "SELECT * FROM emergency_requests "
            "WHERE incident_id = ? ORDER BY requested_at DESC",
            std::move(onSuccess),
            std::move(onError),
            incidentId
        );
    }

    void RequestRepository::create(
        unsigned long long incidentId,
        const std::string& requestType,
        const std::string& description,
        const std::string& priority,
        unsigned int quantity,
        ResultCallback onSuccess,
        ErrorCallback onError) const
    {
        execute(
            "INSERT INTO emergency_requests "
            "(incident_id, request_type, description, priority, "
            "quantity_required) VALUES (?, ?, ?, ?, ?)",
            std::move(onSuccess),
            std::move(onError),
            incidentId,
            requestType,
            description,
            priority,
            quantity
        );
    }

        void RequestRepository::createForActiveIncident(
        unsigned long long incidentId,
        const std::string& requestType,
        const std::string& description,
        const std::string& priority,
        unsigned int quantity,
        ResultCallback onSuccess,
        ErrorCallback onError) const
    {
        execute(
            "INSERT INTO emergency_requests "
            "(incident_id, request_type, description, priority, "
            "quantity_required) "
            "SELECT ?, ?, ?, ?, ? "
            "FROM incidents "
            "WHERE incident_id = ? AND status = 'ACTIVE'",
            std::move(onSuccess),
            std::move(onError),
            incidentId,
            requestType,
            description,
            priority,
            quantity,
            incidentId
        );
    }

    void RequestRepository::findPrioritizable(
        ResultCallback onSuccess,
        ErrorCallback onError) const
{
        execute(
            "SELECT "
            "er.request_id, "
            "er.incident_id, "
            "er.request_type, "
            "er.description, "
            "er.priority AS request_priority, "
            "er.quantity_required, "
            "er.requested_at, "
            "i.severity AS incident_severity, "
            "COALESCE(a.allocated_units, 0) AS allocated_units, "
            "(er.quantity_required - "
            "COALESCE(a.allocated_units, 0)) AS outstanding_units "
            "FROM emergency_requests er "
            "INNER JOIN incidents i "
            "ON i.incident_id = er.incident_id "
            "LEFT JOIN ("
            " SELECT request_id, "
            " SUM(allocated_units) AS allocated_units "
            " FROM resource_allocations "
            " WHERE status IN "
            " ('ASSIGNED', 'DISPATCHED', 'ON_SITE') "
            " GROUP BY request_id"
            ") a ON a.request_id = er.request_id "
            "WHERE i.status = 'ACTIVE' "
            "AND er.status IN ('PENDING', 'PROCESSING') "
            "AND er.quantity_required > "
            "COALESCE(a.allocated_units, 0) "
            "ORDER BY er.requested_at ASC",
            std::move(onSuccess),
            std::move(onError)
        );
    }

} // namespace nexa
