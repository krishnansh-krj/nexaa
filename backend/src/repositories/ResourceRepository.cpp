#include "repositories/ResourceRepository.hpp"

#include <utility>

namespace nexa
{
    void ResourceRepository::findAll(
        ResultCallback onSuccess,
        ErrorCallback onError) const
{
        execute(
            "SELECT * FROM resources ORDER BY resource_id DESC",
            std::move(onSuccess),
            std::move(onError)
        );
    }

    void ResourceRepository::findById(
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

    void ResourceRepository::findAvailable(
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

    void ResourceRepository::create(
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

    void ResourceRepository::create(
        const std::string& code,
        const std::string& name,
        const std::string& type,
        unsigned int totalUnits,
        const std::optional<double>& latitude,
        const std::optional<double>& longitude,
        const std::string& address,
        ResultCallback onSuccess,
        ErrorCallback onError) const
{
        if (latitude && longitude)
        {
            execute(
                "INSERT INTO resources "
                "(resource_code, resource_name, resource_type, "
                "total_units, available_units, latitude, longitude, address) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, NULLIF(?, ''))",
                std::move(onSuccess),
                std::move(onError),
                code,
                name,
                type,
                totalUnits,
                totalUnits,
                *latitude,
                *longitude,
                address
            );
            return;
        }

        execute(
            "INSERT INTO resources "
            "(resource_code, resource_name, resource_type, "
            "total_units, available_units, address) "
            "VALUES (?, ?, ?, ?, ?, NULLIF(?, ''))",
            std::move(onSuccess),
            std::move(onError),
            code,
            name,
            type,
            totalUnits,
            totalUnits,
            address
        );
    }

} // namespace nexa
