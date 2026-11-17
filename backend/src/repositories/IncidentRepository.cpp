#include "repositories/IncidentRepository.hpp"

#include <utility>

namespace nexa
{
    void IncidentRepository::findAll(
        ResultCallback onSuccess,
        ErrorCallback onError) const
{
        execute(
            "SELECT * FROM incidents ORDER BY reported_at DESC",
            std::move(onSuccess),
            std::move(onError)
        );
    }

    void IncidentRepository::findById(
        unsigned long long incidentId,
        ResultCallback onSuccess,
        ErrorCallback onError) const
{
        execute(
            "SELECT * FROM incidents WHERE incident_id = ?",
            std::move(onSuccess),
            std::move(onError),
            incidentId
        );
    }

    void IncidentRepository::create(
        const std::string& code,
        const std::string& title,
        const std::string& description,
        const std::string& type,
        const std::string& severity,
        ResultCallback onSuccess,
        ErrorCallback onError) const
{
        execute(
            "INSERT INTO incidents "
            "(incident_code, title, description, incident_type, severity) "
            "VALUES (?, ?, ?, ?, ?)",
            std::move(onSuccess),
            std::move(onError),
            code,
            title,
            description,
            type,
            severity
        );
    }

} // namespace nexa
