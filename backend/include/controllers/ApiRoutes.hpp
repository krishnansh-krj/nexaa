
#pragma once

#include <drogon/drogon.h>

#include "repositories/IncidentRepository.hpp"
#include "repositories/ResourceRepository.hpp"
#include "repositories/RequestRepository.hpp"
#include "repositories/AllocationRepository.hpp"
#include "repositories/IncidentEventRepository.hpp"
#include "repositories/NetworkRepository.hpp"

#include "services/AllocationService.hpp"
#include "services/NetworkGraphService.hpp"
#include "utilities/ApiResponse.hpp"
#include "utilities/Validation.hpp"

#include "services/RequestPrioritizationService.hpp"
#include "services/ResourceMatchingService.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace nexa
{

class ApiRoutes
{
private:

    using HttpCallback =
        std::function<void(const drogon::HttpResponsePtr&)>;

    // =========================================================
    // COMMON DATABASE ERROR HANDLER
    // =========================================================

    static drogon::HttpResponsePtr databaseError(
        const drogon::orm::DrogonDbException& error);

    static bool parsePositiveId(
    const std::string& value,
    unsigned long long& output);

    // =========================================================
    // JSON SERIALIZERS
    // =========================================================

    static Json::Value serializeIncident(
        const drogon::orm::Row& row);

    static Json::Value serializeResource(
        const drogon::orm::Row& row);

    static Json::Value serializeRequest(
        const drogon::orm::Row& row);

    static Json::Value serializeAllocation(
        const drogon::orm::Row& row);

    static Json::Value serializeEvent(
        const drogon::orm::Row& row);

    static Json::Value serializeNode(
        const drogon::orm::Row& row);

    static Json::Value serializeEdge(
        const drogon::orm::Row& row);

    // =========================================================
    // INCIDENT ROUTES
    // =========================================================

    static void registerIncidentRoutes();

    // =========================================================
    // RESOURCE ROUTES
    // =========================================================

    static void registerResourceRoutes();

    // =========================================================
    // EMERGENCY REQUEST ROUTES
    // =========================================================

    static void registerRequestRoutes();

    // =========================================================
    // RESOURCE ALLOCATION ROUTES
    // =========================================================

    static void registerAllocationRoutes();

    // =========================================================
    // INCIDENT EVENT ROUTES
    // =========================================================

    static void registerIncidentEventRoutes();

    // =========================================================
    // NETWORK MANAGEMENT ROUTES
    // =========================================================

    static void registerNetworkRoutes();

public:

    // =========================================================
    // REGISTER ALL NEXA API ROUTES
    // =========================================================

    static void registerRoutes();

};

}
