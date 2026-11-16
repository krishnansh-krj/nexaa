
#pragma once

#include <drogon/drogon.h>

#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace nexa
{
class BaseRepository
{
public:
    using ResultCallback =
        std::function<void(const drogon::orm::Result&)>;

    using ErrorCallback =
        std::function<void(const drogon::orm::DrogonDbException&)>;

    explicit BaseRepository(
        std::shared_ptr<drogon::orm::DbClient> client)
        : dbClient(std::move(client))
    {
    }

    virtual ~BaseRepository() = default;

protected:
    std::shared_ptr<drogon::orm::DbClient> dbClient;

    template <typename... Args>
    void execute(
        const std::string& sql,
        ResultCallback onSuccess,
        ErrorCallback onError,
        Args&&... args) const
    {
        dbClient->execSqlAsync(
            sql,
            std::move(onSuccess),
            std::move(onError),
            std::forward<Args>(args)...
        );
    }
};
}
