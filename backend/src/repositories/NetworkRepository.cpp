#include "repositories/NetworkRepository.hpp"

#include <utility>

namespace nexa
{
    void NetworkRepository::findAllNodes(
        ResultCallback onSuccess,
        ErrorCallback onError) const
{
        execute(
            "SELECT * FROM network_nodes ORDER BY node_id",
            std::move(onSuccess),
            std::move(onError)
        );
    }

    void NetworkRepository::findNodeById(
        unsigned long long nodeId,
        ResultCallback onSuccess,
        ErrorCallback onError) const
{
        execute(
            "SELECT * FROM network_nodes WHERE node_id = ?",
            std::move(onSuccess),
            std::move(onError),
            nodeId
        );
    }

    void NetworkRepository::findAllEdges(
        ResultCallback onSuccess,
        ErrorCallback onError) const
{
        execute(
            "SELECT * FROM network_edges ORDER BY edge_id",
            std::move(onSuccess),
            std::move(onError)
        );
    }

    void NetworkRepository::findEdgesFromNode(
        unsigned long long nodeId,
        ResultCallback onSuccess,
        ErrorCallback onError) const
{
        execute(
            "SELECT * FROM network_edges "
            "WHERE source_node_id = ? AND is_blocked = FALSE",
            std::move(onSuccess),
            std::move(onError),
            nodeId
        );
    }

    void NetworkRepository::setEdgeBlocked(
        unsigned long long edgeId,
        bool blocked,
        ResultCallback onSuccess,
        ErrorCallback onError) const
{
        execute(
            "UPDATE network_edges SET is_blocked = ? WHERE edge_id = ?",
            std::move(onSuccess),
            std::move(onError),
            blocked,
            edgeId
        );
    }

} // namespace nexa
