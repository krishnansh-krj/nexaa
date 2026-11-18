
#include "utilities/TypeMapping.hpp"

#include <cassert>
#include <iostream>

int main()
{
    using namespace nexa;

    assert(isCompatibleResource(
        "FIRE_SUPPRESSION", "FIRE_TRUCK"));

    assert(isCompatibleResource(
        "MEDICAL_ASSISTANCE", "MEDICAL_TEAM"));

    assert(isCompatibleResource(
        "RESCUE_SUPPORT", "RESCUE_TEAM"));

    assert(isCompatibleResource(
        "AMBULANCE", "AMBULANCE"));

    // Case and whitespace normalization
    assert(isCompatibleResource(
        " fire_suppression ", " fire_truck "));

    // Invalid and incompatible combinations
    assert(!isValidRequestType("UNKNOWN"));
    assert(!isCompatibleResource(
        "FIRE_SUPPRESSION", "AMBULANCE"));

    assert(!isCompatibleResource(
        "MEDICAL_ASSISTANCE", "FIRE_TRUCK"));

    std::cout << "All type mapping tests passed.\n";

    return 0;
}
