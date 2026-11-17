#include "services/AllocationService.hpp"
#include <algorithm>
#include <cctype>
#include <utility>
namespace nexa {
AllocationService::CompletionState::CompletionState(OperationCallback cb) : callback(std::move(cb)) {}
void AllocationService::CompletionState::finish(bool success, const std::string& message) { if (!completed.exchange(true) && callback) callback(success, message); }
std::string AllocationService::normalizeType(std::string value)
{
        auto notSpace = [](unsigned char ch)
        {
            return !std::isspace(ch);
        };

        value.erase(
            value.begin(),
            std::find_if(value.begin(), value.end(), notSpace)
        );

        value.erase(
            std::find_if(value.rbegin(), value.rend(), notSpace).base(),
            value.end()
        );

        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char ch)
            {
                return static_cast<char>(std::toupper(ch));
            }
        );

        return value;
    }

void AllocationService::refreshRequestStatus(
        const std::shared_ptr<drogon::orm::Transaction>& trans,
        unsigned long long requestId,
        std::function<void()> onSuccess,
        std::function<void(const std::string&)> onFailure)
{
        trans->execSqlAsync(
            "SELECT quantity_required "
            "FROM emergency_requests "
            "WHERE request_id = ? FOR UPDATE",

            [=](const drogon::orm::Result& requestResult)
            {
                if (requestResult.empty())
                {
                    if (onFailure)
                        onFailure("Linked emergency request not found");
                    return;
                }

                const unsigned int quantityRequired =
                    requestResult[0]["quantity_required"]
                        .as<unsigned int>();

                if (quantityRequired == 0)
                {
                    if (onFailure)
                        onFailure(
                            "Emergency request has invalid required quantity"
                        );
                    return;
                }

                trans->execSqlAsync(
                    "SELECT COALESCE(SUM(allocated_units), 0) "
                    "AS active_units "
                    "FROM resource_allocations "
                    "WHERE request_id = ? "
                    "AND status IN ('ASSIGNED', 'DISPATCHED', 'ON_SITE')",

                    [=](const drogon::orm::Result& sumResult)
                    {
                        const unsigned long long activeUnits =
                            sumResult[0]["active_units"]
                                .as<unsigned long long>();

                        const std::string newStatus =
                            activeUnits >= quantityRequired
                                ? "FULFILLED"
                                : (activeUnits > 0
                                    ? "PROCESSING"
                                    : "PENDING");

                        trans->execSqlAsync(
                            "UPDATE emergency_requests "
                            "SET status = ? WHERE request_id = ?",

                            [onSuccess](const drogon::orm::Result&)
                            {
                                if (onSuccess)
                                    onSuccess();
                            },

                            [onFailure](
                                const drogon::orm::DrogonDbException& e)
                            {
                                if (onFailure)
                                    onFailure(e.base().what());
                            },

                            newStatus,
                            requestId
                        );
                    },

                    [onFailure](
                        const drogon::orm::DrogonDbException& e)
                    {
                        if (onFailure)
                            onFailure(e.base().what());
                    },

                    requestId
                );
            },

            [onFailure](
                const drogon::orm::DrogonDbException& e)
            {
                if (onFailure)
                    onFailure(e.base().what());
            },

            requestId
        );
    }

    AllocationService::AllocationService(
        std::shared_ptr<drogon::orm::DbClient> db)
        : dbClient(std::move(db))
{
    }

    void AllocationService::allocate(
        unsigned long long incidentId,
        unsigned long long resourceId,
        unsigned int units,
        std::optional<unsigned long long> requestId,
        OperationCallback callback)
{
        if (!callback)
            return;

        if (!dbClient)
        {
            callback(false, "Database client is unavailable");
            return;
        }

        if (incidentId == 0 || resourceId == 0 || units == 0)
        {
            callback(false, "Invalid allocation details");
            return;
        }

        if (requestId.has_value() && requestId.value() == 0)
        {
            callback(false, "Invalid emergency request ID");
            return;
        }

        auto state =
            std::make_shared<CompletionState>(std::move(callback));

        dbClient->newTransactionAsync(
            [=](const std::shared_ptr<drogon::orm::Transaction>& trans)
            {
                if (!trans)
                {
                    state->finish(
                        false,
                        "Could not create database transaction"
                    );
                    return;
                }

                auto fail = [trans, state](
                    const std::string& message)
                {
                    trans->rollback();
                    state->finish(false, message);
                };

                trans->setCommitCallback(
                    [state](bool committed)
                    {
                        state->finish(
                            committed,
                            committed
                                ? "Resource allocated successfully"
                                : "Allocation commit failed"
                        );
                    }
                );

                // STEP 1: Lock and validate resource.
                trans->execSqlAsync(
                    "SELECT available_units, status, resource_type "
                    "FROM resources "
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
                            resourceResult[0]["status"]
                                .as<std::string>();

                        const std::string resourceType =
                            resourceResult[0]["resource_type"]
                                .as<std::string>();

                        if (resourceStatus != "AVAILABLE" ||
                            available < units)
                        {
                            fail(
                                "Resource is unavailable or has "
                                "insufficient units"
                            );
                            return;
                        }

                        // STEP 2: Lock and validate incident.
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
                                    fail(
                                        "Incident must be ACTIVE "
                                        "before allocation"
                                    );
                                    return;
                                }

                                // STEP 3: Insert allocation.
                                auto insertAllocation = [=]()
                                {
                                    auto onInserted =
                                        [=](const drogon::orm::Result&)
                                    {
                                        // STEP 4: Decrease inventory.
                                        trans->execSqlAsync(
                                            "UPDATE resources SET "
                                            "available_units = "
                                            "available_units - ?, "
                                            "status = CASE "
                                            "WHEN available_units = ? "
                                            "THEN 'DEPLOYED' "
                                            "ELSE 'AVAILABLE' END "
                                            "WHERE resource_id = ? "
                                            "AND available_units >= ?",

                                            [=](const drogon::orm::Result& updateResult)
                                            {
                                                if (updateResult.affectedRows() != 1)
                                                {
                                                    fail(
                                                        "Resource inventory "
                                                        "update failed"
                                                    );
                                                    return;
                                                }

                                                // STEP 5: Record incident event.
                                                trans->execSqlAsync(
                                                    "INSERT INTO incident_events "
                                                    "(incident_id, event_type, description) "
                                                    "VALUES (?, ?, ?)",

                                                    [=](const drogon::orm::Result&)
                                                    {
                                                        if (!requestId.has_value())
                                                            return;

                                                        refreshRequestStatus(
                                                            trans,
                                                            requestId.value(),
                                                            []() {},
                                                            [=](const std::string& message)
                                                            {
                                                                fail(message);
                                                            }
                                                        );
                                                    },

                                                    [=](const drogon::orm::DrogonDbException& e)
                                                    {
                                                        fail(e.base().what());
                                                    },

                                                    incidentId,
                                                    std::string("RESOURCE_ALLOCATED"),
                                                    std::string(
                                                        "Emergency resource allocated"
                                                    )
                                                );
                                            },

                                            [=](const drogon::orm::DrogonDbException& e)
                                            {
                                                fail(e.base().what());
                                            },

                                            units,
                                            units,
                                            resourceId,
                                            units
                                        );
                                    };

                                    auto onInsertError =
                                        [=](const drogon::orm::DrogonDbException& e)
                                    {
                                        fail(e.base().what());
                                    };

                                    if (requestId.has_value())
                                    {
                                        trans->execSqlAsync(
                                            "INSERT INTO resource_allocations "
                                            "(incident_id, resource_id, "
                                            "request_id, allocated_units) "
                                            "VALUES (?, ?, ?, ?)",

                                            onInserted,
                                            onInsertError,

                                            incidentId,
                                            resourceId,
                                            requestId.value(),
                                            units
                                        );
                                    }
                                    else
                                    {
                                        trans->execSqlAsync(
                                            "INSERT INTO resource_allocations "
                                            "(incident_id, resource_id, "
                                            "request_id, allocated_units) "
                                            "VALUES (?, ?, NULL, ?)",

                                            onInserted,
                                            onInsertError,

                                            incidentId,
                                            resourceId,
                                            units
                                        );
                                    }
                                };

                                // Direct allocation without request.
                                if (!requestId.has_value())
                                {
                                    insertAllocation();
                                    return;
                                }

                                // STEP 3A: Lock emergency request.
                                trans->execSqlAsync(
                                    "SELECT status, quantity_required, request_type "
                                    "FROM emergency_requests "
                                    "WHERE request_id = ? "
                                    "AND incident_id = ? FOR UPDATE",

                                    [=](const drogon::orm::Result& requestResult)
                                    {
                                        if (requestResult.empty())
                                        {
                                            fail(
                                                "Emergency request not found "
                                                "for this incident"
                                            );
                                            return;
                                        }

                                        const std::string requestStatus =
                                            requestResult[0]["status"]
                                                .as<std::string>();

                                        const unsigned int quantityRequired =
                                            requestResult[0]["quantity_required"]
                                                .as<unsigned int>();

                                        const std::string requestType =
                                            requestResult[0]["request_type"]
                                                .as<std::string>();

                                        // STEP 3B: Validate resource type.
                                        if (normalizeType(requestType) !=
                                            normalizeType(resourceType))
                                        {
                                            fail(
                                                "Resource type does not match "
                                                "the emergency request type"
                                            );
                                            return;
                                        }

                                        if (requestStatus != "PENDING" &&
                                            requestStatus != "PROCESSING")
                                        {
                                            fail(
                                                "Emergency request is not "
                                                "available for allocation"
                                            );
                                            return;
                                        }

                                        if (quantityRequired == 0)
                                        {
                                            fail(
                                                "Emergency request has "
                                                "invalid required quantity"
                                            );
                                            return;
                                        }

                                        // STEP 3C: Calculate existing active allocations.
                                        trans->execSqlAsync(
                                            "SELECT COALESCE(SUM(allocated_units), 0) "
                                            "AS allocated_units "
                                            "FROM resource_allocations "
                                            "WHERE request_id = ? "
                                            "AND status IN "
                                            "('ASSIGNED', 'DISPATCHED', 'ON_SITE')",

                                            [=](const drogon::orm::Result& sumResult)
                                            {
                                                const unsigned long long alreadyAllocated =
                                                    sumResult[0]["allocated_units"]
                                                        .as<unsigned long long>();

                                                // STEP 3D: Prevent over-allocation.
                                                if (alreadyAllocated >= quantityRequired ||
                                                    units > quantityRequired - alreadyAllocated)
                                                {
                                                    fail(
                                                        "Allocation exceeds the remaining "
                                                        "quantity required by this "
                                                        "emergency request"
                                                    );
                                                    return;
                                                }

                                                // STEP 3E: Mark request as processing.
                                                trans->execSqlAsync(
                                                    "UPDATE emergency_requests "
                                                    "SET status = 'PROCESSING' "
                                                    "WHERE request_id = ? "
                                                    "AND incident_id = ?",

                                                    [=](const drogon::orm::Result&)
                                                    {
                                                        insertAllocation();
                                                    },

                                                    [=](const drogon::orm::DrogonDbException& e)
                                                    {
                                                        fail(e.base().what());
                                                    },

                                                    requestId.value(),
                                                    incidentId
                                                );
                                            },

                                            [=](const drogon::orm::DrogonDbException& e)
                                            {
                                                fail(e.base().what());
                                            },

                                            requestId.value()
                                        );
                                    },

                                    [=](const drogon::orm::DrogonDbException& e)
                                    {
                                        fail(e.base().what());
                                    },

                                    requestId.value(),
                                    incidentId
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

    void AllocationService::allocate(
        unsigned long long incidentId,
        unsigned long long resourceId,
        unsigned int units,
        OperationCallback callback)
{
        allocate(
            incidentId,
            resourceId,
            units,
            std::nullopt,
            std::move(callback)
        );
    }

    void AllocationService::releaseAllocation(
        unsigned long long allocationId,
        OperationCallback callback)
{
        if (!callback)
            return;

        if (!dbClient)
        {
            callback(false, "Database client is unavailable");
            return;
        }

        if (allocationId == 0)
        {
            callback(false, "Invalid allocation ID");
            return;
        }

        auto state =
            std::make_shared<CompletionState>(std::move(callback));

        dbClient->newTransactionAsync(
            [=](const std::shared_ptr<drogon::orm::Transaction>& trans)
            {
                if (!trans)
                {
                    state->finish(
                        false,
                        "Could not create database transaction"
                    );
                    return;
                }

                auto fail = [trans, state](
                    const std::string& message)
                {
                    trans->rollback();
                    state->finish(false, message);
                };

                trans->setCommitCallback(
                    [state](bool committed)
                    {
                        state->finish(
                            committed,
                            committed
                                ? "Resource allocation released successfully"
                                : "Allocation release commit failed"
                        );
                    }
                );

                // STEP 1: Lock allocation record.
                trans->execSqlAsync(
                    "SELECT resource_id, incident_id, "
                    "allocated_units, status, "
                    "COALESCE(request_id, 0) AS request_id "
                    "FROM resource_allocations "
                    "WHERE allocation_id = ? FOR UPDATE",

                    [=](const drogon::orm::Result& allocationResult)
                    {
                        if (allocationResult.empty())
                        {
                            fail("Allocation record not found");
                            return;
                        }

                        const unsigned long long resourceId =
                            allocationResult[0]["resource_id"]
                                .as<unsigned long long>();

                        const unsigned long long incidentId =
                            allocationResult[0]["incident_id"]
                                .as<unsigned long long>();

                        const unsigned int units =
                            allocationResult[0]["allocated_units"]
                                .as<unsigned int>();

                        const unsigned long long requestId =
                            allocationResult[0]["request_id"]
                                .as<unsigned long long>();

                        const std::string allocationStatus =
                            allocationResult[0]["status"]
                                .as<std::string>();

                        if (units == 0)
                        {
                            fail("Allocation has invalid unit quantity");
                            return;
                        }

                        if (allocationStatus != "ASSIGNED" &&
                            allocationStatus != "DISPATCHED" &&
                            allocationStatus != "ON_SITE")
                        {
                            fail(
                                "Allocation cannot be released. "
                                "It may already be completed or cancelled"
                            );
                            return;
                        }

                        // STEP 2: Lock associated resource.
                        trans->execSqlAsync(
                            "SELECT total_units, available_units, status "
                            "FROM resources "
                            "WHERE resource_id = ? FOR UPDATE",

                            [=](const drogon::orm::Result& resourceResult)
                            {
                                if (resourceResult.empty())
                                {
                                    fail("Associated resource not found");
                                    return;
                                }

                                const unsigned int totalUnits =
                                    resourceResult[0]["total_units"]
                                        .as<unsigned int>();

                                const unsigned int availableUnits =
                                    resourceResult[0]["available_units"]
                                        .as<unsigned int>();

                                if (availableUnits > totalUnits ||
                                    units > totalUnits - availableUnits)
                                {
                                    fail(
                                        "Inventory integrity check failed. "
                                        "Cannot restore allocated units"
                                    );
                                    return;
                                }

                                // STEP 3: Restore resource inventory.
                                trans->execSqlAsync(
                                    "UPDATE resources SET "
                                    "available_units = available_units + ?, "
                                    "status = CASE "
                                    "WHEN status IN "
                                    "('MAINTENANCE', 'UNAVAILABLE') "
                                    "THEN status "
                                    "ELSE 'AVAILABLE' END "
                                    "WHERE resource_id = ? "
                                    "AND available_units + ? <= total_units",

                                    [=](const drogon::orm::Result& updateResult)
                                    {
                                        if (updateResult.affectedRows() != 1)
                                        {
                                            fail(
                                                "Could not restore resource "
                                                "inventory"
                                            );
                                            return;
                                        }

                                        // STEP 4: Mark allocation completed.
                                        trans->execSqlAsync(
                                            "UPDATE resource_allocations "
                                            "SET status = 'COMPLETED', "
                                            "released_at = NOW() "
                                            "WHERE allocation_id = ? "
                                            "AND status IN "
                                            "('ASSIGNED', 'DISPATCHED', 'ON_SITE')",

                                            [=](const drogon::orm::Result& allocationUpdate)
                                            {
                                                if (allocationUpdate.affectedRows() != 1)
                                                {
                                                    fail(
                                                        "Could not update "
                                                        "allocation status"
                                                    );
                                                    return;
                                                }

                                                // STEP 5: Record incident event.
                                                trans->execSqlAsync(
                                                    "INSERT INTO incident_events "
                                                    "(incident_id, event_type, description) "
                                                    "VALUES (?, ?, ?)",

                                                    [=](const drogon::orm::Result&)
                                                    {
                                                        if (requestId == 0)
                                                            return;

                                                        refreshRequestStatus(
                                                            trans,
                                                            requestId,
                                                            []() {},
                                                            [=](const std::string& message)
                                                            {
                                                                fail(message);
                                                            }
                                                        );
                                                    },

                                                    [=](const drogon::orm::DrogonDbException& e)
                                                    {
                                                        fail(e.base().what());
                                                    },

                                                    incidentId,
                                                    std::string("RESOURCE_RELEASED"),
                                                    std::string(
                                                        "Allocated resource units "
                                                        "were returned to inventory"
                                                    )
                                                );
                                            },

                                            [=](const drogon::orm::DrogonDbException& e)
                                            {
                                                fail(e.base().what());
                                            },

                                            allocationId
                                        );
                                    },

                                    [=](const drogon::orm::DrogonDbException& e)
                                    {
                                        fail(e.base().what());
                                    },

                                    units,
                                    resourceId,
                                    units
                                );
                            },

                            [=](const drogon::orm::DrogonDbException& e)
                            {
                                fail(e.base().what());
                            },

                            resourceId
                        );
                    },

                    [=](const drogon::orm::DrogonDbException& e)
                    {
                        fail(e.base().what());
                    },

                    allocationId
                );
            }
        );
    }
} // namespace nexa
