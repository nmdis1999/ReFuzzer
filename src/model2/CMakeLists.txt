add_executable(recompile recompile.cpp)

find_package(nlohmann_json REQUIRED)
target_link_libraries(recompile nlohmann_json::nlohmann_json)

find_package(CURL REQUIRED)
target_link_libraries(recompile CURL::libcurl)
