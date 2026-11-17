#include "services/IncidentEventService.hpp"

#include <utility>

namespace nexa {
IncidentEventService::IncidentEventService(std::shared_ptr<drogon::orm::DbClient> db) : repository(std::move(db)) {}
void IncidentEventService::getTimeline(unsigned long long incidentId, BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error) {
    if (incidentId == 0) { LOG_WARN << "Invalid incident ID"; return; }
    repository.findByIncident(incidentId, std::move(success), std::move(error));
}
void IncidentEventService::recordEvent(unsigned long long incidentId, const std::string& type, const std::string& description, BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error) {
    if (incidentId == 0 || type.empty() || description.empty()) { LOG_WARN << "Invalid incident event data"; return; }
    repository.create(incidentId, type, description, std::move(success), std::move(error));
}
} // namespace nexa
