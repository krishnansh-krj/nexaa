#pragma once

#include "data_structures/PriorityQueue.hpp"

#include <string>
#include <vector>

namespace nexa
{
struct PrioritizedRequest
{
    unsigned long long requestId = 0;
    unsigned long long incidentId = 0;
    std::string requestType;
    std::string description;
    std::string requestPriority;
    std::string incidentSeverity;
    std::string requestedAt;
    unsigned int quantityRequired = 0;
    unsigned int allocatedUnits = 0;
    unsigned int outstandingUnits = 0;
    int severityRank = 0;
    int priorityRank = 0;
    unsigned long long insertionOrder = 0;

    bool operator>(const PrioritizedRequest& other) const;
};

class RequestPrioritizer
{
public:
    static int severityRank(const std::string& value);
    static int priorityRank(const std::string& value);
    static std::vector<PrioritizedRequest> rank(
        const std::vector<PrioritizedRequest>& requests);
};
}
