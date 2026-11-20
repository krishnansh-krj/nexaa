# NEXA

Real-time integrated disaster intelligence and emergency coordination platform —
C++20 data structures and algorithms, Drogon REST API, MySQL 8.4.

- `backend/` — server, data-structure library (`include/dsa`), tests
- `database/` — schema, seed (fresh installs only), migrations
- `scripts/smoke_test.ps1` — read-only API check

Build and test:

```cmd
cmake --build build --config Release --parallel
ctest --test-dir build\backend -C Release --output-on-failure
```

See `backend/API_REFERENCE.md` and `backend/BACKEND_READINESS.md`.
