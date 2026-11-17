#include "config/DatabaseConfig.hpp"

#include <drogon/drogon.h>
#include <cstdlib>
#include <string>

namespace nexa
{
bool DatabaseConfig::registerMySql()
{
    const char* password = std::getenv("NEXA_DB_PASSWORD");
    if (password == nullptr || std::string(password).empty())
    {
        LOG_ERROR << "NEXA_DB_PASSWORD environment variable is not set.";
        return false;
    }

    drogon::app().createDbClient(
        "mysql", "127.0.0.1", 3306, "nexa_db", "nexa_app", password, 2);
    LOG_INFO << "NEXA MySQL client registered.";
    return true;
}
}
