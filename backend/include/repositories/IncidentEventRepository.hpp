
#pragma once

#include "BaseRepository.hpp"

namespace nexa
{
class IncidentEventRepository : public BaseRepository
{
public:
    using BaseRepository::BaseRepository;

    void findByIncident(
        unsigned long long incidentId,
        ResultCallback onSuccess,
        ErrorCallback onError) const;

    void create(
        unsigned long long incidentId,
        const std::string& eventType,
        const std::string& description,
        ResultCallback onSuccess,
        ErrorCallback onError) const;
};
}
