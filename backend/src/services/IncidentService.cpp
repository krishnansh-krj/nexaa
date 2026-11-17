#include "services/IncidentService.hpp"

#include <utility>

namespace nexa {
IncidentService::IncidentService(std::shared_ptr<drogon::orm::DbClient> db) : repository(std::move(db)) {}
void IncidentService::getAll(BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error) { repository.findAll(std::move(success), std::move(error)); }
void IncidentService::getById(unsigned long long id, BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error) {
    if (id == 0) { LOG_WARN << "Invalid incident ID"; return; }
    repository.findById(id, std::move(success), std::move(error));
}
bool IncidentService::validateNewIncident(const std::string& code, const std::string& title, const std::string& type, const std::string& severity) const {
    return Validation::isNonEmpty(code) && Validation::isNonEmpty(title) && Validation::isNonEmpty(type) && Validation::isValidEnum(severity, {"LOW", "MEDIUM", "HIGH", "CRITICAL"});
}
} // namespace nexa
