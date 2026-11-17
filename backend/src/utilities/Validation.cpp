#include "utilities/Validation.hpp"

#include <algorithm>

namespace nexa
{
std::string Validation::trim(const std::string& value)
{
    const auto first = value.find_first_not_of(" \t\n\r");
    if (first == std::string::npos)
        return "";
    const auto last = value.find_last_not_of(" \t\n\r");
    return value.substr(first, last - first + 1);
}

bool Validation::isNonEmpty(const std::string& value)
{
    return !trim(value).empty();
}

bool Validation::isValidEnum(
    const std::string& value,
    std::initializer_list<std::string> allowed)
{
    return std::find(allowed.begin(), allowed.end(), value) != allowed.end();
}

bool Validation::isValidLatitude(double latitude)
{
    return latitude >= -90.0 && latitude <= 90.0;
}

bool Validation::isValidLongitude(double longitude)
{
    return longitude >= -180.0 && longitude <= 180.0;
}

bool Validation::isPositive(int value)
{
    return value > 0;
}
}
