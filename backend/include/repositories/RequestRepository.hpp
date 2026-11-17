
#pragma once

#include "BaseRepository.hpp"

namespace nexa
{
class RequestRepository : public BaseRepository
{
public:
    using BaseRepository::BaseRepository;

    void findAll(
        ResultCallback onSuccess,
        ErrorCallback onError) const;

    void findById(
        unsigned long long requestId,
        ResultCallback onSuccess,
        ErrorCallback onError) const;

    void findByIncident(
        unsigned long long incidentId,
        ResultCallback onSuccess,
        ErrorCallback onError) const;

    void create(
        unsigned long long incidentId,
        const std::string& requestType,
        const std::string& description,
        const std::string& priority,
        unsigned int quantity,
        ResultCallback onSuccess,
        ErrorCallback onError) const;

    void findPrioritizable(
        ResultCallback onSuccess,
        ErrorCallback onError) const;

};
}
