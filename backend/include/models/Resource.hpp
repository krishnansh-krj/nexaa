
#pragma once

#include <string>
#include <optional>

namespace nexa
{
struct Resource
{
    unsigned long long resourceId = 0;

    std::string resourceCode;
    std::string resourceName;
    std::string resourceType;
    std::string status = "AVAILABLE";

    unsigned int totalUnits = 1;
    unsigned int availableUnits = 1;

    std::optional<double> latitude;
    std::optional<double> longitude;
    std::optional<std::string> address;

    std::string lastUpdated;
    std::string createdAt;
};
}
