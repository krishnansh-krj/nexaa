
#pragma once

#include <string>
#include <optional>

namespace nexa
{
struct Incident
{
    unsigned long long incidentId = 0;

    std::string incidentCode;
    std::string title;
    std::string description;
    std::string incidentType;
    std::string severity = "MEDIUM";
    std::string status = "REPORTED";

    std::optional<double> latitude;
    std::optional<double> longitude;
    std::optional<std::string> address;

    std::string reportedAt;
    std::optional<std::string> resolvedAt;
    std::string createdAt;
    std::string updatedAt;
};
}
