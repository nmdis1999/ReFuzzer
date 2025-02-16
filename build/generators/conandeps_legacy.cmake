message(STATUS "Conan: Using CMakeDeps conandeps_legacy.cmake aggregator via include()")
message(STATUS "Conan: It is recommended to use explicit find_package() per dependency instead")

find_package(nlohmann_json)
find_package(OpenSSL)
find_package(CURL)
find_package(ZLIB)

set(CONANDEPS_LEGACY  nlohmann_json::nlohmann_json  openssl::openssl  CURL::libcurl  ZLIB::ZLIB )