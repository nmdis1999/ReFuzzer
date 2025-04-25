cmake_minimum_required(VERSION 3.15)
project(SmartCompilerTesting)

set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES ${CMAKE_SOURCE_DIR}/conan_provider.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSION off)

add_subdirectory(src/query_generator)
add_subdirectory(src/model2)

add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/src/log
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/src/test
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/src/sanitizer_log
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/src/sanitizer_crash
    COMMAND bash -c "rm -f ${CMAKE_SOURCE_DIR}/src/model2/sanitizer.supp"
    COMMAND bash -c "rm -f ${CMAKE_SOURCE_DIR}/src/query_generator/sanitizer.supp"
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_SOURCE_DIR}/query_generator/overflow*
)