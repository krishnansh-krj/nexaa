#include "services/NetworkService.hpp"

#include <utility>

namespace nexa {
NetworkService::NetworkService(std::shared_ptr<drogon::orm::DbClient> db) : repository(std::move(db)) {}
void NetworkService::getNodes(BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error) { repository.findAllNodes(std::move(success), std::move(error)); }
void NetworkService::getEdges(BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error) { repository.findAllEdges(std::move(success), std::move(error)); }
void NetworkService::getOutgoingEdges(unsigned long long nodeId, BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error) {
    if (nodeId == 0) { LOG_WARN << "Invalid network node ID"; return; }
    repository.findEdgesFromNode(nodeId, std::move(success), std::move(error));
}
void NetworkService::blockEdge(unsigned long long edgeId, bool blocked, BaseRepository::ResultCallback success, BaseRepository::ErrorCallback error) {
    if (edgeId == 0) { LOG_WARN << "Invalid network edge ID"; return; }
    repository.setEdgeBlocked(edgeId, blocked, std::move(success), std::move(error));
}
} // namespace nexa
