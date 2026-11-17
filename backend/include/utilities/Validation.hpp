#pragma once

#include <string>
#include <initializer_list>

namespace nexa
{
class Validation
{
public:
    static std::string trim(const std::string& value);
    static bool isNonEmpty(const std::string& value);
    static bool isValidEnum(const std::string& value,
                            std::initializer_list<std::string> allowed);
    static bool isValidLatitude(double latitude);
    static bool isValidLongitude(double longitude);
    static bool isPositive(int value);
};
}
