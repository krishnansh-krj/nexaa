#pragma once

#include "repositories/IncidentRepository.hpp"
 #include "utilities/Validation.hpp"

#include <memory>
#include <string>

namespace nexa {
class IncidentService {
private:
    IncidentRepository repository;
public:
    explicit IncidentService(std::shared_ptr<drogon::orm::DbClient> db);
    void getAll(BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error);
    void getById(unsigned long long id, BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error);
    bool validateNewIncident(const std::string& code, const std::string& title, const std::string& type, const std::string& severity) const;
};
} // namespace nexa
