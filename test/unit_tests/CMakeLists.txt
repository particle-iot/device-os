cmake_minimum_required(VERSION 3.0)
project(unit_tests)

# NOTE: Keep this in sync with lang-std.mk
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_C_STANDARD 11)

# Create variables for commonly referenced directories
execute_process(
  COMMAND realpath ${CMAKE_CURRENT_LIST_DIR}/../.. --zero
  OUTPUT_VARIABLE DEVICE_OS_DIR
)
execute_process(
  COMMAND realpath ${DEVICE_OS_DIR}/test --zero
  OUTPUT_VARIABLE TEST_DIR
)
execute_process(
  COMMAND realpath ${DEVICE_OS_DIR}/third_party --zero
  OUTPUT_VARIABLE THIRD_PARTY_DIR
)

# Add Catch2 from git submodule
set(CATCH2_DIR ${THIRD_PARTY_DIR}/catch2/catch2)
add_subdirectory(${CATCH2_DIR} ${CMAKE_BINARY_DIR}/catch2-build)
list(APPEND CMAKE_MODULE_PATH ${CATCH2_DIR}/contrib)
include(Catch)

# Expose `test` target from the `make` command
enable_testing()

# Global defines for all tests
add_definitions(-DRELEASE_BUILD)
add_definitions(-DUNIT_TEST)

# Link against dependencies of all tests
link_libraries(
  Catch2::Catch2
  gcov
)

# Build and discover unit-tests
add_subdirectory(cloud)
add_subdirectory(services)

# Create `coverage` target in the `make` command
add_custom_target( coverage
  gcovr --root ${DEVICE_OS_DIR} --exclude ${TEST_DIR} --exclude ${THIRD_PARTY_DIR} -j 4 --print-summary
)
