
#pragma once

namespace nexa
{
struct NetworkEdge
{
    unsigned long long edgeId = 0;
    unsigned long long sourceNodeId = 0;
    unsigned long long destinationNodeId = 0;

    double distanceKm = 0.0;
    double travelTimeMinutes = 0.0;

    bool isBlocked = false;
};
}
