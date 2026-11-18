#include "controllers/ApiRoutes.hpp"
#include "utilities/TypeMapping.hpp"


namespace nexa
{

drogon::HttpResponsePtr ApiRoutes::databaseError(
        const drogon::orm::DrogonDbException& error)
{
        LOG_ERROR << "NEXA Database Error: "
                  << error.base().what();

        return ApiResponse::error(
            "A database operation failed",
            drogon::k500InternalServerError,
            "DATABASE_ERROR"
        );
    }


bool ApiRoutes::parsePositiveId(
    const std::string& value,
    unsigned long long& output)
{
    if (value.empty())
        return false;

    for (char ch : value)
    {
        if (ch < '0' || ch > '9')
            return false;
    }

    try
    {
        size_t processed = 0;
        unsigned long long parsed =
            std::stoull(value, &processed);

        if (processed != value.size() || parsed == 0)
            return false;

        output = parsed;
        return true;
    }
    catch (...)
    {
        return false;
    }
}


Json::Value ApiRoutes::serializeIncident(
        const drogon::orm::Row& row)
{
        Json::Value item;

        item["incident_id"] = Json::UInt64(
            row["incident_id"].as<unsigned long long>());

        item["incident_code"] =
            row["incident_code"].as<std::string>();

        item["title"] =
            row["title"].as<std::string>();

        item["description"] =
            row["description"].as<std::string>();

        item["incident_type"] =
            row["incident_type"].as<std::string>();

        item["severity"] =
            row["severity"].as<std::string>();

        item["status"] =
            row["status"].as<std::string>();

        return item;
    }


Json::Value ApiRoutes::serializeResource(
        const drogon::orm::Row& row)
{
        Json::Value item;

        item["resource_id"] = Json::UInt64(
            row["resource_id"].as<unsigned long long>());

        item["resource_code"] =
            row["resource_code"].as<std::string>();

        item["resource_name"] =
            row["resource_name"].as<std::string>();

        item["resource_type"] =
            row["resource_type"].as<std::string>();

        item["status"] =
            row["status"].as<std::string>();

        item["total_units"] =
            row["total_units"].as<unsigned int>();

        item["available_units"] =
            row["available_units"].as<unsigned int>();

        item["latitude"] = row["latitude"].isNull()
            ? Json::Value(Json::nullValue)
            : Json::Value(row["latitude"].as<double>());

        item["longitude"] = row["longitude"].isNull()
            ? Json::Value(Json::nullValue)
            : Json::Value(row["longitude"].as<double>());

        item["address"] = row["address"].isNull()
            ? Json::Value(Json::nullValue)
            : Json::Value(row["address"].as<std::string>());

        return item;
    }


Json::Value ApiRoutes::serializeRequest(
        const drogon::orm::Row& row)
{
        Json::Value item;

        item["request_id"] = Json::UInt64(
            row["request_id"].as<unsigned long long>());

        item["incident_id"] = Json::UInt64(
            row["incident_id"].as<unsigned long long>());

        item["request_type"] =
            row["request_type"].as<std::string>();

        item["description"] =
            row["description"].as<std::string>();

        item["priority"] =
            row["priority"].as<std::string>();

        item["quantity_required"] =
            row["quantity_required"].as<unsigned int>();

        item["status"] =
            row["status"].as<std::string>();

        return item;
    }


Json::Value ApiRoutes::serializeAllocation(
        const drogon::orm::Row& row)
{
        Json::Value item;

        item["allocation_id"] = Json::UInt64(
            row["allocation_id"].as<unsigned long long>());

        item["incident_id"] = Json::UInt64(
            row["incident_id"].as<unsigned long long>());

        item["resource_id"] = Json::UInt64(
            row["resource_id"].as<unsigned long long>());

        item["allocated_units"] =
            row["allocated_units"].as<unsigned int>();

        item["status"] =
            row["status"].as<std::string>();

        item["request_id"] = row["request_id"].isNull()
            ? Json::Value(Json::nullValue)
            : Json::Value(Json::UInt64(
                row["request_id"].as<unsigned long long>()));

        return item;
    }


Json::Value ApiRoutes::serializeEvent(
        const drogon::orm::Row& row)
{
        Json::Value item;

        item["event_id"] = Json::UInt64(
            row["event_id"].as<unsigned long long>());

        item["incident_id"] = Json::UInt64(
            row["incident_id"].as<unsigned long long>());

        item["event_type"] =
            row["event_type"].as<std::string>();

        item["description"] =
            row["description"].as<std::string>();

        item["event_time"] =
            row["event_time"].as<std::string>();

        return item;
    }


Json::Value ApiRoutes::serializeNode(
        const drogon::orm::Row& row)
{
        Json::Value item;

        item["node_id"] = Json::UInt64(
            row["node_id"].as<unsigned long long>());

        item["node_code"] =
            row["node_code"].as<std::string>();

        item["node_name"] =
            row["node_name"].as<std::string>();

        item["latitude"] =
            row["latitude"].as<double>();

        item["longitude"] =
            row["longitude"].as<double>();

        item["node_type"] =
            row["node_type"].as<std::string>();

        return item;
    }


Json::Value ApiRoutes::serializeEdge(
        const drogon::orm::Row& row)
{
        Json::Value item;

        item["edge_id"] = Json::UInt64(
            row["edge_id"].as<unsigned long long>());

        item["source_node_id"] = Json::UInt64(
            row["source_node_id"].as<unsigned long long>());

        item["destination_node_id"] = Json::UInt64(
            row["destination_node_id"].as<unsigned long long>());

        item["distance_km"] =
            row["distance_km"].as<double>();

        item["travel_time_minutes"] =
            row["travel_time_minutes"].isNull()
                ? Json::Value(Json::nullValue)
                : Json::Value(
                    row["travel_time_minutes"].as<double>());

        item["is_blocked"] =
            row["is_blocked"].as<bool>();

        return item;
    }


void ApiRoutes::registerIncidentRoutes()
{
        // GET /api/incidents

        drogon::app().registerHandler(
            "/api/incidents",

            [](const drogon::HttpRequestPtr&,
               HttpCallback&& callback)
            {
                IncidentRepository repo(
                    drogon::app().getDbClient());

                repo.findAll(
                    [callback](const drogon::orm::Result& result)
                    {
                        Json::Value data(Json::arrayValue);

                        for (const auto& row : result)
                            data.append(serializeIncident(row));

                        callback(ApiResponse::success(
                            data,
                            "Incidents retrieved successfully"));
                    },

                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    }
                );
            },
            {drogon::Get}
        );

