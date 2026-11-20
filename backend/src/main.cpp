
#include <drogon/drogon.h>

#include "config/DatabaseConfig.hpp"

#include "models/Incident.hpp"
#include "models/Resource.hpp"
#include "models/EmergencyRequest.hpp"
#include "models/ResourceAllocation.hpp"
#include "models/IncidentEvent.hpp"
#include "models/NetworkNode.hpp"
#include "models/NetworkEdge.hpp"

#include "repositories/BaseRepository.hpp"
#include "controllers/ApiRoutes.hpp"
#include "utilities/ApiResponse.hpp"
#include "utilities/Validation.hpp"

#include "repositories/IncidentRepository.hpp"
#include "repositories/ResourceRepository.hpp"
#include "repositories/RequestRepository.hpp"
#include "repositories/AllocationRepository.hpp"
#include "repositories/IncidentEventRepository.hpp"
#include "repositories/NetworkRepository.hpp"

#include "services/IncidentService.hpp"
#include "services/ResourceService.hpp"
#include "services/RequestService.hpp"
#include "services/AllocationService.hpp"
#include "services/IncidentEventService.hpp"
#include "services/NetworkService.hpp"

#include "data_structures/Graph.hpp"
#include "data_structures/MinHeap.hpp"
#include "algorithms/Dijkstra.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

int main()
{
    // Register the MySQL database connection.
if (!nexa::DatabaseConfig::registerMySql())
{
    return 1;
}
    // Register all NEXA API routes
    nexa::ApiRoutes::registerRoutes();

    // CORS for the React frontend. Origin comes from NEXA_CORS_ORIGIN
    // (default: Vite dev server). Preflight OPTIONS requests are answered
    // before routing; every other response gets the allow-origin header.
    static const std::string corsOrigin = []
    {
        const char* fromEnv = std::getenv("NEXA_CORS_ORIGIN");
        return std::string(fromEnv && *fromEnv ? fromEnv : "http://localhost:5173");
    }();

    drogon::app().registerSyncAdvice(
        [](const drogon::HttpRequestPtr& request) -> drogon::HttpResponsePtr
        {
            if (request->method() != drogon::Options)
                return nullptr;

            auto preflight = drogon::HttpResponse::newHttpResponse();
            preflight->setStatusCode(drogon::k204NoContent);
            preflight->addHeader("Access-Control-Allow-Origin", corsOrigin);
            preflight->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
            preflight->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            preflight->addHeader("Access-Control-Max-Age", "86400");
            return preflight;
        });

    drogon::app().registerPostHandlingAdvice(
        [](const drogon::HttpRequestPtr&, const drogon::HttpResponsePtr& response)
        {
            if (response->getHeader("Access-Control-Allow-Origin").empty())
                response->addHeader("Access-Control-Allow-Origin", corsOrigin);
            response->addHeader("Vary", "Origin");
        });

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

    LOG_INFO << "NEXA Backend starting on port 8080 (CORS origin: " << corsOrigin << ")";

    drogon::app().run();

    return 0;
}
