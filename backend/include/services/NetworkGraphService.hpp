#pragma once
#include "repositories/NetworkRepository.hpp"
#include "data_structures/Graph.hpp"
#include <drogon/drogon.h>
#include <functional>
#include <memory>
#include <string>
namespace nexa {
class NetworkGraphService {
public:
 using SuccessCallback=std::function<void(Json::Value)>;
 using ErrorCallback=std::function<void(const drogon::orm::DrogonDbException&)>;
 using FailureCallback=std::function<void(const std::string&,drogon::HttpStatusCode,const std::string&)>;
private:
 struct GraphSnapshot { int nodeCount=0; std::unique_ptr<unsigned long long[]> databaseIds; std::unique_ptr<Graph> graph; int findIndex(unsigned long long databaseId) const; unsigned long long getDatabaseId(int graphVertex) const; };
 using SnapshotPtr=std::shared_ptr<GraphSnapshot>;
 static void loadGraph(const std::shared_ptr<drogon::orm::DbClient>& dbClient,std::function<void(SnapshotPtr)> onReady,ErrorCallback onError);
public:
 static void shortestPath(const std::shared_ptr<drogon::orm::DbClient>& dbClient,unsigned long long sourceDatabaseId,unsigned long long destinationDatabaseId,SuccessCallback onSuccess,ErrorCallback onError,FailureCallback onFailure);
 static void breadthFirstSearch(const std::shared_ptr<drogon::orm::DbClient>& dbClient,unsigned long long startDatabaseId,SuccessCallback onSuccess,ErrorCallback onError,FailureCallback onFailure);
 static void depthFirstSearch(const std::shared_ptr<drogon::orm::DbClient>& dbClient,unsigned long long startDatabaseId,SuccessCallback onSuccess,ErrorCallback onError,FailureCallback onFailure);
};
}
