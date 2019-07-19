# Global defines

# Generate include path
include_directories(
  ${DEVICE_OS_DIR}/services/inc
)

# Create test executable
add_executable(
  services
  ${DEVICE_OS_DIR}/services/src/str_util.cpp
  str_util.cpp
)

# Set compiler flags specific to target
target_compile_options( services
  PRIVATE -fno-inline -fprofile-arcs -ftest-coverage -O0 -g
)

# Link against dependencies specific to target

# Add tests to `test` target
catch_discover_tests( services
  TEST_PREFIX services_str_util_
)
