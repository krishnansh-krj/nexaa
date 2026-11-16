
#pragma once

#include <string>
#include <algorithm>
#include <cctype>
#include <initializer_list>

namespace nexa
{
class Validation
{
public:
    static std::string trim(const std::string& value)
    {
        const auto first = value.find_first_not_of(" \t\n\r");

        if (first == std::string::npos)
            return "";

        const auto last = value.find_last_not_of(" \t\n\r");
        return value.substr(first, last - first + 1);
    }

    static bool isNonEmpty(const std::string& value)
    {
        return !trim(value).empty();
    }

    static bool isValidEnum(
        const std::string& value,
        std::initializer_list<std::string> allowed)
    {
        return std::find(
            allowed.begin(),
            allowed.end(),
            value
        ) != allowed.end();
    }

    static bool isValidLatitude(double latitude)
    {
        return latitude >= -90.0 && latitude <= 90.0;
    }

    static bool isValidLongitude(double longitude)
    {
        return longitude >= -180.0 && longitude <= 180.0;
    }

    static bool isPositive(int value)
    {
        return value > 0;
    }
};
}
