set(target_name wiring)

# Create test executable
add_executable( ${target_name}
  ${DEVICE_OS_DIR}/hal/src/gcc/timer_hal.cpp
  ${DEVICE_OS_DIR}/hal/src/template/wlan_hal.cpp
  ${DEVICE_OS_DIR}/services/src/completion_handler.cpp
  ${DEVICE_OS_DIR}/services/src/system_error.cpp
  ${DEVICE_OS_DIR}/services/src/jsmn.c
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_async.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_print.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_stream.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_random.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_string.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_wifi.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_network.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_json.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_fuel.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_i2c.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_ipaddress.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_variant.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_buffer.cpp
  ${DEVICE_OS_DIR}/wiring_globals/src/wiring_globals_i2c.cpp
  ${DEVICE_OS_DIR}/hal/src/template/i2c_hal.cpp
  ${DEVICE_OS_DIR}/wiring/src/string_convert.cpp
  ${TEST_DIR}/util/alloc.cpp
  ${TEST_DIR}/util/buffer.cpp
  ${TEST_DIR}/util/string.cpp
  ${TEST_DIR}/util/random_old.cpp
  ${TEST_DIR}/stub/system_network.cpp
  ${TEST_DIR}/stub/inet_hal_compat.cpp
  async.cpp
  print.cpp
  vector.cpp
  print2.cpp
  random.cpp
  string.cpp
  character.cpp
  error.cpp
  flags.cpp
  fuel_gauge.cpp
  ipaddress.cpp
  json.cpp
  std_functions.cpp
  wlan.cpp
  map.cpp
  variant.cpp
  buffer.cpp
)

# Set defines specific to target
target_compile_definitions( ${target_name}
  PRIVATE PLATFORM_ID=3
  PRIVATE HAL_PLATFORM_ERROR_MESSAGES=1
)

# Set compiler flags specific to target
target_compile_options( ${target_name}
  PRIVATE ${COVERAGE_CFLAGS}
)

# Set include path specific to target
target_include_directories( ${target_name}
  PRIVATE ${TEST_DIR}
  PRIVATE ${TEST_DIR}/stub/
  PRIVATE ${DEVICE_OS_DIR}/communication/inc/
  PRIVATE ${DEVICE_OS_DIR}/hal/inc/
  PRIVATE ${DEVICE_OS_DIR}/hal/src/gcc/
  PRIVATE ${DEVICE_OS_DIR}/hal/shared/
  PRIVATE ${DEVICE_OS_DIR}/platform/MCU/gcc/inc
  PRIVATE ${DEVICE_OS_DIR}/dynalib/inc/
  PRIVATE ${DEVICE_OS_DIR}/services/inc/
  PRIVATE ${DEVICE_OS_DIR}/system/inc/
  PRIVATE ${DEVICE_OS_DIR}/wiring/inc/
  PRIVATE ${THIRD_PARTY_DIR}/hippomocks
)

# Link against dependencies specific to target

# Add tests to `test` target
catch_discover_tests( ${target_name}
  TEST_PREFIX ${target_name}_
)
