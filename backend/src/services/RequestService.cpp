#include "services/RequestService.hpp"

#include <utility>

namespace nexa {
RequestService::RequestService(std::shared_ptr<drogon::orm::DbClient> db) : repository(std::move(db)) {}
void RequestService::getAll(BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error) { repository.findAll(std::move(success), std::move(error)); }
void RequestService::getById(unsigned long long id, BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error) {
    if (id == 0) { LOG_WARN << "Invalid request ID"; return; }
    repository.findById(id, std::move(success), std::move(error));
}
void RequestService::getByIncident(unsigned long long incidentId, BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error) {
    if (incidentId == 0) { LOG_WARN << "Invalid incident ID"; return; }
    repository.findByIncident(incidentId, std::move(success), std::move(error));
}
bool RequestService::validateNewRequest(const std::string& type, const std::string& priority, unsigned int quantity) const {
    return Validation::isNonEmpty(type) && Validation::isValidEnum(priority, {"LOW", "MEDIUM", "HIGH", "CRITICAL"}) && quantity > 0;
}
} // namespace nexa