        // GET /api/incidents/{id}

        drogon::app().registerHandler(
            "/api/incidents/{1}",

            [](const drogon::HttpRequestPtr&,
               HttpCallback&& callback,
               long long id)
            {
                if (id <= 0)
                {
                    callback(ApiResponse::error(
                        "Invalid incident ID",
                        drogon::k400BadRequest,
                        "INVALID_ID"));
                    return;
                }

                IncidentRepository repo(
                    drogon::app().getDbClient());

                repo.findById(
                    static_cast<unsigned long long>(id),

                    [callback](const drogon::orm::Result& result)
                    {
                        if (result.empty())
                        {
                            callback(ApiResponse::error(
                                "Incident not found",
                                drogon::k404NotFound,
                                "NOT_FOUND"));
                            return;
                        }

                        callback(ApiResponse::success(
                            serializeIncident(result[0]),
                            "Incident retrieved successfully"));
                    },

                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    }
                );
            },
            {drogon::Get}
        );
    }


void ApiRoutes::registerResourceRoutes()
{
        // GET /api/resources

        drogon::app().registerHandler(
            "/api/resources",

            [](const drogon::HttpRequestPtr&,
               HttpCallback&& callback)
            {
                ResourceRepository repo(
                    drogon::app().getDbClient());

                repo.findAll(
                    [callback](const drogon::orm::Result& result)
                    {
                        Json::Value data(Json::arrayValue);

                        for (const auto& row : result)
                            data.append(serializeResource(row));

                        callback(ApiResponse::success(
                            data,
                            "Resources retrieved successfully"));
                    },

                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    }
                );
            },
            {drogon::Get}
        );

        // GET /api/resources/available?type=AMBULANCE
        // Register before /api/resources/{id}.

        drogon::app().registerHandler(
            "/api/resources/available",

            [](const drogon::HttpRequestPtr& request,
               HttpCallback&& callback)
            {
                std::string type =
                    Validation::trim(request->getParameter("type"));

                if (!Validation::isValidEnum(
                    type,
                    {
                        "AMBULANCE",
                        "FIRE_TRUCK",
                        "RESCUE_TEAM",
                        "MEDICAL_TEAM",
                        "SUPPLIES",
                        "SHELTER",
                        "HOSPITAL",
                        "OTHER"
                    }))
                {
                    callback(ApiResponse::error(
                        "Invalid or missing resource type",
                        drogon::k400BadRequest,
                        "INVALID_RESOURCE_TYPE"));
                    return;
                }

                ResourceRepository repo(
                    drogon::app().getDbClient());

                repo.findAvailable(
                    type,

                    [callback](const drogon::orm::Result& result)
                    {
                        Json::Value data(Json::arrayValue);

                        for (const auto& row : result)
                            data.append(serializeResource(row));

                        callback(ApiResponse::success(
                            data,
                            "Available resources retrieved successfully"));
                    },

                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    }
                );
            },
            {drogon::Get}
        );

        // GET /api/resources/{id}

        drogon::app().registerHandler(
            "/api/resources/{1}",

            [](const drogon::HttpRequestPtr&,
               HttpCallback&& callback,
               long long id)
            {
                if (id <= 0)
                {
                    callback(ApiResponse::error(
                        "Invalid resource ID",
                        drogon::k400BadRequest,
                        "INVALID_ID"));
                    return;
                }

                ResourceRepository repo(
                    drogon::app().getDbClient());

                repo.findById(
                    static_cast<unsigned long long>(id),

                    [callback](const drogon::orm::Result& result)
                    {
                        if (result.empty())
                        {
                            callback(ApiResponse::error(
                                "Resource not found",
                                drogon::k404NotFound,
                                "NOT_FOUND"));
                            return;
                        }

                        callback(ApiResponse::success(
                            serializeResource(result[0]),
                            "Resource retrieved successfully"));
                    },

                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    }
                );
            },
            {drogon::Get}
        );

        // POST /api/resources

        drogon::app().registerHandler(
            "/api/resources",

            [](const drogon::HttpRequestPtr& request,
               HttpCallback&& callback)
            {
                auto json = request->getJsonObject();

                if (!json)
                {
                    callback(ApiResponse::error(
                        "Invalid JSON body",
                        drogon::k400BadRequest,
                        "INVALID_JSON"));
                    return;
                }

                // Validate all resource fields before reading them.
                if (!json->isMember("resource_code") ||
                    !(*json)["resource_code"].isString() ||
                    !json->isMember("resource_name") ||
                    !(*json)["resource_name"].isString() ||
                    !json->isMember("resource_type") ||
                    !(*json)["resource_type"].isString())
                {
                    callback(ApiResponse::error(
                        "Resource code, name and type must be strings",
                        drogon::k400BadRequest,
                        "INVALID_RESOURCE_FIELDS"));
                    return;
                }

                if (!json->isMember("total_units") ||
                    !(*json)["total_units"].isUInt())
                {
                    callback(ApiResponse::error(
                        "Total units must be a valid unsigned integer",
                        drogon::k400BadRequest,
                        "INVALID_TOTAL_UNITS"));
                    return;
                }

                std::string code = Validation::trim(
                    (*json)["resource_code"].asString());

                std::string name = Validation::trim(
                    (*json)["resource_name"].asString());

                std::string type = normalizeType(
                    (*json)["resource_type"].asString());

                const unsigned int totalUnits =
                    (*json)["total_units"].asUInt();

                if (totalUnits == 0)
                {
                    callback(ApiResponse::error(
                        "Total units must be greater than zero",
                        drogon::k400BadRequest,
                        "INVALID_TOTAL_UNITS"));
                    return;
                }

                if (code.empty() || name.empty())
                {
                    callback(ApiResponse::error(
                        "Resource code and name are required"));
                    return;
                }

                if (code.size() > 50 || name.size() > 150)
                {
                    callback(ApiResponse::error(
                        "Resource code or name is too long"));
                    return;
                }

                if (!Validation::isValidEnum(
                    type,
                    {
                        "AMBULANCE",
                        "FIRE_TRUCK",
                        "RESCUE_TEAM",
                        "MEDICAL_TEAM",
                        "SUPPLIES",
                        "SHELTER",
                        "HOSPITAL",
                        "OTHER"
                    }))
                {
                    callback(ApiResponse::error(
                        "Invalid resource type"));
                    return;
                }

                ResourceRepository repo(
                    drogon::app().getDbClient());

                repo.create(
                    code,
                    name,
                    type,
                    totalUnits,

                    [callback](const drogon::orm::Result& result)
                    {
                        Json::Value data;

                        data["resource_id"] =
                            Json::UInt64(result.insertId());

                        callback(ApiResponse::success(
                            data,
                            "Resource created successfully",
                            drogon::k201Created));
                    },

                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    }
                );
            },
            {drogon::Post}
        );
    }


