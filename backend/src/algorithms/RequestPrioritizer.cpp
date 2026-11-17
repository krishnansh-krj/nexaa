#include "algorithms/RequestPrioritizer.hpp"

namespace nexa
{
bool PrioritizedRequest::operator>(const PrioritizedRequest& other) const
{
    if (severityRank != other.severityRank)
        return severityRank > other.severityRank;
    if (priorityRank != other.priorityRank)
        return priorityRank > other.priorityRank;
    if (outstandingUnits != other.outstandingUnits)
        return outstandingUnits > other.outstandingUnits;
    return insertionOrder < other.insertionOrder;
}

int RequestPrioritizer::severityRank(const std::string& value)
{
    if (value == "CRITICAL") return 4;
    if (value == "HIGH") return 3;
    if (value == "MEDIUM") return 2;
    if (value == "LOW") return 1;
    return 0;
}

int RequestPrioritizer::priorityRank(const std::string& value)
{
    if (value == "CRITICAL") return 4;
    if (value == "HIGH") return 3;
    if (value == "MEDIUM") return 2;
    if (value == "LOW") return 1;
    return 0;
}

std::vector<PrioritizedRequest> RequestPrioritizer::rank(
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
}
