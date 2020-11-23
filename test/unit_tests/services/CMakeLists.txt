set(target_name services)

# Create test executable
add_executable( ${target_name}
  ${TEST_DIR}/stub/filesystem.cpp
  ${TEST_DIR}/mock/filesystem.cpp
  ${TEST_DIR}/util/random.cpp
  ${DEVICE_OS_DIR}/services/src/simple_file_storage.cpp
  ${DEVICE_OS_DIR}/services/src/str_util.cpp
  simple_file_storage.cpp
  str_util.cpp
  varint.cpp
  main.cpp
)

# Set defines specific to target
target_compile_definitions( ${target_name}
  PRIVATE PLATFORM_ID=3
  PRIVATE HAL_PLATFORM_FILESYSTEM=1
)

# Set compiler flags specific to target
target_compile_options( ${target_name}
  PRIVATE ${COVERAGE_CFLAGS}
)

# Set include path specific to target
target_include_directories( ${target_name}
  PRIVATE ${TEST_DIR}
  PRIVATE ${TEST_DIR}/stub
  PRIVATE ${DEVICE_OS_DIR}/services/inc
  PRIVATE ${DEVICE_OS_DIR}/hal/inc
  PRIVATE ${DEVICE_OS_DIR}/hal/shared
  PRIVATE ${THIRD_PARTY_DIR}/hippomocks
)

# Link against dependencies specific to target

# Add tests to `test` target
catch_discover_tests( ${target_name}
  TEST_PREFIX ${target_name}_
)
