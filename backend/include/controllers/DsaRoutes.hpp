#pragma once

// /api/dsa/* - read-only endpoints that expose the synopsis data structures
// through the Intelligence Engine. All routes are GET and never modify data.

namespace nexa
{

class DsaRoutes
{
public:
    static void registerRoutes();
};

} // namespace nexa
