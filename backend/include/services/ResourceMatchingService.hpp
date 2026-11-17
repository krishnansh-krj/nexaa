
#pragma once

#include <drogon/drogon.h>

#include <functional>
#include <memory>
#include <string>

#include "utilities/ApiResponse.hpp"

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
    static std::string normalizeType(std::string value);


    // Find available resources matching an emergency request.
    static void findMatches(
        const std::shared_ptr<drogon::orm::DbClient>& db,
        unsigned long long requestId,
        SuccessCallback onSuccess,
        ErrorCallback onError,
        FailureCallback onFailure
    );
};

} // namespace nexa
