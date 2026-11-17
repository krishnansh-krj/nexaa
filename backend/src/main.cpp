
#include <drogon/drogon.h>

#include "config/DatabaseConfig.h"

#include "models/Incident.h"
#include "models/Resource.h"
#include "models/EmergencyRequest.h"
#include "models/ResourceAllocation.h"
#include "models/IncidentEvent.h"
#include "models/NetworkNode.h"
#include "models/NetworkEdge.h"

#include "repositories/BaseRepository.h"
#include "controllers/ApiRoutes.h"
#include "utilities/ApiResponse.h"
#include "utilities/Validation.h"

#include "repositories/IncidentRepository.h"
#include "repositories/ResourceRepository.h"
#include "repositories/RequestRepository.h"
#include "repositories/AllocationRepository.h"
#include "repositories/IncidentEventRepository.h"
#include "repositories/NetworkRepository.h"

#include "controllers/ApiRoutes.h"

#include "services/IncidentService.h"
#include "services/ResourceService.h"
#include "services/RequestService.h"
#include "services/AllocationService.h"
#include "services/IncidentEventService.h"
#include "services/NetworkService.h"

#include "data_structures/Graph.h"
#include "data_structures/MinHeap.h"
#include "algorithms/Dijkstra.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

int main()
{
    const char* password = std::getenv("NEXA_DB_PASSWORD");

    if (password == nullptr || std::string(password).empty())
    {
        std::cerr << "ERROR: NEXA_DB_PASSWORD environment variable is not set.\n";
        return 1;
    }

    // Register the MySQL database connection.
    drogon::app().createDbClient(
        "mysql",
        "127.0.0.1",
        3306,
        "nexa_db",
        "nexa_app",
        password,
        2
    );

    // Register all NEXA API routes
    nexa::ApiRoutes::registerRoutes();

    // API 1: Backend health check
    drogon::app().registerHandler(
        "/api/health",
        [](const drogon::HttpRequestPtr&,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback)
        {
            Json::Value response;
            response["status"] = "online";
            response["project"] = "NEXA";
            response["backend"] = "Drogon";
            response["version"] = "1.0.0";

            callback(drogon::HttpResponse::newHttpJsonResponse(response));
        },
        {drogon::Get}
    );


    
    // API 2: MySQL database health check
    drogon::app().registerHandler(
        "/api/db/health",
        [](const drogon::HttpRequestPtr&,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback)
        {
            auto dbClient = drogon::app().getDbClient();

            dbClient->execSqlAsync(
                "SELECT DATABASE() AS database_name, "
                "VERSION() AS server_version",

                [callback](const drogon::orm::Result& result)
                {
                    Json::Value response;
                    response["status"] = "connected";
                    response["database"] =
                        result[0]["database_name"].as<std::string>();
                    response["mysql_version"] =
                        result[0]["server_version"].as<std::string>();

                    callback(
                        drogon::HttpResponse::newHttpJsonResponse(response)
                    );
                },

                [callback](const drogon::orm::DrogonDbException& error)
                {
                    Json::Value response;
                    response["status"] = "database_error";
                    response["message"] = error.base().what();

                    auto httpResponse =
                        drogon::HttpResponse::newHttpJsonResponse(response);

                    httpResponse->setStatusCode(
                        drogon::k500InternalServerError
                    );

                    callback(httpResponse);
                }
            );
        },
        {drogon::Get}
    );



    // Start server
    drogon::app().addListener("127.0.0.1", 8080);

    LOG_INFO << "NEXA Backend starting on port 8080";

    drogon::app().run();

    return 0;
}
