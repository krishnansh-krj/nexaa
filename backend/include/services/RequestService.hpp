#pragma once

#include "repositories/RequestRepository.hpp"
 #include "utilities/Validation.hpp"

#include <memory>
#include <string>

namespace nexa {
class RequestService {
private:
    RequestRepository repository;
public:
    explicit RequestService(std::shared_ptr<drogon::orm::DbClient> db);
    void getAll(BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error);
    void getById(unsigned long long id, BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error);
    void getByIncident(unsigned long long incidentId, BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error);
    bool validateNewRequest(const std::string& type, const std::string& priority, unsigned int quantity) const;
};
} // namespace nexa
