#pragma once

// Incident status lifecycle:
//   REPORTED -> VERIFIED | ACTIVE | CLOSED (false alarm)
//   VERIFIED -> ACTIVE | RESOLVED | CLOSED
//   ACTIVE   -> RESOLVED
//   RESOLVED -> CLOSED | ACTIVE (re-opened)
//   CLOSED   -> (terminal)

#include <string>

namespace nexa
{

inline bool isAllowedIncidentTransition(const std::string& from, const std::string& to)
{
    if (from == to) return false;
    if (from == "REPORTED") return to == "VERIFIED" || to == "ACTIVE" || to == "CLOSED";
    if (from == "VERIFIED") return to == "ACTIVE" || to == "RESOLVED" || to == "CLOSED";
    if (from == "ACTIVE") return to == "RESOLVED";
    if (from == "RESOLVED") return to == "CLOSED" || to == "ACTIVE";
    return false;
}

} // namespace nexa
