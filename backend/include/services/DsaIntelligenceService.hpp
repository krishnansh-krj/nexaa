#pragma once

// Loads a read-only snapshot of nexa_db (all seven tables) for the
// Intelligence Engine. The snapshot is built per request, so the data
// structures always reflect the live database; nothing is written back.

#include "intelligence/IntelligenceEngine.hpp"

#include <drogon/drogon.h>

#include <functional>
#include <memory>
#include <string>

namespace nexa
{

class DsaIntelligenceService
{
public:
    using SnapshotPtr = std::shared_ptr<const intel::Snapshot>;
    using ReadyCallback = std::function<void(SnapshotPtr)>;
    using ErrorCallback = std::function<void(const drogon::orm::DrogonDbException&)>;
    using FailureCallback = std::function<void(const std::string&)>;

    static void loadSnapshot(const std::shared_ptr<drogon::orm::DbClient>& db,
                             ReadyCallback onReady,
                             ErrorCallback onError,
                             FailureCallback onFailure);
};

} // namespace nexa
