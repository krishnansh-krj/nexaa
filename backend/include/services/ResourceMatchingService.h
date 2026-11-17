
#pragma once

#include <drogon/drogon.h>

#include <algorithm>
#include <cctype>
#include <functional>
#include <memory>
#include <string>

#include "utilities/ApiResponse.h"

namespace nexa
{

class ResourceMatchingService
{
public:

    using SuccessCallback =
        std::function<void(const Json::Value&)>;

    using ErrorCallback =
        std::function<void(const drogon::orm::DrogonDbException&)>;

    using FailureCallback =
        std::function<void(const std::string&)>;


    // Normalize resource/request types:
    // - Remove leading and trailing spaces
    // - Convert to uppercase
    static std::string normalizeType(std::string value)
    {
        auto notSpace = [](unsigned char ch)
        {
            return !std::isspace(ch);
        };

        value.erase(
            value.begin(),
            std::find_if(value.begin(), value.end(), notSpace)
        );

        value.erase(
            std::find_if(
                value.rbegin(),
                value.rend(),
                notSpace
            ).base(),
            value.end()
        );

        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char ch)
            {
                return static_cast<char>(std::toupper(ch));
            }
        );

        return value;
    }


    // Find available resources matching an emergency request.
    static void findMatches(
        const std::shared_ptr<drogon::orm::DbClient>& db,
        unsigned long long requestId,
        SuccessCallback onSuccess,
        ErrorCallback onError,
        FailureCallback onFailure
    )
    {
        if (!db)
        {
            onFailure("Database connection is unavailable");
            return;
        }

        if (requestId == 0)
        {
            onFailure("Invalid emergency request ID");
            return;
        }

        // Step 1: Fetch request, incident and existing allocations.
        db->execSqlAsync(
            "SELECT "
            "er.request_id, "
            "er.incident_id, "
            "er.request_type, "
            "er.description, "
            "er.priority, "
            "er.quantity_required, "
            "er.status AS request_status, "
            "i.severity AS incident_severity, "
            "i.status AS incident_status, "
            "COALESCE(a.allocated_units, 0) AS allocated_units, "
            "(er.quantity_required - "
            "COALESCE(a.allocated_units, 0)) AS outstanding_units "
            "FROM emergency_requests er "
            "INNER JOIN incidents i "
            "ON i.incident_id = er.incident_id "
            "LEFT JOIN ("
                "SELECT request_id, "
                "SUM(allocated_units) AS allocated_units "
                "FROM resource_allocations "
                "WHERE status IN "
                "('ASSIGNED', 'DISPATCHED', 'ON_SITE') "
                "GROUP BY request_id"
            ") a ON a.request_id = er.request_id "
            "WHERE er.request_id = ?",

            [db, requestId, onSuccess, onError, onFailure]
            (const drogon::orm::Result& requestResult)
            {
                if (requestResult.empty())
                {
                    onFailure("Emergency request not found");
                    return;
                }

                const auto& request = requestResult[0];

                const unsigned long long incidentId =
                    request["incident_id"].as<unsigned long long>();

                const std::string requestType =
                    normalizeType(
                        request["request_type"].as<std::string>()
                    );

                const std::string requestStatus =
                    request["request_status"].as<std::string>();

                const std::string incidentStatus =
                    request["incident_status"].as<std::string>();

                const unsigned int quantityRequired =
                    request["quantity_required"].as<unsigned int>();

                const unsigned int allocatedUnits =
                    request["allocated_units"].as<unsigned int>();

                const unsigned int outstandingUnits =
                    request["outstanding_units"].as<unsigned int>();


                // Step 2: Validate incident status.
                if (incidentStatus != "ACTIVE")
                {
                    onFailure(
                        "Resources cannot be matched because "
                        "the incident is not active"
                    );
                    return;
                }


                // Step 3: Validate request status.
                if (requestStatus != "PENDING" &&
                    requestStatus != "PROCESSING")
                {
                    onFailure(
                        "Request is not eligible for resource matching"
                    );
                    return;
                }


                // Step 4: Validate outstanding quantity.
                if (outstandingUnits == 0)
                {
                    onFailure(
                        "This emergency request is already fulfilled"
                    );
                    return;
                }


                // Step 5: Find available resources.
                db->execSqlAsync(
                    "SELECT "
                    "resource_id, "
                    "resource_code, "
                    "resource_name, "
                    "resource_type, "
                    "available_units, "
                    "status, "
                    "latitude, "
                    "longitude, "
                    "address "
                    "FROM resources "
                    "WHERE status = 'AVAILABLE' "
                    "AND available_units > 0 "
                    "ORDER BY available_units DESC, resource_id ASC",

                    [requestId,
                    incidentId,
                    requestType,
                    requestStatus,
                    quantityRequired,
                    allocatedUnits,
                    outstandingUnits,
                    onSuccess,
                    onError,
                    onFailure]
                    (const drogon::orm::Result& resourceResult)
                    {
                        Json::Value matchingResources(
                            Json::arrayValue
                        );

                        unsigned long long totalMatchingUnits = 0;

                        // Step 6: Match resource types.
                        for (const auto& resource : resourceResult)
                        {
                            const std::string resourceType =
                                normalizeType(
                                    resource["resource_type"]
                                        .as<std::string>()
                                );

                            if (resourceType != requestType)
                            {
                                continue;
                            }

                            const unsigned int availableUnits =
                                resource["available_units"]
                                    .as<unsigned int>();

                            if (availableUnits == 0)
                            {
                                continue;
                            }

                            Json::Value item;

                            item["resource_id"] =
                                resource["resource_id"]
                                    .as<unsigned long long>();

                            item["resource_code"] =
                                resource["resource_code"]
                                    .as<std::string>();

                            item["resource_name"] =
                                resource["resource_name"]
                                    .as<std::string>();

                            item["resource_type"] =
                                resource["resource_type"]
                                    .as<std::string>();

                            item["available_units"] =
                                availableUnits;

                            item["status"] =
                                resource["status"].as<std::string>();

                            // Nullable latitude.
                            if (!resource["latitude"].isNull())
                            {
                                item["latitude"] =
                                    resource["latitude"].as<double>();
                            }
                            else
                            {
                                item["latitude"] = Json::nullValue;
                            }

                            // Nullable longitude.
                            if (!resource["longitude"].isNull())
                            {
                                item["longitude"] =
                                    resource["longitude"].as<double>();
                            }
                            else
                            {
                                item["longitude"] = Json::nullValue;
                            }

                            // Nullable address.
                            if (!resource["address"].isNull())
                            {
                                item["address"] =
                                    resource["address"].as<std::string>();
                            }
                            else
                            {
                                item["address"] = "";
                            }

                            matchingResources.append(item);

                            totalMatchingUnits += availableUnits;
                        }


                        // Step 7: Prepare response.
                        Json::Value data;

                        data["request_id"] = requestId;

                        data["incident_id"] = incidentId;

                        data["request_type"] = requestType;

                        data["request_status"] = requestStatus;

                        data["quantity_required"] =
                            quantityRequired;

                        data["already_allocated_units"] =
                            allocatedUnits;

                        data["outstanding_units"] =
                            outstandingUnits;

                        data["total_matching_units"] =
                            totalMatchingUnits;

                        data["matching_resource_count"] =
                            static_cast<unsigned int>(
                                matchingResources.size()
                            );

                        data["can_fully_satisfy"] =
                            totalMatchingUnits >= outstandingUnits;

                        data["matching_status"] =
                            matchingResources.empty()
                                ? "NO_MATCHING_RESOURCES"
                                : (
                                    totalMatchingUnits >= outstandingUnits
                                        ? "FULL_MATCH_AVAILABLE"
                                        : "PARTIAL_MATCH_AVAILABLE"
                                  );

                        data["matching_resources"] =
                            matchingResources;


                        // Step 8: Return successful response.
                        onSuccess(data);
                    },

                    // IMPORTANT:
                    // Database errors must use ErrorCallback.
                    [onError]
                    (const drogon::orm::DrogonDbException& exception)
                    {
                        onError(exception);
                    }
                );
            },

            // First query database error callback.
            [onError]
            (const drogon::orm::DrogonDbException& exception)
            {
                onError(exception);
            },

            requestId
        );
    }
};

} // namespace nexa
