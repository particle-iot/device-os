set(target_name cloud)

# Create test executable
add_executable( ${target_name}
  ${DEVICE_OS_DIR}/system/src/system_publish_vitals.cpp
  publish_vitals.cpp
)

# Set defines specific to target
target_compile_definitions( ${target_name}
  PRIVATE PLATFORM_ID=6
)

# Set compiler flags specific to target
target_compile_options( ${target_name}
  PRIVATE ${COVERAGE_CFLAGS}
)

# Set include path specific to target
target_include_directories( ${target_name}
  PRIVATE ${DEVICE_OS_DIR}/communication/inc/
  PRIVATE ${DEVICE_OS_DIR}/hal/inc/
  PRIVATE ${DEVICE_OS_DIR}/hal/shared/
  PRIVATE ${DEVICE_OS_DIR}/hal/src/stm32/
  PRIVATE ${DEVICE_OS_DIR}/services/inc/
  PRIVATE ${DEVICE_OS_DIR}/system/inc/
  PRIVATE ${DEVICE_OS_DIR}/system/src/
  PRIVATE ${DEVICE_OS_DIR}/wiring/inc/
  PRIVATE ${TEST_DIR}/
  PRIVATE ${THIRD_PARTY_DIR}/fakeit/fakeit/single_header/catch/
)

# Link against dependencies specific to target

# Add tests to `test` target
catch_discover_tests( ${target_name}
  TEST_PREFIX ${target_name}_
)
