#include "algorithms/RequestPrioritizer.hpp"

#include <cassert>
#include <vector>

int main()
{
    nexa::PrioritizedRequest low;
    low.requestId = 1;
    low.severityRank = nexa::RequestPrioritizer::severityRank("LOW");
    low.priorityRank = nexa::RequestPrioritizer::priorityRank("LOW");
    low.outstandingUnits = 1;
    low.insertionOrder = 1;

    nexa::PrioritizedRequest critical;
    critical.requestId = 2;
    critical.severityRank = nexa::RequestPrioritizer::severityRank("CRITICAL");
    critical.priorityRank = nexa::RequestPrioritizer::priorityRank("HIGH");
    critical.outstandingUnits = 3;
    critical.insertionOrder = 2;

    const auto ranked = nexa::RequestPrioritizer::rank({low, critical});

    assert(ranked.size() == 2);
    assert(ranked.front().requestId == 2);
    assert(ranked.back().requestId == 1);
    assert(nexa::RequestPrioritizer::severityRank("UNKNOWN") == 0);
    return 0;
}
