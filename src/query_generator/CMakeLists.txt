add_executable(query_generator 
    query_generator.cpp
)

find_package(nlohmann_json REQUIRED)
target_link_libraries(query_generator nlohmann_json::nlohmann_json)

find_package(CURL REQUIRED)
target_link_libraries(query_generator CURL::libcurl)