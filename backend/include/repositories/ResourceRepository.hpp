
#pragma once

#include "BaseRepository.hpp"

#include <optional>
#include <string>

namespace nexa
{
class ResourceRepository : public BaseRepository
{
public:
    using BaseRepository::BaseRepository;

    void findAll(
        ResultCallback onSuccess,
        ErrorCallback onError) const;

    void findById(
        unsigned long long resourceId,
        ResultCallback onSuccess,
        ErrorCallback onError) const;

    void findAvailable(
        const std::string& resourceType,
        ResultCallback onSuccess,
        ErrorCallback onError) const;

    void create(
        const std::string& code,
        const std::string& name,
        const std::string& type,
        unsigned int totalUnits,
        ResultCallback onSuccess,
        ErrorCallback onError) const;

    // Same as create(), with an optional map location and address.
    void create(
        const std::string& code,
        const std::string& name,
        const std::string& type,
        unsigned int totalUnits,
        const std::optional<double>& latitude,
        const std::optional<double>& longitude,
        const std::string& address,
        ResultCallback onSuccess,
        ErrorCallback onError) const;
};
}
