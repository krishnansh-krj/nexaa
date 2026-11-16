
#pragma once

#include <string>
#include <optional>

namespace nexa
{
struct ResourceAllocation
{
    unsigned long long allocationId = 0;
    unsigned long long incidentId = 0;
    unsigned long long resourceId = 0;

    std::optional<unsigned long long> requestId;

    unsigned int allocatedUnits = 1;
    std::string status = "ASSIGNED";

    std::string assignedAt;
    std::optional<std::string> releasedAt;
};
}
