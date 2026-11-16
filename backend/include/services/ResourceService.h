
#pragma once

#include "repositories/ResourceRepository.h"
#include "utilities/Validation.h"

#include <memory>
#include <string>
#include <utility>

namespace nexa
{
class ResourceService
{
private:
    ResourceRepository repository;

public:
    explicit ResourceService(
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
            LOG_WARN << "Invalid resource ID";
            return;
        }

        repository.findById(
            id, std::move(success), std::move(error));
    }

    void getAvailable(
        const std::string& type,
        BaseRepository::ResultCallback success,
        BaseRepository::ErrorCallback error)
    {
        if (!Validation::isNonEmpty(type))
        {
            LOG_WARN << "Resource type is required";
            return;
        }

        repository.findAvailable(
            type, std::move(success), std::move(error));
    }

    bool validateNewResource(
        const std::string& code,
        const std::string& name,
        const std::string& type,
        unsigned int units) const
    {
        return Validation::isNonEmpty(code) &&
               Validation::isNonEmpty(name) &&
               Validation::isValidEnum(
                   type,
                   {"AMBULANCE", "FIRE_TRUCK", "RESCUE_TEAM",
                    "MEDICAL_TEAM", "SUPPLIES", "SHELTER",
                    "HOSPITAL", "OTHER"}) &&
               units > 0;
    }
};
}
