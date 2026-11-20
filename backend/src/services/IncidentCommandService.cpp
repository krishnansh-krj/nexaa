#include "services/IncidentCommandService.hpp"
#include "utilities/IncidentLifecycle.hpp"

#include <cstdio>
#include <memory>
#include <utility>

namespace nexa
{

namespace
{

using drogon::orm::DrogonDbException;
using drogon::orm::Result;
using drogon::orm::Transaction;

// Delivers exactly one response per command, whichever path finishes first.
class Outcome
{
public:
    explicit Outcome(IncidentCommandService::Completion done) : done_(std::move(done)) {}

    void succeed(drogon::HttpStatusCode status, const std::string& message, const Json::Value& data)
    {
        if (finished_) return;
        finished_ = true;
        done_(true, status, "", message, data);
    }

    void fail(drogon::HttpStatusCode status, const std::string& code, const std::string& message)
    {
        if (finished_) return;
        finished_ = true;
        done_(false, status, code, message, Json::Value(Json::nullValue));
    }

    // Stored until the commit callback confirms the transaction.
    Json::Value pendingData;
    std::string pendingMessage;
    drogon::HttpStatusCode pendingStatus = drogon::k200OK;

private:
    IncidentCommandService::Completion done_;
    bool finished_ = false;
};

using OutcomePtr = std::shared_ptr<Outcome>;

void dbFailure(const std::shared_ptr<Transaction>& trans, const OutcomePtr& outcome, const DrogonDbException& e)
{
    const std::string what = e.base().what();
    LOG_ERROR << "NEXA incident command failed: " << what;
    trans->rollback();
    if (what.find("Duplicate entry") != std::string::npos)
        outcome->fail(drogon::k409Conflict, "DUPLICATE_INCIDENT_CODE", "Incident code already exists");
    else
        outcome->fail(drogon::k500InternalServerError, "DATABASE_ERROR", "A database operation failed");
}

void beginTransaction(const std::shared_ptr<drogon::orm::DbClient>& db, const OutcomePtr& outcome,
                      std::function<void(const std::shared_ptr<Transaction>&)> body)
{
    if (!db)
    {
        outcome->fail(drogon::k500InternalServerError, "DATABASE_UNAVAILABLE", "Database connection is unavailable");
        return;
    }

    db->newTransactionAsync(
        [outcome, body](const std::shared_ptr<Transaction>& trans)
        {
            if (!trans)
            {
                outcome->fail(drogon::k500InternalServerError, "TRANSACTION_FAILED",
                              "Could not create database transaction");
                return;
            }
            trans->setCommitCallback(
                [outcome](bool committed)
                {
                    if (committed)
                        outcome->succeed(outcome->pendingStatus, outcome->pendingMessage, outcome->pendingData);
                    else
                        outcome->fail(drogon::k500InternalServerError, "COMMIT_FAILED", "Transaction commit failed");
                });
            body(trans);
        });
}

std::string generatedCode(unsigned long long id)
{
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "NEXA-INC-%03llu", id);
    return buffer;
}

} // namespace

bool IncidentCommandService::isAllowedTransition(const std::string& from, const std::string& to)
{
    return isAllowedIncidentTransition(from, to);
}

void IncidentCommandService::create(const std::shared_ptr<drogon::orm::DbClient>& db, const NewIncident& incident,
                                    Completion done)
{
    auto outcome = std::make_shared<Outcome>(std::move(done));
    const NewIncident in = incident;

    beginTransaction(db, outcome, [outcome, in](const std::shared_ptr<Transaction>& trans)
    {
        // Step 3: audit event, then commit on scope exit.
        auto writeEvent = [trans, outcome, in](unsigned long long incidentId, const std::string& code)
        {
            trans->execSqlAsync(
                "INSERT INTO incident_events (incident_id, event_type, description) VALUES (?, ?, ?)",
                [outcome, incidentId, code, in](const Result&)
                {
                    Json::Value data;
                    data["incident_id"] = Json::UInt64(incidentId);
                    data["incident_code"] = code;
                    data["status"] = in.status;
                    outcome->pendingData = data;
                    outcome->pendingMessage = "Incident registered successfully";
                    outcome->pendingStatus = drogon::k201Created;
                },
                [trans, outcome](const DrogonDbException& e) { dbFailure(trans, outcome, e); },
                incidentId, std::string("INCIDENT_REPORTED"), "Incident registered: " + in.title);
        };

        // Step 2: assign the generated code when none was supplied.
        auto assignCode = [trans, outcome, writeEvent](unsigned long long incidentId)
        {
            const std::string code = generatedCode(incidentId);
            trans->execSqlAsync(
                "UPDATE incidents SET incident_code = ? WHERE incident_id = ?",
                [writeEvent, incidentId, code](const Result&) { writeEvent(incidentId, code); },
                [trans, outcome](const DrogonDbException& e) { dbFailure(trans, outcome, e); },
                code, incidentId);
        };

        // Step 1: insert. When no code is supplied, a UUID_SHORT() placeholder
        // keeps the UNIQUE constraint satisfied until the ID is known.
        const bool generate = in.code.empty();
        const std::string initialCode = in.code;
        auto onInserted = [outcome, generate, initialCode, assignCode, writeEvent](const Result& r)
        {
            const unsigned long long id = r.insertId();
            if (generate) assignCode(id); else writeEvent(id, initialCode);
        };
        auto onError = [trans, outcome](const DrogonDbException& e) { dbFailure(trans, outcome, e); };

        if (in.latitude && in.longitude)
        {
            trans->execSqlAsync(
                "INSERT INTO incidents (incident_code, title, description, incident_type, severity, status, "
                "latitude, longitude, address) VALUES "
                "(COALESCE(NULLIF(?, ''), CONCAT('PENDING-', UUID_SHORT())), ?, NULLIF(?, ''), ?, ?, ?, ?, ?, NULLIF(?, ''))",
                onInserted, onError,
                in.code, in.title, in.description, in.type, in.severity, in.status, *in.latitude, *in.longitude, in.address);
        }
        else
        {
            trans->execSqlAsync(
                "INSERT INTO incidents (incident_code, title, description, incident_type, severity, status, address) "
                "VALUES (COALESCE(NULLIF(?, ''), CONCAT('PENDING-', UUID_SHORT())), ?, NULLIF(?, ''), ?, ?, ?, NULLIF(?, ''))",
                onInserted, onError,
                in.code, in.title, in.description, in.type, in.severity, in.status, in.address);
        }
    });
}

