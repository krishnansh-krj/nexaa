
#pragma once

#include "BaseRepository.h"

namespace nexa
{
class ResourceRepository : public BaseRepository
{
public:
    using BaseRepository::BaseRepository;

    void findAll(
        ResultCallback onSuccess,
        ErrorCallback onError) const
    {
        execute(
            "SELECT * FROM resources ORDER BY resource_id DESC",
            std::move(onSuccess),
            std::move(onError)
        );
    }

    void findById(
        unsigned long long resourceId,
        ResultCallback onSuccess,
        ErrorCallback onError) const
    {
        execute(
            "SELECT * FROM resources WHERE resource_id = ?",
            std::move(onSuccess),
            std::move(onError),
            resourceId
        );
    }

    void findAvailable(
        const std::string& resourceType,
        ResultCallback onSuccess,
        ErrorCallback onError) const
    {
        execute(
            "SELECT * FROM resources "
            "WHERE status = 'AVAILABLE' "
            "AND available_units > 0 "
            "AND resource_type = ?",
            std::move(onSuccess),
            std::move(onError),
            resourceType
        );
    }

    void create(
        const std::string& code,
        const std::string& name,
        const std::string& type,
        unsigned int totalUnits,
        ResultCallback onSuccess,
        ErrorCallback onError) const
    {
        execute(
            "INSERT INTO resources "
            "(resource_code, resource_name, resource_type, "
            "total_units, available_units) "
            "VALUES (?, ?, ?, ?, ?)",
            std::move(onSuccess),
            std::move(onError),
            code,
            name,
            type,
            totalUnits,
            totalUnits
        );
    }
};
}
