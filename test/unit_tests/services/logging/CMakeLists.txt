set(target_name logging)

# Create test executable
add_executable( ${target_name}
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_logging.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_json.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_string.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_buffer.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_print.cpp
  ${DEVICE_OS_DIR}/wiring/src/string_convert.cpp
  ${DEVICE_OS_DIR}/services/src/logging.cpp
  ${DEVICE_OS_DIR}/services/src/jsmn.c
  ${DEVICE_OS_DIR}/services/src/debug.c
  ${DEVICE_OS_DIR}/hal/src/gcc/timer_hal.cpp
  ${TEST_DIR}/util/random_old.cpp
  ${TEST_DIR}/util/string.cpp
  ${TEST_DIR}/stub/system_control.cpp
  ${TEST_DIR}/stub/security_mode.cpp
  logging.cpp
  main.cpp
)

# Set defines specific to target
target_compile_definitions( ${target_name}
  PRIVATE PLATFORM_ID=3
  PRIVATE USE_STDPERIPH_DRIVER
)

remove_definitions(-DLOG_DISABLE)

# Set compiler flags specific to target
target_compile_options( ${target_name}
  PRIVATE ${COVERAGE_CFLAGS}
)

# Set include path specific to target
target_include_directories( ${target_name}
  PRIVATE ${TEST_DIR}
  PRIVATE ${TEST_DIR}/stub
  PRIVATE ${DEVICE_OS_DIR}/services/inc
  PRIVATE ${DEVICE_OS_DIR}/dynalib/inc
  PRIVATE ${DEVICE_OS_DIR}/wiring/inc
  PRIVATE ${DEVICE_OS_DIR}/system/inc
  PRIVATE ${DEVICE_OS_DIR}/communication/inc
  PRIVATE ${DEVICE_OS_DIR}/hal/inc
  PRIVATE ${DEVICE_OS_DIR}/hal/shared
  PRIVATE ${DEVICE_OS_DIR}/platform/MCU/gcc/inc
  PRIVATE ${DEVICE_OS_DIR}/hal/src/gcc
  PRIVATE ${THIRD_PARTY_DIR}/hippomocks
)

# Link against dependencies specific to target

# Add tests to `test` target
catch_discover_tests( ${target_name}
  TEST_PREFIX ${target_name}_
)
