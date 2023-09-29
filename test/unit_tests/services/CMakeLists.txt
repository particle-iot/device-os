set(target_name services)

# Create test executable
add_executable( ${target_name}
  ${TEST_DIR}/stub/filesystem.cpp
  ${TEST_DIR}/mock/filesystem.cpp
  ${TEST_DIR}/util/random.cpp
  ${TEST_DIR}/util/random_old.cpp
  ${DEVICE_OS_DIR}/services/src/simple_file_storage.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_led.cpp
  ${DEVICE_OS_DIR}/services/src/str_util.cpp
  ${DEVICE_OS_DIR}/services/src/diagnostics.cpp
  ${DEVICE_OS_DIR}/services/src/rgbled.c
  ${DEVICE_OS_DIR}/services/src/led_service.cpp
  ${DEVICE_OS_DIR}/hal/src/gcc/rgbled_hal.cpp
  ${DEVICE_OS_DIR}/system/src/system_led_signal.cpp
  simple_file_storage.cpp
  str_util.cpp
  varint.cpp
  service_bytes2hex.cpp
  diagnostics.cpp
  rgbled.cpp
  pool_allocator.cpp
  led_service.cpp
  fixed_queue.cpp
  eeprom_emulation.cpp
  main.cpp
)

get_filename_component(CURRENT_TEST_DIRECTORY_FULL "${CMAKE_CURRENT_SOURCE_DIR}"
    ABSOLUTE)

# Set defines specific to target
target_compile_definitions( ${target_name}
  PRIVATE PLATFORM_ID=3
  PRIVATE HAL_PLATFORM_FILESYSTEM=1
  PRIVATE FIXTURES_DIRECTORY="${CURRENT_TEST_DIRECTORY_FULL}/fixtures"
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
  PRIVATE ${DEVICE_OS_DIR}/dynalib/inc
  PRIVATE ${DEVICE_OS_DIR}/hal/shared
  PRIVATE ${DEVICE_OS_DIR}/hal/src/gcc
  PRIVATE ${DEVICE_OS_DIR}/system/inc
  PRIVATE ${DEVICE_OS_DIR}/wiring/inc
  PRIVATE ${DEVICE_OS_DIR}/hal/
  PRIVATE ${DEVICE_OS_DIR}/hal/network/ncp/
  PRIVATE ${DEVICE_OS_DIR}/hal/network/ncp/at_parser/
  PRIVATE ${THIRD_PARTY_DIR}/hippomocks
)

# Link against dependencies specific to target

# Add tests to `test` target
catch_discover_tests( ${target_name}
  TEST_PREFIX ${target_name}_
)

add_subdirectory(logging)
