#pragma once

#include <drogon/drogon.h>
#include <json/json.h>
#include <string>

namespace nexa
{
class ApiResponse
{
public:
    static drogon::HttpResponsePtr success(
        const Json::Value& data,
        const std::string& message = "Request successful",
        drogon::HttpStatusCode status = drogon::k200OK);

    static drogon::HttpResponsePtr error(
        const std::string& message,
        drogon::HttpStatusCode status = drogon::k400BadRequest,
        const std::string& code = "BAD_REQUEST");
};
}
