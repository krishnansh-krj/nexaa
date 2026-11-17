#pragma once

#include "repositories/NetworkRepository.hpp"

#include <memory>
#include <string>

namespace nexa {
class NetworkService {
private:
    NetworkRepository repository;
public:
    explicit NetworkService(std::shared_ptr<drogon::orm::DbClient> db);
    void getNodes(BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error);
    void getEdges(BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error);
    void getOutgoingEdges(unsigned long long nodeId, BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error);
    void blockEdge(unsigned long long edgeId, bool blocked, BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error);
};
} // namespace nexa