void ApiRoutes::registerRequestRoutes()
{
        // GET /api/requests

        drogon::app().registerHandler(
            "/api/requests",

            [](const drogon::HttpRequestPtr&,
               HttpCallback&& callback)
            {
                RequestRepository repo(
                    drogon::app().getDbClient());

                repo.findAll(
                    [callback](const drogon::orm::Result& result)
                    {
                        Json::Value data(Json::arrayValue);

                        for (const auto& row : result)
                            data.append(serializeRequest(row));

                        callback(ApiResponse::success(
                            data,
                            "Emergency requests retrieved successfully"));
                    },

                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    }
                );
            },
            {drogon::Get}
        );


        // GET /api/requests/prioritized
        drogon::app().registerHandler(
            "/api/requests/prioritized",

            [](const drogon::HttpRequestPtr&,
               HttpCallback&& callback)
            {
                RequestPrioritizationService service(
                    drogon::app().getDbClient());

                service.getPrioritized(
                    [callback](Json::Value data)
                    {
                        callback(ApiResponse::success(
                            data,
                            "Emergency requests prioritised successfully"));
                    },
                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    }
                );
            },
            {drogon::Get}
        );

        // GET /api/requests/matching-resources?request_id=1
        drogon::app().registerHandler(
            "/api/requests/matching-resources",

            [](const drogon::HttpRequestPtr& request,
               HttpCallback&& callback)
            {
                unsigned long long requestId = 0;

                if (!parsePositiveId(
                    request->getParameter("request_id"),
                    requestId))
                {
                    callback(ApiResponse::error(
                        "A valid request_id is required",
                        drogon::k400BadRequest,
                        "INVALID_REQUEST_ID"));
                    return;
                }

                ResourceMatchingService::findMatches(
                    drogon::app().getDbClient(),
                    requestId,

                    [callback](Json::Value data)
                    {
                        callback(ApiResponse::success(
                            data,
                            "Matching resources retrieved successfully"));
                    },

                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    },

                    [callback](const std::string& message)
                    {
                        callback(ApiResponse::error(
                        message,
                        drogon::k400BadRequest,
                        "RESOURCE_MATCHING_FAILED"));
                    }
                );
            },
            {drogon::Get}
        );


        // GET /api/requests/{id}

        drogon::app().registerHandler(
            "/api/requests/{1}",

            [](const drogon::HttpRequestPtr&,
               HttpCallback&& callback,
               long long id)
            {
                if (id <= 0)
                {
                    callback(ApiResponse::error(
                        "Invalid request ID",
                        drogon::k400BadRequest,
                        "INVALID_ID"));
                    return;
                }

                RequestRepository repo(
                    drogon::app().getDbClient());

                repo.findById(
                    static_cast<unsigned long long>(id),

                    [callback](const drogon::orm::Result& result)
                    {
                        if (result.empty())
                        {
                            callback(ApiResponse::error(
                                "Request not found",
                                drogon::k404NotFound,
                                "NOT_FOUND"));
                            return;
                        }

                        callback(ApiResponse::success(
                            serializeRequest(result[0])));
                    },

                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    }
                );
            },
            {drogon::Get}
        );

        // GET /api/requests/incident/{id}

        drogon::app().registerHandler(
            "/api/requests/incident/{1}",

            [](const drogon::HttpRequestPtr&,
               HttpCallback&& callback,
               long long id)
            {
                if (id <= 0)
                {
                    callback(ApiResponse::error(
                        "Invalid incident ID"));
                    return;
                }

                RequestRepository repo(
                    drogon::app().getDbClient());

                repo.findByIncident(
                    static_cast<unsigned long long>(id),

                    [callback](const drogon::orm::Result& result)
                    {
                        Json::Value data(Json::arrayValue);

                        for (const auto& row : result)
                            data.append(serializeRequest(row));

                        callback(ApiResponse::success(data));
                    },

                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    }
                );
            },
            {drogon::Get}
        );

        // POST /api/requests

        drogon::app().registerHandler(
            "/api/requests",

            [](const drogon::HttpRequestPtr& request,
               HttpCallback&& callback)
            {
                auto json = request->getJsonObject();

if (!json)
{
    callback(ApiResponse::error("Invalid JSON"));
    return;
}

// 1. Validate request_type
if (!json->isMember("request_type") ||
    !(*json)["request_type"].isString())
{
    callback(ApiResponse::error(
        "A valid request_type string is required",
        drogon::k400BadRequest,
        "INVALID_REQUEST_TYPE"));
    return;
}

                // 2. Validate incident_id and quantity_required.
                // isUInt64/isUInt reject negative values and out-of-range
                // values before conversion.
                if (!json->isMember("incident_id") ||
                    !(*json)["incident_id"].isUInt64() ||
                    !json->isMember("quantity_required") ||
                    !(*json)["quantity_required"].isUInt())
                {
                    callback(ApiResponse::error(
                        "Incident ID must be a positive integer and "
                        "quantity must be a valid unsigned integer",
                        drogon::k400BadRequest,
                        "INVALID_NUMERIC_FIELDS"));
                    return;
                }

                // 3. Validate description and priority.
                if (!json->isMember("description") ||
                    !(*json)["description"].isString() ||
                    !json->isMember("priority") ||
                    !(*json)["priority"].isString())
                {
                    callback(ApiResponse::error(
                        "Description and priority must be valid strings",
                        drogon::k400BadRequest,
                        "INVALID_REQUEST_DETAILS"));
                    return;
                }

                const unsigned long long incidentId =
                    (*json)["incident_id"].asUInt64();

                const unsigned int quantity =
                    (*json)["quantity_required"].asUInt();

                std::string type = normalizeType(
                    (*json)["request_type"].asString());

                std::string description = Validation::trim(
                    (*json)["description"].asString());

                std::string priority = Validation::trim(
                    (*json)["priority"].asString());

                // 4. Apply business validation.
                if (incidentId == 0 ||
                    quantity == 0 ||
                    !isValidRequestType(type) ||
                    description.empty() ||
                    type.size() > 100 ||
                    description.size() > 1000 ||
                    !Validation::isValidEnum(
                        priority,
                        {"LOW", "MEDIUM", "HIGH", "CRITICAL"}))
                {
                    callback(ApiResponse::error(
                        "Invalid emergency request details",
                        drogon::k400BadRequest,
                        "INVALID_REQUEST_DETAILS"));
                    return;
                }

                // 5. Insert only if the incident is ACTIVE.
                RequestRepository repo(
                    drogon::app().getDbClient());

                repo.createForActiveIncident(
                    incidentId,
                    type,
                    description,
                    priority,
                    quantity,

                    [callback](const drogon::orm::Result& result)
                    {
                        // INSERT ... SELECT affects zero rows when the
                        // incident does not exist or is not ACTIVE.
                        if (result.affectedRows() == 0)
                        {
                            callback(ApiResponse::error(
                                "Emergency requests can only be created "
                                "for ACTIVE incidents",
                                drogon::k409Conflict,
                                "INCIDENT_NOT_ACTIVE"));
                            return;
                        }

                        Json::Value data;

                        data["request_id"] =
                            Json::UInt64(result.insertId());

                        callback(ApiResponse::success(
                            data,
                            "Emergency request created successfully",
                            drogon::k201Created));
                    },

                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    }
                );
            },
            {drogon::Post}
        );
    }


