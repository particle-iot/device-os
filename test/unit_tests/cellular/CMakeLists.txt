set(target_name cellular)

# Generate include path
include_directories(
  ${DEVICE_OS_DIR}/hal/inc/
  ${DEVICE_OS_DIR}/hal/shared/
  ${DEVICE_OS_DIR}/services/inc/
  ${DEVICE_OS_DIR}/wiring/inc/
  ${DEVICE_OS_DIR}/hal/network
  ${DEVICE_OS_DIR}/hal/src/gcc
)

# Create test executable
add_executable( ${target_name}
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_cellular_printable.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_print.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_variant.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_json.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_string.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_buffer.cpp
  ${DEVICE_OS_DIR}/wiring/src/string_convert.cpp
  ${DEVICE_OS_DIR}/hal/network/ncp/cellular/network_config_db.cpp
  ${DEVICE_OS_DIR}/hal/shared/cellular_sig_perc_mapping.cpp
  ${DEVICE_OS_DIR}/services/src/jsmn.c
  cellular.cpp
)

# Set defines specific to target
target_compile_definitions( ${target_name}
  PRIVATE PLATFORM_ID=3
)

# Set compiler flags specific to target
target_compile_options( ${target_name}
  PRIVATE ${COVERAGE_CFLAGS}
)

# Link against dependencies specific to target

# Add tests to `test` target
catch_discover_tests( ${target_name}
  TEST_PREFIX ${target_name}_
)
