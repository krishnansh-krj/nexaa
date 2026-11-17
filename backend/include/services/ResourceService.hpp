#pragma once

#include "repositories/ResourceRepository.hpp"
 #include "utilities/Validation.hpp"

#include <memory>
#include <string>

namespace nexa {
class ResourceService {
private:
    ResourceRepository repository;
public:
    explicit ResourceService(std::shared_ptr<drogon::orm::DbClient> db);
    void getAll(BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error);
    void getById(unsigned long long id, BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error);
    void getAvailable(const std::string& type, BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error);
    bool validateNewResource(const std::string& code, const std::string& name, const std::string& type, unsigned int units) const;
};
} // namespace nexa
