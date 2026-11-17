#include "services/NetworkGraphService.hpp"
#include "algorithms/Dijkstra.hpp"
#include "algorithms/BFS.hpp"
#include "algorithms/DFS.hpp"
#include <climits>
namespace nexa {
int NetworkGraphService::GraphSnapshot::findIndex(unsigned long long databaseId) const { for(int i=0;i<nodeCount;++i) if(databaseIds[i]==databaseId) return i+1; return -1; }
unsigned long long NetworkGraphService::GraphSnapshot::getDatabaseId(int graphVertex) const { if(graphVertex<1 || graphVertex>nodeCount) return 0; return databaseIds[graphVertex-1]; }
void NetworkGraphService::loadGraph(
        const std::shared_ptr<drogon::orm::DbClient>& dbClient,
        std::function<void(SnapshotPtr)> onReady,
        ErrorCallback onError)
{
        auto snapshot = std::make_shared<GraphSnapshot>();

        NetworkRepository nodeRepository(dbClient);

        nodeRepository.findAllNodes(

            [dbClient, snapshot, onReady, onError]
            (const drogon::orm::Result& nodes)
            {
                snapshot->nodeCount =
                    static_cast<int>(nodes.size());

                if (snapshot->nodeCount == 0)
                {
                    snapshot->graph =
                        std::make_unique<Graph>(0);

                    onReady(snapshot);
                    return;
                }

                snapshot->databaseIds =
                    std::make_unique<unsigned long long[]>(
                        snapshot->nodeCount);

                int index = 0;

                for (const auto& row : nodes)
                {
                    snapshot->databaseIds[index++] =
                        row["node_id"].as<unsigned long long>();
                }

                snapshot->graph =
                    std::make_unique<Graph>(
                        snapshot->nodeCount);

                NetworkRepository edgeRepository(dbClient);

                edgeRepository.findAllEdges(

                    [snapshot, onReady]
                    (const drogon::orm::Result& edges)
                    {
                        for (const auto& row : edges)
                        {
                            const auto edgeId =
                                row["edge_id"].as<unsigned long long>();

                            const auto sourceId =
                                row["source_node_id"].as<unsigned long long>();

                            const auto destinationId =
                                row["destination_node_id"].as<unsigned long long>();

                            const double distance =
                                row["distance_km"].as<double>();

                            const bool blocked =
                                row["is_blocked"].as<bool>();

                            // Graph currently stores edge IDs as int.
                            if (edgeId > static_cast<unsigned long long>(INT_MAX))
                                continue;

                            const int source =
                                snapshot->findIndex(sourceId);

                            const int destination =
                                snapshot->findIndex(destinationId);

                            if (source == -1 || destination == -1)
                                continue;

                            snapshot->graph->addEdge(
                                source,
                                destination,
                                static_cast<int>(edgeId),
                                distance,
                                blocked
                            );
                        }

                        onReady(snapshot);
                    },

                    onError
                );
            },

            onError
        );
    }

void NetworkGraphService::shortestPath(
        const std::shared_ptr<drogon::orm::DbClient>& dbClient,
        unsigned long long sourceDatabaseId,
        unsigned long long destinationDatabaseId,
        SuccessCallback onSuccess,
        ErrorCallback onError,
        FailureCallback onFailure)
{
        loadGraph(
            dbClient,

            [sourceDatabaseId,
             destinationDatabaseId,
             onSuccess,
             onFailure]
            (SnapshotPtr snapshot)
            {
                if (snapshot->nodeCount == 0)
                {
                    onFailure(
                        "Network graph contains no nodes",
                        drogon::k404NotFound,
                        "EMPTY_NETWORK"
                    );
                    return;
                }

                const int source =
                    snapshot->findIndex(sourceDatabaseId);

                const int destination =
                    snapshot->findIndex(destinationDatabaseId);

                if (source == -1 || destination == -1)
                {
                    onFailure(
                        "Source or destination node was not found",
                        drogon::k404NotFound,
                        "NODE_NOT_FOUND"
                    );
                    return;
                }

                auto path =
                    std::make_unique<int[]>(
                        snapshot->nodeCount + 1);

                int pathLength = 0;
                double shortestDistance = 0.0;

                const bool found = Dijkstra::shortestPath(
                    *snapshot->graph,
                    source,
                    destination,
                    shortestDistance,
                    path.get(),
                    pathLength
                );

                if (!found)
                {
                    onFailure(
                        "No available route exists between these nodes",
                        drogon::k404NotFound,
                        "NO_ROUTE"
                    );
                    return;
                }

                Json::Value data;

                data["algorithm"] = "DIJKSTRA";

                data["source_node_id"] =
                    Json::UInt64(sourceDatabaseId);

                data["destination_node_id"] =
                    Json::UInt64(destinationDatabaseId);

                data["distance_km"] = shortestDistance;

                data["path_length"] = pathLength;

                data["path"] = Json::Value(Json::arrayValue);

                for (int i = 0; i < pathLength; ++i)
                {
                    data["path"].append(
                        Json::UInt64(
                            snapshot->getDatabaseId(path[i])
                        )
                    );
                }

                onSuccess(std::move(data));
            },

            onError
        );
    }

void NetworkGraphService::breadthFirstSearch(
        const std::shared_ptr<drogon::orm::DbClient>& dbClient,
        unsigned long long startDatabaseId,
        SuccessCallback onSuccess,
        ErrorCallback onError,
        FailureCallback onFailure)
{
        loadGraph(
            dbClient,

            [startDatabaseId, onSuccess, onFailure]
            (SnapshotPtr snapshot)
            {
                if (snapshot->nodeCount == 0)
                {
                    onFailure(
                        "Network graph contains no nodes",
                        drogon::k404NotFound,
                        "EMPTY_NETWORK"
                    );
                    return;
                }

                const int start =
                    snapshot->findIndex(startDatabaseId);

                if (start == -1)
                {
                    onFailure(
                        "Starting node was not found",
                        drogon::k404NotFound,
                        "NODE_NOT_FOUND"
                    );
                    return;
                }

                auto traversal =
                    std::make_unique<int[]>(
                        snapshot->nodeCount + 1);

                int count = 0;

                const bool success = BFS::traverse(
                    *snapshot->graph,
                    start,
                    traversal.get(),
                    count
                );

                if (!success)
                {
                    onFailure(
                        "BFS traversal could not be completed",
                        drogon::k500InternalServerError,
                        "BFS_FAILED"
                    );
                    return;
                }

                Json::Value data;

                data["algorithm"] = "BFS";

                data["start_node_id"] =
                    Json::UInt64(startDatabaseId);

                data["visited_count"] = count;

                data["traversal"] =
                    Json::Value(Json::arrayValue);

                for (int i = 0; i < count; ++i)
                {
                    data["traversal"].append(
                        Json::UInt64(
                            snapshot->getDatabaseId(traversal[i])
                        )
                    );
                }

                onSuccess(std::move(data));
            },

            onError
        );
    }

void NetworkGraphService::depthFirstSearch(
        const std::shared_ptr<drogon::orm::DbClient>& dbClient,
        unsigned long long startDatabaseId,
        SuccessCallback onSuccess,
        ErrorCallback onError,
        FailureCallback onFailure)
{
        loadGraph(
            dbClient,

            [startDatabaseId, onSuccess, onFailure]
            (SnapshotPtr snapshot)
            {
                if (snapshot->nodeCount == 0)
                {
                    onFailure(
                        "Network graph contains no nodes",
                        drogon::k404NotFound,
                        "EMPTY_NETWORK"
                    );
                    return;
                }

                const int start =
                    snapshot->findIndex(startDatabaseId);

                if (start == -1)
                {
                    onFailure(
                        "Starting node was not found",
                        drogon::k404NotFound,
                        "NODE_NOT_FOUND"
                    );
                    return;
                }

                auto traversal =
                    std::make_unique<int[]>(
                        snapshot->nodeCount + 1);

                int count = 0;

                const bool success = DFS::traverse(
                    *snapshot->graph,
                    start,
                    traversal.get(),
                    count
                );

                if (!success)
                {
                    onFailure(
                        "DFS traversal could not be completed",
                        drogon::k500InternalServerError,
                        "DFS_FAILED"
                    );
                    return;
                }

                Json::Value data;

                data["algorithm"] = "DFS";

                data["start_node_id"] =
                    Json::UInt64(startDatabaseId);

                data["visited_count"] = count;

                data["traversal"] =
                    Json::Value(Json::arrayValue);

                for (int i = 0; i < count; ++i)
                {
                    data["traversal"].append(
                        Json::UInt64(
                            snapshot->getDatabaseId(traversal[i])
                        )
                    );
                }

                onSuccess(std::move(data));
            },

            onError
        );
    }
} // namespace nexa
