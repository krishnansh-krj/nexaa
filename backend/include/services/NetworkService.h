
#pragma once

#include "repositories/NetworkRepository.h"

#include <memory>
#include <utility>

namespace nexa
{
class NetworkService
{
private:
    NetworkRepository repository;

public:
    explicit NetworkService(
        std::shared_ptr<drogon::orm::DbClient> db)
        : repository(std::move(db))
    {
    }

    void getNodes(
        BaseRepository::ResultCallback success,
        BaseRepository::ErrorCallback error)
    {
        repository.findAllNodes(
            std::move(success), std::move(error));
    }

    void getEdges(
        BaseRepository::ResultCallback success,
        BaseRepository::ErrorCallback error)
    {
        repository.findAllEdges(
            std::move(success), std::move(error));
    }

    void getOutgoingEdges(
        unsigned long long nodeId,
        BaseRepository::ResultCallback success,
        BaseRepository::ErrorCallback error)
    {
        if (nodeId == 0)
        {
            LOG_WARN << "Invalid network node ID";
            return;
        }

        repository.findEdgesFromNode(
            nodeId, std::move(success), std::move(error));
    }

    void blockEdge(
        unsigned long long edgeId,
        bool blocked,
        BaseRepository::ResultCallback success,
        BaseRepository::ErrorCallback error)
    {
        if (edgeId == 0)
        {
            LOG_WARN << "Invalid network edge ID";
            return;
        }

        repository.setEdgeBlocked(
            edgeId,
            blocked,
            std::move(success),
            std::move(error));
    }
};
}
