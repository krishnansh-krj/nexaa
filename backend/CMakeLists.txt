
add_executable(nexa_backend
    src/main.cpp
)

find_package(Drogon CONFIG REQUIRED)

target_link_libraries(nexa_backend PRIVATE Drogon::Drogon)
