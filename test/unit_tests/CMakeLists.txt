cmake_minimum_required(VERSION 3.2)
project(unit_tests)

# NOTE: Keep this in sync with lang-std.mk
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_C_STANDARD 11)

set(Boost_USE_STATIC_LIBS ON)

# Create variables for commonly referenced directories
get_filename_component(
  DEVICE_OS_DIR
  ${CMAKE_CURRENT_LIST_DIR}/../..
  REALPATH
)
set(TEST_DIR ${DEVICE_OS_DIR}/test/unit_tests)
set(THIRD_PARTY_DIR ${DEVICE_OS_DIR}/third_party)

# Add Boost from git submodule
find_package(Boost
  1.59.0
  REQUIRED
  COMPONENTS program_options random regex system thread iostreams
)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  link_libraries(${Boost_LIBRARIES})
endif()

# Add Catch2 from git submodule
set(CATCH2_DIR ${THIRD_PARTY_DIR}/catch2/catch2)
add_subdirectory(${CATCH2_DIR} ${CMAKE_BINARY_DIR}/catch2-build)
list(APPEND CMAKE_MODULE_PATH ${CATCH2_DIR}/contrib)
include(Catch)

# Expose `test` target from the `make` command
enable_testing()

# Global defines for all tests
add_definitions(-DLOG_DISABLE)
add_definitions(-DRELEASE_BUILD)
add_definitions(-DUNIT_TEST)
add_definitions(-DBOOST_NO_AUTO_PTR)

if (CMAKE_COMPILER_IS_GNUCXX)
  set(GCOV_ENABLE TRUE)
endif()

if (GCOV_ENABLE)
  set(COVERAGE_LIBRARIES gcov)
  set(COVERAGE_CFLAGS -fno-inline -fprofile-arcs -ftest-coverage -O0 -g)
endif()

# Link against dependencies of all tests
link_libraries(
  Catch2::Catch2
  ${COVERAGE_LIBRARIES}
)

# Build and discover unit-tests
add_subdirectory(cellular)
add_subdirectory(cloud)
add_subdirectory(communication)
add_subdirectory(services)
add_subdirectory(wiring)
add_subdirectory(hal)

# Create `coverage` target in the `make` command
add_custom_target( coverage
  gcovr --root ${DEVICE_OS_DIR} --exclude ${TEST_DIR} --exclude ${THIRD_PARTY_DIR} -j 4 --print-summary
)

# Create `coveralls` target in the `make` command
add_custom_target( coveralls
  gcovr --root ${DEVICE_OS_DIR} --exclude ${TEST_DIR} --exclude ${THIRD_PARTY_DIR} -j 4 --coveralls coverage.json
)
