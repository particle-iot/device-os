cmake_minimum_required(VERSION 3.7)
project(unit_tests)

# NOTE: Keep this in sync with lang-std.mk
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_C_STANDARD 11)

# Create variables for commonly referenced directories
set(DEVICE_OS_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)
set(THIRD_PARTY_DIR ${DEVICE_OS_DIR}/third_party)

# Add Catch2 from git submodule
set(CATCH2_DIR ${THIRD_PARTY_DIR}/catch2/catch2)
add_subdirectory(${CATCH2_DIR} ${CMAKE_BINARY_DIR}/catch2-build)
list(APPEND CMAKE_MODULE_PATH ${CATCH2_DIR}/contrib)
include(Catch)

# Expose `test` target from the `make` command
enable_testing()

# Build flags to enable code coverage
SET(GCC_COVERAGE_COMPILE_FLAGS "-fno-inline -fprofile-arcs -ftest-coverage -O0 -g")
SET(GCC_COVERAGE_LINK_FLAGS "-lgcov")

# Set compiler flags for all tests
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

# Global defines for all tests
add_definitions(-DUNIT_TEST)

# Build and discover unit-tests
add_subdirectory(cloud)
add_subdirectory(services)

add_custom_target( coverage
  gcovr --root ${CMAKE_CURRENT_LIST_DIR} --print-summary
)
