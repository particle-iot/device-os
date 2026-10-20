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

add_subdirectory(ledger)

# DeviceKeyManager tests: real LittleFS backed by a RAM block device and the real mbedtls
set(target_name device_key_manager)

set(MBEDTLS_DIR ${THIRD_PARTY_DIR}/mbedtls/mbedtls)
file(GLOB MBEDTLS_SOURCES ${MBEDTLS_DIR}/library/*.c)
add_library(device_key_manager_mbedtls STATIC ${MBEDTLS_SOURCES})
target_include_directories(device_key_manager_mbedtls
  PUBLIC ${MBEDTLS_DIR}/include
)

add_executable( ${target_name}
  ${DEVICE_OS_DIR}/system/src/device_key_manager.cpp
  ${DEVICE_OS_DIR}/communication/src/eckeygen.cpp
  ${DEVICE_OS_DIR}/services/src/file_util.cpp
  ${DEVICE_OS_DIR}/services/src/nanopb_misc.c
  ${DEVICE_OS_DIR}/services/src/system_error.cpp
  ${THIRD_PARTY_DIR}/nanopb/nanopb/pb_common.c
  ${THIRD_PARTY_DIR}/nanopb/nanopb/pb_encode.c
  ${THIRD_PARTY_DIR}/nanopb/nanopb/pb_decode.c
  ${THIRD_PARTY_DIR}/littlefs/littlefs/lfs.c
  ${TEST_DIR}/mock/dct_hal_mock.cpp
  ${TEST_DIR}/stub/dct_hal.cpp
  ledger/util/filesystem_ram.cpp
  device_key_manager.cpp
)

target_compile_definitions( ${target_name}
  PRIVATE PLATFORM_ID=3
  PRIVATE LFS_CONFIG=lfs_config.h
  PRIVATE LFS_NO_DEBUG
  PRIVATE LFS_NO_WARN
  PRIVATE LFS_NO_ERROR
  PRIVATE LFS_NO_ASSERT
)

target_compile_options( ${target_name}
  PRIVATE ${COVERAGE_CFLAGS}
)

# NOTE: The real mbedtls and LittleFS headers must come before ${TEST_DIR}/stub, which has its own
# mbedtls/, lfs.h and lfs_util.h. The stub directory must in turn come before hal/src/gcc/littlefs
# so that the stub filesystem_impl.h is used
target_include_directories( ${target_name}
  PRIVATE ${MBEDTLS_DIR}/include
  PRIVATE ${THIRD_PARTY_DIR}/littlefs/littlefs
  PRIVATE ${TEST_DIR}/stub
  PRIVATE ${DEVICE_OS_DIR}/hal/src/gcc/littlefs
  PRIVATE ${TEST_DIR}
  PRIVATE ${TEST_DIR}/mock
  PRIVATE ${DEVICE_OS_DIR}/communication/inc
  PRIVATE ${DEVICE_OS_DIR}/crypto/inc
  PRIVATE ${DEVICE_OS_DIR}/system/inc
  PRIVATE ${DEVICE_OS_DIR}/system/src
  PRIVATE ${DEVICE_OS_DIR}/services/inc
  PRIVATE ${DEVICE_OS_DIR}/hal/inc
  PRIVATE ${DEVICE_OS_DIR}/hal/shared
  PRIVATE ${DEVICE_OS_DIR}/hal/src/gcc
  PRIVATE ${DEVICE_OS_DIR}/wiring/inc
  PRIVATE ${DEVICE_OS_DIR}/dynalib/inc
  PRIVATE ${THIRD_PARTY_DIR}/nanopb/nanopb
  PRIVATE ${THIRD_PARTY_DIR}/fakeit/fakeit/single_header/catch
)

target_link_libraries( ${target_name}
  device_key_manager_mbedtls
)

catch_discover_tests( ${target_name}
  TEST_PREFIX ${target_name}_
)
