
#pragma once

#include <string>

namespace nexa
{
struct IncidentEvent
{
    unsigned long long eventId = 0;
    unsigned long long incidentId = 0;

    std::string eventType;
    std::string description;
    std::string eventTime;
};
}
