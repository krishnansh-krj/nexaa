#include "repositories/AllocationRepository.hpp"

#include <utility>

namespace nexa
{
    void AllocationRepository::findAll(
        ResultCallback onSuccess,
        ErrorCallback onError) const
{
        execute(
            "SELECT * FROM resource_allocations "
            "ORDER BY assigned_at DESC",
            std::move(onSuccess),
            std::move(onError)
        );
    }

    void AllocationRepository::findById(
        unsigned long long allocationId,
        ResultCallback onSuccess,
        ErrorCallback onError) const
{
        execute(
            "SELECT * FROM resource_allocations WHERE allocation_id = ?",
            std::move(onSuccess),
            std::move(onError),
            allocationId
        );
    }

    void AllocationRepository::findByIncident(
        unsigned long long incidentId,
        ResultCallback onSuccess,
        ErrorCallback onError) const
{
        execute(
            "SELECT * FROM resource_allocations "
            "WHERE incident_id = ? ORDER BY assigned_at DESC",
            std::move(onSuccess),
            std::move(onError),
            incidentId
        );
    }

    void AllocationRepository::findByResource(
        unsigned long long resourceId,
        ResultCallback onSuccess,
        ErrorCallback onError) const
{
        execute(
            "SELECT * FROM resource_allocations "
            "WHERE resource_id = ? ORDER BY assigned_at DESC",
            std::move(onSuccess),
            std::move(onError),
            resourceId
        );
    }

} // namespace nexa
