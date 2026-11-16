
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
        drogon::HttpStatusCode status = drogon::k200OK)
    {
        Json::Value body;
        body["success"] = true;
        body["message"] = message;
        body["data"] = data;

        auto response = drogon::HttpResponse::newHttpJsonResponse(body);
        response->setStatusCode(status);
        return response;
    }

    static drogon::HttpResponsePtr error(
        const std::string& message,
        drogon::HttpStatusCode status = drogon::k400BadRequest,
        const std::string& code = "BAD_REQUEST")
    {
        Json::Value body;
        body["success"] = false;
        body["message"] = message;
        body["error"]["code"] = code;

        auto response = drogon::HttpResponse::newHttpJsonResponse(body);
        response->setStatusCode(status);
        return response;
    }
};
}
