#include "repositories/IncidentEventRepository.hpp"

#include <utility>

namespace nexa
{
    void IncidentEventRepository::findByIncident(
        unsigned long long incidentId,
        ResultCallback onSuccess,
        ErrorCallback onError) const
{
        execute(
            "SELECT * FROM incident_events "
            "WHERE incident_id = ? ORDER BY event_time DESC",
            std::move(onSuccess),
            std::move(onError),
            incidentId
        );
    }

    void IncidentEventRepository::create(
        unsigned long long incidentId,
        const std::string& eventType,
        const std::string& description,
        ResultCallback onSuccess,
        ErrorCallback onError) const
{
        execute(
            "INSERT INTO incident_events "
            "(incident_id, event_type, description) VALUES (?, ?, ?)",
            std::move(onSuccess),
            std::move(onError),
            incidentId,
            eventType,
            description
        );
    }

} // namespace nexa
