
#pragma once

#include "BaseRepository.hpp"

namespace nexa
{
class AllocationRepository : public BaseRepository
{
public:
    using BaseRepository::BaseRepository;

    void findAll(
        ResultCallback onSuccess,
        ErrorCallback onError) const;

    void findById(
        unsigned long long allocationId,
        ResultCallback onSuccess,
        ErrorCallback onError) const;

    void findByIncident(
        unsigned long long incidentId,
        ResultCallback onSuccess,
        ErrorCallback onError) const;

    void findByResource(
        unsigned long long resourceId,
        ResultCallback onSuccess,
        ErrorCallback onError) const;
};
}
