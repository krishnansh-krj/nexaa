
#pragma once

#include "repositories/RequestRepository.h"
#include "algorithms/RequestPrioritizer.h"

#include <drogon/drogon.h>
#include <memory>
#include <vector>

namespace nexa
{
class RequestPrioritizationService
{
public:
    using SuccessCallback =
        std::function<void(Json::Value)>;

    using ErrorCallback =
        std::function<void(const drogon::orm::DrogonDbException&)>;

    explicit RequestPrioritizationService(
        std::shared_ptr<drogon::orm::DbClient> db)
        : repository(std::move(db))
    {
    }

    void getPrioritized(
        SuccessCallback onSuccess,
        ErrorCallback onError)
    {
        repository.findPrioritizable(
            [onSuccess](const drogon::orm::Result& rows)
            {
                std::vector<PrioritizedRequest> requests;
                requests.reserve(rows.size());

                unsigned long long order = 0;

                for (const auto& row : rows)
                {
                    PrioritizedRequest item;

                    item.requestId =
                        row["request_id"].as<unsigned long long>();
                    item.incidentId =
                        row["incident_id"].as<unsigned long long>();
                    item.requestType =
                        row["request_type"].as<std::string>();
                    item.description =
                        row["description"].as<std::string>();
                    item.requestPriority =
                        row["request_priority"].as<std::string>();
                    item.incidentSeverity =
                        row["incident_severity"].as<std::string>();
                    item.quantityRequired =
                        row["quantity_required"].as<unsigned int>();
                    item.allocatedUnits =
                        row["allocated_units"].as<unsigned int>();
                    item.outstandingUnits =
                        row["outstanding_units"].as<unsigned int>();
                    item.requestedAt =
                        row["requested_at"].as<std::string>();

                    item.severityRank =
                        RequestPrioritizer::severityRank(
                            item.incidentSeverity);

                    item.priorityRank =
                        RequestPrioritizer::priorityRank(
                            item.requestPriority);

                    item.insertionOrder = order++;

                    requests.push_back(std::move(item));
                }

                const auto ranked =
                    RequestPrioritizer::rank(requests);

                Json::Value data(Json::arrayValue);

                unsigned int position = 1;

                for (const auto& request : ranked)
                {
                    Json::Value item;

                    item["rank"] = position++;
                    item["request_id"] =
                        Json::UInt64(request.requestId);
                    item["incident_id"] =
                        Json::UInt64(request.incidentId);
                    item["request_type"] = request.requestType;
                    item["description"] = request.description;
                    item["request_priority"] =
                        request.requestPriority;
                    item["incident_severity"] =
                        request.incidentSeverity;
                    item["quantity_required"] =
                        request.quantityRequired;
                    item["allocated_units"] =
                        request.allocatedUnits;
                    item["outstanding_units"] =
                        request.outstandingUnits;
                    item["requested_at"] = request.requestedAt;

                    data.append(item);
                }

                onSuccess(std::move(data));
            },
            std::move(onError)
        );
    }

private:
    RequestRepository repository;
};
}
