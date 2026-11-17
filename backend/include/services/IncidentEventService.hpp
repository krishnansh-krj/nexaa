#pragma once

#include "repositories/IncidentEventRepository.hpp"

#include <memory>
#include <string>

namespace nexa {
class IncidentEventService {
private:
    IncidentEventRepository repository;
public:
    explicit IncidentEventService(std::shared_ptr<drogon::orm::DbClient> db);
    void getTimeline(unsigned long long incidentId, BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error);
    void recordEvent(unsigned long long incidentId, const std::string& type, const std::string& description, BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error);
};
} // namespace nexa
