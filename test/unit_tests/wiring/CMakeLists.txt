set(target_name wiring)

# Create test executable
add_executable( ${target_name}
  ${DEVICE_OS_DIR}/hal/src/gcc/timer_hal.cpp
  ${DEVICE_OS_DIR}/services/src/completion_handler.cpp
  ${DEVICE_OS_DIR}/services/src/system_error.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_async.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_print.cpp
  async.cpp
  print.cpp
)

# Set defines specific to target
target_compile_definitions( ${target_name}
  PRIVATE PLATFORM_ID=3
)

# Set compiler flags specific to target
target_compile_options( ${target_name}
  PRIVATE ${COVERAGE_CFLAGS}
)

# Set include path specific to target
target_include_directories( ${target_name}
  PRIVATE ${TEST_DIR}/stub/
  PRIVATE ${DEVICE_OS_DIR}/communication/inc/
  PRIVATE ${DEVICE_OS_DIR}/hal/inc/
  PRIVATE ${DEVICE_OS_DIR}/hal/shared/
  PRIVATE ${DEVICE_OS_DIR}/services/inc/
  PRIVATE ${DEVICE_OS_DIR}/system/inc/
  PRIVATE ${DEVICE_OS_DIR}/wiring/inc/
)

# Link against dependencies specific to target

# Add tests to `test` target
catch_discover_tests( ${target_name}
  TEST_PREFIX ${target_name}_
)
