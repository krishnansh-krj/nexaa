#pragma once

// Write operations for incidents (registration and lifecycle), each run in a
// single database transaction together with its incident_events audit row.

#include <drogon/drogon.h>

#include <functional>
#include <optional>
#include <string>

namespace nexa
{

struct NewIncident
{
    std::string code;        // optional; generated as NEXA-INC-### when empty
    std::string title;
    std::string description;
    std::string type;        // normalised, e.g. FIRE
    std::string severity;    // LOW | MEDIUM | HIGH | CRITICAL
    std::string status;      // REPORTED | VERIFIED | ACTIVE
    std::optional<double> latitude;
    std::optional<double> longitude;
    std::string address;
};

class IncidentCommandService
{
public:
    // (success, httpStatus, errorCode, message, data)
    using Completion = std::function<void(bool, drogon::HttpStatusCode, const std::string&,
                                          const std::string&, const Json::Value&)>;

    static void create(const std::shared_ptr<drogon::orm::DbClient>& db, const NewIncident& incident,
                       Completion done);

    static void updateStatus(const std::shared_ptr<drogon::orm::DbClient>& db, unsigned long long incidentId,
                             const std::string& newStatus, const std::string& note, Completion done);

    // Lifecycle rule used by updateStatus (exposed for unit tests).
    static bool isAllowedTransition(const std::string& from, const std::string& to);
};

} // namespace nexa