void ApiRoutes::registerAllocationRoutes()
{
        // GET /api/allocations

        drogon::app().registerHandler(
            "/api/allocations",

            [](const drogon::HttpRequestPtr&,
               HttpCallback&& callback)
            {
                AllocationRepository repo(
                    drogon::app().getDbClient());

                repo.findAll(
                    [callback](const drogon::orm::Result& result)
                    {
                        Json::Value data(Json::arrayValue);

                        for (const auto& row : result)
                            data.append(serializeAllocation(row));

                        callback(ApiResponse::success(
                            data,
                            "Resource allocations retrieved successfully"));
                    },

                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    }
                );
            },
            {drogon::Get}
        );

        // GET /api/allocations/{id}

        drogon::app().registerHandler(
            "/api/allocations/{1}",

            [](const drogon::HttpRequestPtr&,
               HttpCallback&& callback,
               long long id)
            {
                if (id <= 0)
                {
                    callback(ApiResponse::error(
                        "Invalid allocation ID"));
                    return;
                }

                AllocationRepository repo(
                    drogon::app().getDbClient());

                repo.findById(
                    static_cast<unsigned long long>(id),

                    [callback](const drogon::orm::Result& result)
                    {
                        if (result.empty())
                        {
                            callback(ApiResponse::error(
                                "Allocation not found",
                                drogon::k404NotFound,
                                "NOT_FOUND"));
                            return;
                        }

                        callback(ApiResponse::success(
                            serializeAllocation(result[0])));
                    },

                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    }
                );
            },
            {drogon::Get}
        );

        // GET /api/allocations/incident/{id}

        drogon::app().registerHandler(
            "/api/allocations/incident/{1}",

            [](const drogon::HttpRequestPtr&,
               HttpCallback&& callback,
               long long id)
            {
                if (id <= 0)
                {
                    callback(ApiResponse::error(
                        "Invalid incident ID"));
                    return;
                }

                AllocationRepository repo(
                    drogon::app().getDbClient());

                repo.findByIncident(
                    static_cast<unsigned long long>(id),

                    [callback](const drogon::orm::Result& result)
                    {
                        Json::Value data(Json::arrayValue);

                        for (const auto& row : result)
                            data.append(serializeAllocation(row));

                        callback(ApiResponse::success(data));
                    },

                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    }
                );
            },
            {drogon::Get}
        );

        // GET /api/allocations/resource/{id}

        drogon::app().registerHandler(
            "/api/allocations/resource/{1}",

            [](const drogon::HttpRequestPtr&,
               HttpCallback&& callback,
               long long id)
            {
                if (id <= 0)
                {
                    callback(ApiResponse::error(
                        "Invalid resource ID"));
                    return;
                }

                AllocationRepository repo(
                    drogon::app().getDbClient());

                repo.findByResource(
                    static_cast<unsigned long long>(id),

                    [callback](const drogon::orm::Result& result)
                    {
                        Json::Value data(Json::arrayValue);

                        for (const auto& row : result)
                            data.append(serializeAllocation(row));

                        callback(ApiResponse::success(data));
                    },

                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    }
                );
            },
            {drogon::Get}
        );

        // POST /api/allocations

        drogon::app().registerHandler(
            "/api/allocations",

            [](const drogon::HttpRequestPtr& request,
               HttpCallback&& callback)
            {
                auto json = request->getJsonObject();

                if (!json)
                {
                    callback(ApiResponse::error("Invalid JSON"));
                    return;
                }

                if (!json->isMember("incident_id") ||
    !(*json)["incident_id"].isUInt64() ||
    !json->isMember("resource_id") ||
    !(*json)["resource_id"].isUInt64() ||
    !json->isMember("allocated_units") ||
    !(*json)["allocated_units"].isUInt())
{
    callback(ApiResponse::error(
        "Incident ID, resource ID and allocated units "
        "must be valid unsigned integers",
        drogon::k400BadRequest,
        "INVALID_ALLOCATION_FIELDS"));
    return;
}

const unsigned long long incidentId =
    (*json)["incident_id"].asUInt64();

const unsigned long long resourceId =
    (*json)["resource_id"].asUInt64();

const unsigned int units =
    (*json)["allocated_units"].asUInt();

if (incidentId == 0 || resourceId == 0 || units == 0)
{
    callback(ApiResponse::error(
        "Incident ID, resource ID and allocated units "
        "must be greater than zero",
        drogon::k400BadRequest,
        "INVALID_ALLOCATION_DETAILS"));
    return;
}

                std::optional<unsigned long long> requestId;

                if (json->isMember("request_id") &&
                    !(*json)["request_id"].isNull())
                {
                    if (!(*json)["request_id"].isUInt64())
{
    callback(ApiResponse::error(
        "Invalid request ID",
        drogon::k400BadRequest,
        "INVALID_REQUEST_ID"));
    return;
}

                    auto value =
                        (*json)["request_id"].asUInt64();

                    if (value == 0)
                    {
                        callback(ApiResponse::error(
                            "Invalid request ID"));
                        return;
                    }

                    requestId = value;
                }

                AllocationService service(
                    drogon::app().getDbClient());

                service.allocate(
                    incidentId,
                    resourceId,
                    units,
                    requestId,

                    [callback](bool success,
                               const std::string& message)
                    {
                        if (!success)
                        {
                            callback(ApiResponse::error(
                                message,
                                drogon::k400BadRequest,
                                "ALLOCATION_FAILED"));
                            return;
                        }

                        Json::Value data;
                        data["operation"] = "allocation";

                        callback(ApiResponse::success(
                            data,
                            message,
                            drogon::k201Created));
                    }
                );
            },
            {drogon::Post}
        );

        // POST /api/allocations/{id}/release

        drogon::app().registerHandler(
            "/api/allocations/{1}/release",

            [](const drogon::HttpRequestPtr&,
               HttpCallback&& callback,
               long long id)
            {
                if (id <= 0)
                {
                    callback(ApiResponse::error(
                        "Invalid allocation ID"));
                    return;
                }

                AllocationService service(
                    drogon::app().getDbClient());

                service.releaseAllocation(
                    static_cast<unsigned long long>(id),

                    [callback](bool success,
                               const std::string& message)
                    {
                        if (!success)
                        {
                            callback(ApiResponse::error(
                                message,
                                drogon::k400BadRequest,
                                "RELEASE_FAILED"));
                            return;
                        }

                        Json::Value data;
                        data["operation"] = "release";

                        callback(ApiResponse::success(
                            data,
                            message));
                    }
                );
            },
            {drogon::Post}
        );
    }


