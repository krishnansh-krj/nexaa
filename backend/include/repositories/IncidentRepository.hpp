
#pragma once

#include "BaseRepository.hpp"

namespace nexa
{
class IncidentRepository : public BaseRepository
{
public:
    using BaseRepository::BaseRepository;

    void findAll(
        ResultCallback onSuccess,
        ErrorCallback onError) const;

    void findById(
        unsigned long long incidentId,
        ResultCallback onSuccess,
        ErrorCallback onError) const;

    void create(
        const std::string& code,
        const std::string& title,
        const std::string& description,
        const std::string& type,
        const std::string& severity,
        ResultCallback onSuccess,
        ErrorCallback onError) const;
};
}
