cmake_minimum_required(VERSION 3.7)

set (CMAKE_CXX_STANDARD 11)

project(unit_tests)

# NOTE: Keep this in sync with lang-std.mk
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_C_STANDARD 11)

set(PROJECT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)
set(THIRD_PARTY_DIR ${PROJECT_DIR}/third_party)
set(COMMON_DIR ${CMAKE_CURRENT_LIST_DIR}/common)

set(CATCH2_DIR ${THIRD_PARTY_DIR}/catch2/catch2)
add_subdirectory(${CATCH2_DIR} ${CMAKE_BINARY_DIR}/catch2-build)
list(APPEND CMAKE_MODULE_PATH ${CATCH2_DIR}/contrib)

include(CTest)
include(Catch)

add_definitions(-DUNIT_TEST)

add_subdirectory(cloud)
add_subdirectory(services)