void ApiRoutes::registerIncidentEventRoutes()
{
        // GET /api/incidents/{id}/events

        drogon::app().registerHandler(
            "/api/incidents/{1}/events",

            [](const drogon::HttpRequestPtr&,
               HttpCallback&& callback,
               long long id)
            {
                if (id <= 0)
                {
                    callback(ApiResponse::error(
                        "Invalid incident ID"));
                    return;
                }

                IncidentEventRepository repo(
                    drogon::app().getDbClient());

                repo.findByIncident(
                    static_cast<unsigned long long>(id),

                    [callback](const drogon::orm::Result& result)
                    {
                        Json::Value data(Json::arrayValue);

                        for (const auto& row : result)
                            data.append(serializeEvent(row));

                        callback(ApiResponse::success(
                            data,
                            "Incident timeline retrieved successfully"));
                    },

                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    }
                );
            },
            {drogon::Get}
        );

        // POST /api/incidents/{id}/events

        drogon::app().registerHandler(
            "/api/incidents/{1}/events",

            [](const drogon::HttpRequestPtr& request,
               HttpCallback&& callback,
               long long id)
            {
                if (id <= 0)
                {
                    callback(ApiResponse::error(
                        "Invalid incident ID"));
                    return;
                }

                auto json = request->getJsonObject();

                if (!json)
                {
                    callback(ApiResponse::error(
                        "Invalid JSON body"));
                    return;
                }

                if (!json->isMember("event_type") ||
                    !(*json)["event_type"].isString() ||
                    !json->isMember("description") ||
                    !(*json)["description"].isString())
                {
                    callback(ApiResponse::error(
                        "Event type and description must be valid strings",
                        drogon::k400BadRequest,
                        "INVALID_EVENT_FIELDS"));
                    return;
                }

                std::string type = Validation::trim(
                    (*json)["event_type"].asString());

                std::string description = Validation::trim(
                    (*json)["description"].asString());

                if (type.empty() ||
                    description.empty() ||
                    type.size() > 100 ||
                    description.size() > 1000)
                {
                    callback(ApiResponse::error(
                        "Valid event type and description are required"));
                    return;
                }

                IncidentEventRepository repo(
                    drogon::app().getDbClient());

                repo.create(
                    static_cast<unsigned long long>(id),
                    type,
                    description,

                    [callback](const drogon::orm::Result& result)
                    {
                        Json::Value data;

                        data["event_id"] =
                            Json::UInt64(result.insertId());

                        callback(ApiResponse::success(
                            data,
                            "Incident event recorded successfully",
                            drogon::k201Created));
                    },

                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    }
                );
            },
            {drogon::Post}
        );
    }


