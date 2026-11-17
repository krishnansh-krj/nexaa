#include "utilities/ApiResponse.hpp"

namespace nexa
{
drogon::HttpResponsePtr ApiResponse::success(
    const Json::Value& data,
    const std::string& message,
    drogon::HttpStatusCode status)
{
    Json::Value body;
    body["success"] = true;
    body["message"] = message;
    body["data"] = data;

    auto response = drogon::HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(status);
    return response;
}

drogon::HttpResponsePtr ApiResponse::error(
    const std::string& message,
    drogon::HttpStatusCode status,
    const std::string& code)
{
    Json::Value body;
    body["success"] = false;
    body["message"] = message;
    body["error"]["code"] = code;

    auto response = drogon::HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(status);
    return response;
}
}
