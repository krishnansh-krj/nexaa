
#pragma once

#include <string>

namespace nexa
{
struct EmergencyRequest
{
    unsigned long long requestId = 0;
    unsigned long long incidentId = 0;

    std::string requestType;
    std::string description;
    std::string priority = "MEDIUM";

    unsigned int quantityRequired = 1;
    std::string status = "PENDING";

    std::string requestedAt;
};
}