void ApiRoutes::registerNetworkRoutes()
{
        // GET /api/network/nodes

        drogon::app().registerHandler(
            "/api/network/nodes",

            [](const drogon::HttpRequestPtr&,
               HttpCallback&& callback)
            {
                NetworkRepository repo(
                    drogon::app().getDbClient());

                repo.findAllNodes(
                    [callback](const drogon::orm::Result& result)
                    {
                        Json::Value data(Json::arrayValue);

                        for (const auto& row : result)
                            data.append(serializeNode(row));

                        callback(ApiResponse::success(
                            data,
                            "Network nodes retrieved successfully"));
                    },

                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    }
                );
            },
            {drogon::Get}
        );

        // GET /api/network/nodes/{id}

        drogon::app().registerHandler(
            "/api/network/nodes/{1}",

            [](const drogon::HttpRequestPtr&,
               HttpCallback&& callback,
               long long id)
            {
                if (id <= 0)
                {
                    callback(ApiResponse::error(
                        "Invalid node ID"));
                    return;
                }

                NetworkRepository repo(
                    drogon::app().getDbClient());

                repo.findNodeById(
                    static_cast<unsigned long long>(id),

                    [callback](const drogon::orm::Result& result)
                    {
                        if (result.empty())
                        {
                            callback(ApiResponse::error(
                                "Network node not found",
                                drogon::k404NotFound,
                                "NOT_FOUND"));
                            return;
                        }

                        callback(ApiResponse::success(
                            serializeNode(result[0])));
                    },

                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    }
                );
            },
            {drogon::Get}
        );

        // GET /api/network/edges

        drogon::app().registerHandler(
            "/api/network/edges",

            [](const drogon::HttpRequestPtr&,
               HttpCallback&& callback)
            {
                NetworkRepository repo(
                    drogon::app().getDbClient());

                repo.findAllEdges(
                    [callback](const drogon::orm::Result& result)
                    {
                        Json::Value data(Json::arrayValue);

                        for (const auto& row : result)
                            data.append(serializeEdge(row));

                        callback(ApiResponse::success(
                            data,
                            "Network edges retrieved successfully"));
                    },

                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    }
                );
            },
            {drogon::Get}
        );

        // GET /api/network/nodes/{id}/edges
        // Only outgoing, unblocked edges are returned.

        drogon::app().registerHandler(
            "/api/network/nodes/{1}/edges",

            [](const drogon::HttpRequestPtr&,
               HttpCallback&& callback,
               long long id)
            {
                if (id <= 0)
                {
                    callback(ApiResponse::error(
                        "Invalid node ID"));
                    return;
                }

                NetworkRepository repo(
                    drogon::app().getDbClient());

                repo.findEdgesFromNode(
                    static_cast<unsigned long long>(id),

                    [callback](const drogon::orm::Result& result)
                    {
                        Json::Value data(Json::arrayValue);

                        for (const auto& row : result)
                            data.append(serializeEdge(row));

                        callback(ApiResponse::success(
                            data,
                            "Outgoing network edges retrieved successfully"));
                    },

                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    }
                );
            },
            {drogon::Get}
        );

        // PATCH /api/network/edges/{id}/block
        // Body: { "blocked": true } or { "blocked": false }

        drogon::app().registerHandler(
            "/api/network/edges/{1}/block",

            [](const drogon::HttpRequestPtr& request,
               HttpCallback&& callback,
               long long id)
            {
                if (id <= 0)
                {
                    callback(ApiResponse::error(
                        "Invalid edge ID"));
                    return;
                }

                auto json = request->getJsonObject();

                if (!json ||
                    !json->isMember("blocked") ||
                    !(*json)["blocked"].isBool())
                {
                    callback(ApiResponse::error(
                        "A boolean blocked field is required"));
                    return;
                }

                bool blocked =
                    (*json)["blocked"].asBool();

                NetworkRepository repo(
                    drogon::app().getDbClient());

                repo.setEdgeBlocked(
                    static_cast<unsigned long long>(id),
                    blocked,

                    [callback, blocked](
                        const drogon::orm::Result&)
                    {
                        Json::Value data;
                        data["blocked"] = blocked;

                        callback(ApiResponse::success(
                            data,
                            blocked
                                ? "Network edge blocked successfully"
                                : "Network edge unblocked successfully"));
                    },

                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    }
                );
            },
            {drogon::Patch}
        );

        
        // =====================================================
        // NETWORK INTELLIGENCE: DIJKSTRA SHORTEST ROUTE
        // GET /api/network/route?source=1&destination=4
        // =====================================================

        drogon::app().registerHandler(
            "/api/network/route",

            [](const drogon::HttpRequestPtr& request,
               HttpCallback&& callback)
            {
                unsigned long long source = 0;
                unsigned long long destination = 0;

                if (!parsePositiveId(
                        request->getParameter("source"), source) ||
                    !parsePositiveId(
                        request->getParameter("destination"),
                        destination))
                {
                    callback(ApiResponse::error(
                        "Valid source and destination node IDs are required",
                        drogon::k400BadRequest,
                        "INVALID_NODE_IDS"));
                    return;
                }

                auto dbClient = drogon::app().getDbClient();

                NetworkGraphService::shortestPath(
                    dbClient,
                    source,
                    destination,

                    [callback](Json::Value data)
                    {
                        callback(ApiResponse::success(
                            data,
                            "Shortest route calculated successfully"));
                    },

                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    },

                    [callback](
                        const std::string& message,
                        drogon::HttpStatusCode status,
                        const std::string& code)
                    {
                        callback(ApiResponse::error(
                            message, status, code));
                    }
                );
            },
            {drogon::Get}
        );


        // =====================================================
        // NETWORK INTELLIGENCE: BREADTH-FIRST SEARCH
        // GET /api/network/traverse/bfs?start=1
        // =====================================================

        drogon::app().registerHandler(
            "/api/network/traverse/bfs",

            [](const drogon::HttpRequestPtr& request,
               HttpCallback&& callback)
            {
                unsigned long long start = 0;

                if (!parsePositiveId(
                        request->getParameter("start"), start))
                {
                    callback(ApiResponse::error(
                        "A valid starting node ID is required",
                        drogon::k400BadRequest,
                        "INVALID_START_NODE"));
                    return;
                }

                auto dbClient = drogon::app().getDbClient();

                NetworkGraphService::breadthFirstSearch(
                    dbClient,
                    start,

                    [callback](Json::Value data)
                    {
                        callback(ApiResponse::success(
                            data,
                            "BFS traversal completed successfully"));
                    },

                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    },

                    [callback](
                        const std::string& message,
                        drogon::HttpStatusCode status,
                        const std::string& code)
                    {
                        callback(ApiResponse::error(
                            message, status, code));
                    }
                );
            },
            {drogon::Get}
        );


        // =====================================================
        // NETWORK INTELLIGENCE: DEPTH-FIRST SEARCH
        // GET /api/network/traverse/dfs?start=1
        // =====================================================

        drogon::app().registerHandler(
            "/api/network/traverse/dfs",

            [](const drogon::HttpRequestPtr& request,
               HttpCallback&& callback)
            {
                unsigned long long start = 0;

                if (!parsePositiveId(
                        request->getParameter("start"), start))
                {
                    callback(ApiResponse::error(
                        "A valid starting node ID is required",
                        drogon::k400BadRequest,
                        "INVALID_START_NODE"));
                    return;
                }

                auto dbClient = drogon::app().getDbClient();

                NetworkGraphService::depthFirstSearch(
                    dbClient,
                    start,

                    [callback](Json::Value data)
                    {
                        callback(ApiResponse::success(
                            data,
                            "DFS traversal completed successfully"));
                    },

                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        callback(databaseError(e));
                    },

                    [callback](
                        const std::string& message,
                        drogon::HttpStatusCode status,
                        const std::string& code)
                    {
                        callback(ApiResponse::error(
                            message, status, code));
                    }
                );
            },
            {drogon::Get}
        );

    }


void ApiRoutes::registerRoutes()
{
        registerIncidentRoutes();
        registerResourceRoutes();
        registerRequestRoutes();
        registerAllocationRoutes();
        registerIncidentEventRoutes();
        registerNetworkRoutes();

        LOG_INFO << "All NEXA API routes registered successfully.";
    }

} // namespace nexa