void IncidentCommandService::updateStatus(const std::shared_ptr<drogon::orm::DbClient>& db,
                                          unsigned long long incidentId, const std::string& newStatus,
                                          const std::string& note, Completion done)
{
    auto outcome = std::make_shared<Outcome>(std::move(done));

    beginTransaction(db, outcome, [outcome, incidentId, newStatus, note](const std::shared_ptr<Transaction>& trans)
    {
        auto onError = [trans, outcome](const DrogonDbException& e) { dbFailure(trans, outcome, e); };

        // Step 4: update row + audit event.
        auto apply = [trans, outcome, incidentId, newStatus, note, onError](const std::string& oldStatus)
        {
            trans->execSqlAsync(
                "UPDATE incidents SET status = ?, "
                "resolved_at = CASE WHEN ? IN ('RESOLVED', 'CLOSED') THEN COALESCE(resolved_at, NOW()) "
                "WHEN ? = 'ACTIVE' THEN NULL ELSE resolved_at END "
                "WHERE incident_id = ?",
                [trans, outcome, incidentId, newStatus, note, oldStatus, onError](const Result&)
                {
                    std::string description = "Status changed from " + oldStatus + " to " + newStatus;
                    if (!note.empty()) description += ": " + note;
                    trans->execSqlAsync(
                        "INSERT INTO incident_events (incident_id, event_type, description) VALUES (?, ?, ?)",
                        [outcome, incidentId, newStatus, oldStatus](const Result&)
                        {
                            Json::Value data;
                            data["incident_id"] = Json::UInt64(incidentId);
                            data["previous_status"] = oldStatus;
                            data["status"] = newStatus;
                            outcome->pendingData = data;
                            outcome->pendingMessage = "Incident status updated";
                            outcome->pendingStatus = drogon::k200OK;
                        },
                        onError, incidentId, std::string("STATUS_CHANGED"), description);
                },
                onError, newStatus, newStatus, newStatus, incidentId);
        };

        // Step 1-3: lock row, validate transition, block close with open allocations.
        trans->execSqlAsync(
            "SELECT status FROM incidents WHERE incident_id = ? FOR UPDATE",
            [trans, outcome, newStatus, incidentId, apply, onError](const Result& r)
            {
                if (r.empty())
                {
                    trans->rollback();
                    outcome->fail(drogon::k404NotFound, "INCIDENT_NOT_FOUND", "Incident not found");
                    return;
                }
                const std::string oldStatus = r[0]["status"].as<std::string>();
                if (!IncidentCommandService::isAllowedTransition(oldStatus, newStatus))
                {
                    trans->rollback();
                    outcome->fail(drogon::k409Conflict, "INVALID_STATUS_TRANSITION",
                                  "Cannot change incident status from " + oldStatus + " to " + newStatus);
                    return;
                }
                if (newStatus != "RESOLVED" && newStatus != "CLOSED")
                {
                    apply(oldStatus);
                    return;
                }
                trans->execSqlAsync(
                    "SELECT COUNT(*) AS open_allocations FROM resource_allocations "
                    "WHERE incident_id = ? AND status IN ('ASSIGNED', 'DISPATCHED', 'ON_SITE')",
                    [trans, outcome, apply, oldStatus](const Result& c)
                    {
                        if (c[0]["open_allocations"].as<unsigned long long>() > 0)
                        {
                            trans->rollback();
                            outcome->fail(drogon::k409Conflict, "ACTIVE_ALLOCATIONS_EXIST",
                                          "Release all active allocations before resolving or closing the incident");
                            return;
                        }
                        apply(oldStatus);
                    },
                    onError, incidentId);
            },
            onError, incidentId);
    });
}

} // namespace nexa
