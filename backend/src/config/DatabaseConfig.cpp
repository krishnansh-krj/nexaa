
#include "config/DatabaseConfig.hpp"

#include <drogon/drogon.h>
#include <variant>
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

    drogon::orm::MysqlConfig mysqlConfig{};

    mysqlConfig.host = "127.0.0.1";
    mysqlConfig.port = 3306;
    mysqlConfig.databaseName = "nexa_db";
    mysqlConfig.username = "nexa_app";
    mysqlConfig.password = password;
    mysqlConfig.connectionNumber = 2;
    mysqlConfig.name = "default";
    mysqlConfig.isFast = false;
    mysqlConfig.characterSet = "utf8mb4";
    mysqlConfig.timeout = -1.0;

    drogon::orm::DbConfig dbConfig{mysqlConfig};

LOG_INFO << "DbConfig variant index: " << dbConfig.index();

LOG_INFO << "Is MySQL config: "
         << std::holds_alternative<drogon::orm::MysqlConfig>(dbConfig);

LOG_INFO << "Is PostgreSQL config: "
         << std::holds_alternative<drogon::orm::PostgresConfig>(dbConfig);

drogon::app().addDbClient(dbConfig);

    LOG_INFO << "NEXA MySQL client registered successfully.";

    return true;
}

}
