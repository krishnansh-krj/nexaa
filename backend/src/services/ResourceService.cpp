#include "services/ResourceService.hpp"

#include <utility>

namespace nexa {
ResourceService::ResourceService(std::shared_ptr<drogon::orm::DbClient> db) : repository(std::move(db)) {}
void ResourceService::getAll(BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error) { repository.findAll(std::move(success), std::move(error)); }
void ResourceService::getById(unsigned long long id, BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error) {
    if (id == 0) { LOG_WARN << "Invalid resource ID"; return; }
    repository.findById(id, std::move(success), std::move(error));
}
void ResourceService::getAvailable(const std::string& type, BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error) {
    if (!Validation::isNonEmpty(type)) { LOG_WARN << "Resource type is required"; return; }
    repository.findAvailable(type, std::move(success), std::move(error));
}
bool ResourceService::validateNewResource(const std::string& code, const std::string& name, const std::string& type, unsigned int units) const {
    return Validation::isNonEmpty(code) && Validation::isNonEmpty(name) && Validation::isValidEnum(type, {"AMBULANCE", "FIRE_TRUCK", "RESCUE_TEAM", "MEDICAL_TEAM", "SUPPLIES", "SHELTER", "HOSPITAL", "OTHER"}) && units > 0;
}
} // namespace nexa
