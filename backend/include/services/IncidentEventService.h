
#pragma once

#include "repositories/IncidentEventRepository.h"

#include <memory>
#include <string>
#include <utility>

namespace nexa
{
class IncidentEventService
{
private:
    IncidentEventRepository repository;

public:
    explicit IncidentEventService(
        std::shared_ptr<drogon::orm::DbClient> db)
        : repository(std::move(db))
    {
    }

    void getTimeline(
        unsigned long long incidentId,
        BaseRepository::ResultCallback success,
        BaseRepository::ErrorCallback error)
    {
        if (incidentId == 0)
        {
            LOG_WARN << "Invalid incident ID";
            return;
        }

        repository.findByIncident(
            incidentId, std::move(success), std::move(error));
    }

    void recordEvent(
        unsigned long long incidentId,
        const std::string& type,
        const std::string& description,
        BaseRepository::ResultCallback success,
        BaseRepository::ErrorCallback error)
    {
        if (incidentId == 0 || type.empty() || description.empty())
        {
            LOG_WARN << "Invalid incident event data";
            return;
        }

        repository.create(
            incidentId,
            type,
            description,
            std::move(success),
            std::move(error));
    }
};
}
