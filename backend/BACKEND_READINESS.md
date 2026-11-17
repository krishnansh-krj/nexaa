# NEXA Backend — Integration Readiness

## Current scope
This package contains the backend after the `.h` → `.hpp` migration batches 1–6,
plus the CMake/test integration improvements described below.

## Completed in this verification pass
- Added `cmake_minimum_required()` and `project()` declarations.
- Added `NEXA_BUILD_SERVER` and `NEXA_BUILD_TESTS` options.
- Kept the Drogon-backed server enabled by default.
- Registered the DSA regression test with CTest.
- Added a RequestPrioritizer unit test and registered it with CTest.
- Verified all 26 server `.cpp` entries listed by CMake exist.
- Verified all quoted project-local includes resolve.
- Configured, built, and ran the non-Drogon test targets with GCC 14.2 / CMake 3.31.6.

## Verified local test command (does not build the server)
From the `backend` directory:

```bash
cmake -S . -B build -DNEXA_BUILD_SERVER=OFF -DNEXA_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Expected result: 2 tests pass (`nexa_dsa_test`, `nexa_request_prioritizer_test`).

## Full server build — BLOCKED pending Drogon
The full server build has not yet been verified. The current execution environment
does not have Drogon installed or a `DrogonConfig.cmake` package configuration.
A default CMake configure therefore stops at `find_package(Drogon CONFIG REQUIRED)`.

After installing Drogon and its development dependencies, configure with:

```bash
cmake -S . -B build-server -DNEXA_BUILD_SERVER=ON -DNEXA_BUILD_TESTS=ON
cmake --build build-server --parallel
ctest --test-dir build-server --output-on-failure
```

If Drogon is installed in a non-standard prefix, pass its package location using
`-DCMAKE_PREFIX_PATH=<drogon-install-prefix>` or `-DDrogon_DIR=<directory-containing-DrogonConfig.cmake>`.

## Not yet approved for frontend phase
Do not treat the backend as integration-complete until:
1. Drogon is installed in the target development environment.
2. The complete `nexa_backend` target compiles and links.
3. All tests pass in the full server-enabled build.
4. Database configuration and connectivity are verified against the intended DB.
5. The server starts and its health/API routes are smoke-tested.
6. Required schema/migrations and runtime configuration are confirmed.

## Source note
`EmergencyPriorityAlgorithm.h` and `ResourceAllocationAlgorithm.h` were zero-byte
files in the supplied source archive. They remain untouched; no implementation
was available to migrate.
