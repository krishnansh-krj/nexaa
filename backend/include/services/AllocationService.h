
#pragma once

#include <drogon/drogon.h>

#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace nexa
{
class AllocationService
{
public:
    using OperationCallback =
        std::function<void(bool, const std::string&)>;

private:
    std::shared_ptr<drogon::orm::DbClient> dbClient;

public:
    explicit AllocationService(
        std::shared_ptr<drogon::orm::DbClient> db)
        : dbClient(std::move(db))
    {
    }

    void allocate(
        unsigned long long incidentId,
        unsigned long long resourceId,
        unsigned int units,
        OperationCallback callback)
    {
        if (incidentId == 0 || resourceId == 0 || units == 0)
        {
            callback(false, "Invalid allocation details");
            return;
        }

        dbClient->newTransactionAsync(
            [=](const std::shared_ptr<drogon::orm::Transaction>& trans)
            {
                if (!trans)
                {
                    callback(false, "Could not create database transaction");
                    return;
                }

                // All failure paths explicitly notify and roll back.
                auto fail = [trans, callback](const std::string& message)
                {
                    trans->rollback();
                    callback(false, message);
                };

                // Only the commit callback reports a successful allocation.
                trans->setCommitCallback(
                    [callback](bool committed)
                    {
                        callback(
                            committed,
                            committed
                                ? "Resource allocated successfully"
                                : "Allocation commit failed"
                        );
                    });

                // Step 1: Lock resource and check availability.
                trans->execSqlAsync(
                    "SELECT available_units, status FROM resources "
                    "WHERE resource_id = ? FOR UPDATE",

                    [=](const drogon::orm::Result& resourceResult)
                    {
                        if (resourceResult.empty())
                        {
                            fail("Resource not found");
                            return;
                        }

                        const unsigned int available =
                            resourceResult[0]["available_units"]
                                .as<unsigned int>();

                        const std::string resourceStatus =
                            resourceResult[0]["status"].as<std::string>();

                        if (resourceStatus != "AVAILABLE" || available < units)
                        {
                            fail("Resource is unavailable or has insufficient units");
                            return;
                        }

                        // Step 2: Lock and verify incident.
                        trans->execSqlAsync(
                            "SELECT status FROM incidents "
                            "WHERE incident_id = ? FOR UPDATE",

                            [=](const drogon::orm::Result& incidentResult)
                            {
                                if (incidentResult.empty())
                                {
                                    fail("Incident not found");
                                    return;
                                }

                                const std::string incidentStatus =
                                    incidentResult[0]["status"]
                                        .as<std::string>();

                                if (incidentStatus != "ACTIVE")
                                {
                                    fail("Incident must be ACTIVE before allocation");
                                    return;
                                }

                                // Step 3: Insert allocation.
                                trans->execSqlAsync(
                                    "INSERT INTO resource_allocations "
                                    "(incident_id, resource_id, allocated_units) "
                                    "VALUES (?, ?, ?)",

                                    [=](const drogon::orm::Result&)
                                    {
                                        // Step 4: Update resource availability.
                                        trans->execSqlAsync(
                                            "UPDATE resources SET "
                                            "available_units = available_units - ?, "
                                            "status = CASE "
                                            "WHEN available_units - ? = 0 "
                                            "THEN 'DEPLOYED' ELSE 'AVAILABLE' END "
                                            "WHERE resource_id = ?",

                                            [=](const drogon::orm::Result&)
                                            {
                                                // Step 5: Record incident event.
                                                trans->execSqlAsync(
                                                    "INSERT INTO incident_events "
                                                    "(incident_id, event_type, description) "
                                                    "VALUES (?, ?, ?)",

                                                    [=](const drogon::orm::Result&)
                                                    {
                                                        // Transaction commits
                                                        // when its final reference
                                                        // is released.
                                                    },

                                                    [=](const drogon::orm::DrogonDbException& e)
                                                    {
                                                        fail(e.base().what());
                                                    },

                                                    incidentId,
                                                    std::string("RESOURCE_ALLOCATED"),
                                                    std::string("Emergency resource allocated")
                                                );
                                            },

                                            [=](const drogon::orm::DrogonDbException& e)
                                            {
                                                fail(e.base().what());
                                            },

                                            units,
                                            units,
                                            resourceId
                                        );
                                    },

                                    [=](const drogon::orm::DrogonDbException& e)
                                    {
                                        fail(e.base().what());
                                    },

                                    incidentId,
                                    resourceId,
                                    units
                                );
                            },

                            [=](const drogon::orm::DrogonDbException& e)
                            {
                                fail(e.base().what());
                            },

                            incidentId
                        );
                    },

                    [=](const drogon::orm::DrogonDbException& e)
                    {
                        fail(e.base().what());
                    },

                    resourceId
                );
            }
        );
    }
};
}
