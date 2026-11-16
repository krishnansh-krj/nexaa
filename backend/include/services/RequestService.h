
#pragma once

#include "repositories/RequestRepository.h"
#include "utilities/Validation.h"

#include <memory>
#include <string>
#include <utility>

namespace nexa
{
class RequestService
{
private:
    RequestRepository repository;

public:
    explicit RequestService(
        std::shared_ptr<drogon::orm::DbClient> db)
        : repository(std::move(db))
    {
    }

    void getAll(
        BaseRepository::ResultCallback success,
        BaseRepository::ErrorCallback error)
    {
        repository.findAll(
            std::move(success), std::move(error));
    }

    void getById(
        unsigned long long id,
        BaseRepository::ResultCallback success,
        BaseRepository::ErrorCallback error)
    {
        if (id == 0)
        {
            LOG_WARN << "Invalid request ID";
            return;
        }

        repository.findById(
            id, std::move(success), std::move(error));
    }

    void getByIncident(
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

    bool validateNewRequest(
        const std::string& type,
        const std::string& priority,
        unsigned int quantity) const
    {
        return Validation::isNonEmpty(type) &&
               Validation::isValidEnum(
                   priority,
                   {"LOW", "MEDIUM", "HIGH", "CRITICAL"}) &&
               quantity > 0;
    }
};
}
