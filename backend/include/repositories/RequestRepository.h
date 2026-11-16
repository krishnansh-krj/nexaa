
#pragma once

#include "BaseRepository.h"

namespace nexa
{
class RequestRepository : public BaseRepository
{
public:
    using BaseRepository::BaseRepository;

    void findAll(
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

    void findById(
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

    void findByIncident(
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

    void create(
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
};
}
