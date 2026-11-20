#pragma once

// Geographic helpers shared by the spatial structures.

#include <cmath>

namespace nexa::dsa
{

struct GeoPoint
{
    double lat = 0.0;
    double lng = 0.0;
};

// Great-circle distance in kilometres.
inline double haversineKm(double lat1, double lng1, double lat2, double lng2)
{
    constexpr double kEarthRadiusKm = 6371.0088;
    constexpr double kDegToRad = 3.14159265358979323846 / 180.0;
    const double dLat = (lat2 - lat1) * kDegToRad;
    const double dLng = (lng2 - lng1) * kDegToRad;
    const double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
                     std::cos(lat1 * kDegToRad) * std::cos(lat2 * kDegToRad) *
                     std::sin(dLng / 2) * std::sin(dLng / 2);
    return 2.0 * kEarthRadiusKm * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
}

struct GeoBox
{
    double minLat = 0.0, maxLat = 0.0, minLng = 0.0, maxLng = 0.0;

    bool contains(double lat, double lng) const
    {
        return lat >= minLat && lat <= maxLat && lng >= minLng && lng <= maxLng;
    }

    bool intersects(const GeoBox& o) const
    {
        return !(o.minLat > maxLat || o.maxLat < minLat || o.minLng > maxLng || o.maxLng < minLng);
    }
};

} // namespace nexa::dsa
