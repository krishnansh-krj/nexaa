
find_package(Drogon CONFIG REQUIRED)

add_executable(nexa_backend
    src/main.cpp
)

target_include_directories(nexa_backend
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(nexa_backend
    PRIVATE
        Drogon::Drogon
)

target_compile_features(nexa_backend PRIVATE cxx_std_20)
