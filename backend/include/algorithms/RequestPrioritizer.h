
#pragma once

#include "data_structures/PriorityQueue.h"

#include <algorithm>
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

    bool operator>(const PrioritizedRequest& other) const
    {
        if (severityRank != other.severityRank)
            return severityRank > other.severityRank;

        if (priorityRank != other.priorityRank)
            return priorityRank > other.priorityRank;

        if (outstandingUnits != other.outstandingUnits)
            return outstandingUnits > other.outstandingUnits;

        // Earlier request wins equal-priority ties.
        return insertionOrder < other.insertionOrder;
    }
};

class RequestPrioritizer
{
public:
    static int severityRank(const std::string& value)
    {
        if (value == "CRITICAL") return 4;
        if (value == "HIGH") return 3;
        if (value == "MEDIUM") return 2;
        if (value == "LOW") return 1;
        return 0;
    }

    static int priorityRank(const std::string& value)
    {
        if (value == "CRITICAL") return 4;
        if (value == "HIGH") return 3;
        if (value == "MEDIUM") return 2;
        if (value == "LOW") return 1;
        return 0;
    }

    static std::vector<PrioritizedRequest> rank(
        const std::vector<PrioritizedRequest>& requests)
    {
        PriorityQueue<PrioritizedRequest> queue;

        for (const auto& request : requests)
            queue.push(request);

        std::vector<PrioritizedRequest> result;
        result.reserve(requests.size());

        PrioritizedRequest current;

        while (queue.pop(current))
            result.push_back(current);

        return result;
    }
};
}
