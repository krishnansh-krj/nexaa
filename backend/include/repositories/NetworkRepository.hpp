
#pragma once

#include "BaseRepository.hpp"

namespace nexa
{
class NetworkRepository : public BaseRepository
{
public:
    using BaseRepository::BaseRepository;

    void findAllNodes(
        ResultCallback onSuccess,
        ErrorCallback onError) const;

    void findNodeById(
        unsigned long long nodeId,
        ResultCallback onSuccess,
        ErrorCallback onError) const;

    void findAllEdges(
        ResultCallback onSuccess,
        ErrorCallback onError) const;

    void findEdgesFromNode(
        unsigned long long nodeId,
        ResultCallback onSuccess,
        ErrorCallback onError) const;

    void setEdgeBlocked(
        unsigned long long edgeId,
        bool blocked,
        ResultCallback onSuccess,
        ErrorCallback onError) const;
};
}
