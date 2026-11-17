cmake_minimum_required(VERSION 3.20)

project(NEXA_Backend LANGUAGES C CXX)

option(NEXA_BUILD_SERVER "Build the Drogon-powered NEXA backend server" ON)
option(NEXA_BUILD_TESTS "Build NEXA backend unit tests" ON)

set(NEXA_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(NEXA_BUILD_SERVER)
    find_package(Drogon CONFIG REQUIRED)

    add_executable(nexa_backend
        src/main.cpp
        src/config/DatabaseConfig.cpp
        src/utilities/Validation.cpp
        src/utilities/ApiResponse.cpp
        src/controllers/ApiRoutes.cpp
        src/services/NetworkGraphService.cpp
        src/services/AllocationService.cpp
        src/services/IncidentEventService.cpp
        src/services/IncidentService.cpp
        src/services/NetworkService.cpp
        src/services/RequestPrioritizationService.cpp
        src/services/RequestService.cpp
        src/services/ResourceService.cpp
        src/services/ResourceMatchingService.cpp
        src/data_structures/Graph.cpp
        src/data_structures/MinHeap.cpp
        src/algorithms/BFS.cpp
        src/algorithms/DFS.cpp
        src/algorithms/Dijkstra.cpp
        src/algorithms/RequestPrioritizer.cpp
        src/repositories/AllocationRepository.cpp
        src/repositories/IncidentEventRepository.cpp
        src/repositories/IncidentRepository.cpp
        src/repositories/NetworkRepository.cpp
        src/repositories/RequestRepository.cpp
        src/repositories/ResourceRepository.cpp
    )

    target_include_directories(nexa_backend PRIVATE "${NEXA_INCLUDE_DIR}")
    target_link_libraries(nexa_backend PRIVATE Drogon::Drogon)
    target_compile_features(nexa_backend PRIVATE cxx_std_20)
endif()

if(NEXA_BUILD_TESTS)
    include(CTest)

    add_executable(nexa_dsa_test
        tests/dsa_test.cpp
        src/data_structures/Graph.cpp
        src/data_structures/MinHeap.cpp
        src/algorithms/BFS.cpp
        src/algorithms/DFS.cpp
        src/algorithms/Dijkstra.cpp
    )
    target_include_directories(nexa_dsa_test PRIVATE "${NEXA_INCLUDE_DIR}")
    target_compile_features(nexa_dsa_test PRIVATE cxx_std_20)
    add_test(NAME nexa_dsa_test COMMAND nexa_dsa_test)

    add_executable(nexa_request_prioritizer_test
        tests/request_prioritizer_test.cpp
        src/algorithms/RequestPrioritizer.cpp
    )
    target_include_directories(nexa_request_prioritizer_test PRIVATE "${NEXA_INCLUDE_DIR}")
    target_compile_features(nexa_request_prioritizer_test PRIVATE cxx_std_20)
    add_test(NAME nexa_request_prioritizer_test COMMAND nexa_request_prioritizer_test)
endif()
