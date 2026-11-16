
#pragma once

#include "repositories/IncidentRepository.h"
#include "utilities/Validation.h"

#include <memory>
#include <string>
#include <utility>

namespace nexa
{
class IncidentService
{
private:
    IncidentRepository repository;

public:
    explicit IncidentService(
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
            LOG_WARN << "Invalid incident ID";
            return;
        }

        repository.findById(
            id, std::move(success), std::move(error));
    }

    bool validateNewIncident(
        const std::string& code,
        const std::string& title,
        const std::string& type,
        const std::string& severity) const
    {
        return Validation::isNonEmpty(code) &&
               Validation::isNonEmpty(title) &&
               Validation::isNonEmpty(type) &&
               Validation::isValidEnum(
                   severity,
                   {"LOW", "MEDIUM", "HIGH", "CRITICAL"});
    }
};
}
