
#pragma once

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>

namespace nexa {

inline std::string normalizeType(std::string value)
{
    const auto first = value.find_first_not_of(" \t\n\r");
    if (first == std::string::npos)
        return {};

    const auto last = value.find_last_not_of(" \t\n\r");

    value = value.substr(first, last - first + 1);

    std::transform(
        value.begin(), value.end(), value.begin(),
        [](unsigned char ch) {
            return static_cast<char>(std::toupper(ch));
        }
    );

    return value;
}

inline std::optional<std::string> compatibleResourceType(
    const std::string& requestType)
{
    const std::string type = normalizeType(requestType);

    if (type == "FIRE_SUPPRESSION")
        return "FIRE_TRUCK";

    if (type == "MEDICAL_ASSISTANCE")
        return "MEDICAL_TEAM";

    if (type == "RESCUE_SUPPORT")
        return "RESCUE_TEAM";

    if (type == "AMBULANCE")
        return "AMBULANCE";

    return std::nullopt;
}

inline bool isValidRequestType(const std::string& type)
{
    return compatibleResourceType(type).has_value();
}

inline bool isCompatibleResource(
    const std::string& requestType,
    const std::string& resourceType)
{
    const auto expected = compatibleResourceType(requestType);

    return expected.has_value() &&
           normalizeType(resourceType) == *expected;
}

}
