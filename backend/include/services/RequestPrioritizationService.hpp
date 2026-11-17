
#pragma once

#include "repositories/RequestRepository.hpp"
#include "algorithms/RequestPrioritizer.hpp"

#include <drogon/drogon.h>
#include <functional>
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
        std::shared_ptr<drogon::orm::DbClient> db);

    void getPrioritized(SuccessCallback onSuccess, ErrorCallback onError);

private:
    RequestRepository repository;
};
}
