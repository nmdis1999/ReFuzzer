cmake_minimum_required(VERSION 3.15)
project(SmartCompilerTesting)

set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES ${CMAKE_SOURCE_DIR}/conan_provider.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSION off)

add_subdirectory(src/query_generator)
add_subdirectory(src/model2)
