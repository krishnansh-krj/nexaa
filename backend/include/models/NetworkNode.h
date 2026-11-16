
#pragma once

#include <string>

namespace nexa
{
struct NetworkNode
{
    unsigned long long nodeId = 0;

    std::string nodeCode;
    std::string nodeName;

    double latitude = 0.0;
    double longitude = 0.0;

    std::string nodeType = "ROAD";
};
}
