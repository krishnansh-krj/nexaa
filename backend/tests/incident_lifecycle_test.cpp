#include "utilities/IncidentLifecycle.hpp"

#include <iostream>

int main()
{
    using nexa::isAllowedIncidentTransition;
    int failures = 0;
    auto expect = [&](bool condition, const char* label)
    {
        if (!condition) { ++failures; std::cerr << "FAIL " << label << "\n"; }
    };

    expect(isAllowedIncidentTransition("REPORTED", "VERIFIED"), "reported->verified");
    expect(isAllowedIncidentTransition("REPORTED", "ACTIVE"), "reported->active");
    expect(isAllowedIncidentTransition("VERIFIED", "ACTIVE"), "verified->active");
    expect(isAllowedIncidentTransition("ACTIVE", "RESOLVED"), "active->resolved");
    expect(isAllowedIncidentTransition("RESOLVED", "CLOSED"), "resolved->closed");
    expect(isAllowedIncidentTransition("RESOLVED", "ACTIVE"), "resolved->active (reopen)");
    expect(!isAllowedIncidentTransition("ACTIVE", "REPORTED"), "no going back to reported");
    expect(!isAllowedIncidentTransition("ACTIVE", "CLOSED"), "active must resolve first");
    expect(!isAllowedIncidentTransition("CLOSED", "ACTIVE"), "closed is terminal");
    expect(!isAllowedIncidentTransition("ACTIVE", "ACTIVE"), "no self transition");
    expect(!isAllowedIncidentTransition("UNKNOWN", "ACTIVE"), "unknown source");

    if (failures == 0) std::cout << "All incident lifecycle tests passed.\n";
    return failures == 0 ? 0 : 1;
}
