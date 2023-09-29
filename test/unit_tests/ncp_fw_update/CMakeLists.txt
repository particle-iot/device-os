set(target_name ncp_fw_update)

# Create test executable
add_executable( ${target_name}
  ${DEVICE_OS_DIR}/services/src/ncp_fw_update.cpp
  ${TEST_DIR}/stub/system_cache.cpp
  ncp_fw_update.cpp
  stubs.cpp
  main.cpp
)

# Set defines specific to target
target_compile_definitions( ${target_name}
  PRIVATE PLATFORM_ID=3
  PRIVATE HAL_PLATFORM_NCP_FW_UPDATE=1
  PRIVATE HAL_PLATFORM_NCP=1
  PRIVATE HAL_PLATFORM_NCP_AT=1
  PRIVATE HAL_PLATFORM_WIFI_SERIAL=0
)

# Set compiler flags specific to target
target_compile_options( ${target_name}
  PRIVATE ${COVERAGE_CFLAGS}
)

# Set include path specific to target
target_include_directories( ${target_name}
  PRIVATE ${TEST_DIR}
  PRIVATE ${TEST_DIR}/stub
  PRIVATE ${DEVICE_OS_DIR}/hal/
  PRIVATE ${DEVICE_OS_DIR}/hal/inc/
  PRIVATE ${DEVICE_OS_DIR}/hal/shared/
  PRIVATE ${DEVICE_OS_DIR}/system/inc/
  PRIVATE ${DEVICE_OS_DIR}/services/inc/
  PRIVATE ${DEVICE_OS_DIR}/wiring/inc/
  PRIVATE ${DEVICE_OS_DIR}/dynalib/inc/
  PRIVATE ${DEVICE_OS_DIR}/hal/src/gcc/
  PRIVATE ${DEVICE_OS_DIR}/hal/network/ncp/
  PRIVATE ${DEVICE_OS_DIR}/hal/network/ncp/at_parser/
  PRIVATE ${THIRD_PARTY_DIR}/hippomocks
)

# Link against dependencies specific to target

# Add tests to `test` target
catch_discover_tests( ${target_name}
  TEST_PREFIX ${target_name}_
)
