set(target_name system)

# Create test executable
add_executable( ${target_name}
  ${DEVICE_OS_DIR}/system/src/system_info.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_json.cpp
  ${DEVICE_OS_DIR}/hal/src/gcc/timer_hal.cpp
  ${DEVICE_OS_DIR}/hal/src/gcc/usb_hal.cpp
  ${DEVICE_OS_DIR}/hal/src/template/deviceid_hal.cpp
  ${DEVICE_OS_DIR}/hal/src/gcc/interrupts_hal.cpp
  ${DEVICE_OS_DIR}/services/src/jsmn.c
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_string.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_buffer.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_print.cpp
  ${DEVICE_OS_DIR}/wiring/src/string_convert.cpp
  ${DEVICE_OS_DIR}/system/src/system_utilities.cpp
  ${DEVICE_OS_DIR}/system/src/active_object.cpp
  ${DEVICE_OS_DIR}/system/src/control_request_handler.cpp
  ${DEVICE_OS_DIR}/system/src/usb_control_request_channel.cpp
  ${DEVICE_OS_DIR}/system/src/system_string_interpolate.cpp
  ${DEVICE_OS_DIR}/system/src/server_config.cpp
  ${TEST_DIR}/mock/system_info_mock.cpp
  ${TEST_DIR}/mock/core_hal_mock.cpp
  ${TEST_DIR}/mock/dct_hal_mock.cpp
  ${TEST_DIR}/mock/mbedtls_mock.cpp
  ${TEST_DIR}/stub/mbedtls/md.cpp
  ${TEST_DIR}/stub/mbedtls/pk.cpp
  ${TEST_DIR}/stub/mbedtls/asn1.cpp
  ${TEST_DIR}/stub/mbedtls_util.cpp
  ${TEST_DIR}/stub/system_mode.cpp
  ${TEST_DIR}/stub/core_hal.cpp
  ${TEST_DIR}/stub/system_pool.cpp
  ${TEST_DIR}/stub/system_cloud_internal.cpp
  ${TEST_DIR}/stub/test_malloc.cpp
  ${TEST_DIR}/stub/system_cloud.cpp
  ${TEST_DIR}/stub/system_network.cpp
  ${TEST_DIR}/stub/dct_hal.cpp
  ${TEST_DIR}/stub/security_mode.cpp
  ${TEST_DIR}/util/random.cpp
  ${TEST_DIR}/util/alloc.cpp
  ${TEST_DIR}/util/buffer.cpp
  ${TEST_DIR}/util/random_old.cpp
  system_info.cpp
  module_info.c
  stubs.cpp
  system_task.cpp
  string_interpolate.cpp
  usb_control_request_channel.cpp
  server_config.cpp
)

file(STRINGS "${DEVICE_OS_DIR}/build/version.mk" VERSION_STRING REGEX "^VERSION_STRING[ \t\r\n]*=[ \t\r\n]*(.*)$")

string(REGEX MATCH "^VERSION_STRING[ \t\r\n]*=[ \t\r\n]*(.*)$" _ ${VERSION_STRING})
set(VERSION_STRING ${CMAKE_MATCH_1})

# Set defines specific to target
target_compile_definitions( ${target_name}
  PRIVATE PLATFORM_ID=3
  PRIVATE SYSTEM_VERSION_STRING=${VERSION_STRING}
  PRIVATE HAL_PLATFORM_PROTOBUF=0
)

# Set compiler flags specific to target
target_compile_options( ${target_name}
  PRIVATE ${COVERAGE_CFLAGS}
)

# Set include path specific to target
target_include_directories( ${target_name}
  PRIVATE ${TEST_DIR}
  PRIVATE ${TEST_DIR}/stub/
  PRIVATE ${TEST_DIR}/mock
  PRIVATE ${DEVICE_OS_DIR}/communication/inc/
  PRIVATE ${DEVICE_OS_DIR}/hal/inc/
  PRIVATE ${DEVICE_OS_DIR}/hal/shared/
  PRIVATE ${DEVICE_OS_DIR}/services/inc/
  PRIVATE ${DEVICE_OS_DIR}/crypto/inc
  PRIVATE ${DEVICE_OS_DIR}/system/inc/
  PRIVATE ${DEVICE_OS_DIR}/system/src/
  PRIVATE ${DEVICE_OS_DIR}/wiring/inc/
  PRIVATE ${DEVICE_OS_DIR}/dynalib/inc/
  PRIVATE ${DEVICE_OS_DIR}/hal/src/gcc/
  PRIVATE ${THIRD_PARTY_DIR}/fakeit/fakeit/single_header/catch
  PRIVATE ${THIRD_PARTY_DIR}/hippomocks
)

# Link against dependencies specific to target

# Add tests to `test` target
catch_discover_tests( ${target_name}
  TEST_PREFIX ${target_name}_
)
