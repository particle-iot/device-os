set(target_name device_key_manager)

set(MBEDTLS_DIR ${THIRD_PARTY_DIR}/mbedtls/mbedtls)

file(GLOB MBEDTLS_SOURCES ${MBEDTLS_DIR}/library/*.c)
add_library(device_key_manager_mbedtls STATIC ${MBEDTLS_SOURCES})

target_include_directories(device_key_manager_mbedtls
  PUBLIC ${MBEDTLS_DIR}/include
)

add_executable(${target_name}
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
  ${TEST_DIR}/system/mock/filesystem_ram.cpp
  device_key_manager.cpp
)

target_compile_definitions(${target_name}
  PRIVATE PLATFORM_ID=3
  PRIVATE LFS_CONFIG=lfs_config.h
  PRIVATE LFS_NO_DEBUG
  PRIVATE LFS_NO_WARN
  PRIVATE LFS_NO_ERROR
  PRIVATE LFS_NO_ASSERT
)

target_compile_options(${target_name}
  PRIVATE ${COVERAGE_CFLAGS}
)

# NOTE: The real mbedtls and LittleFS headers must come before ${TEST_DIR}/stub, which has its own
# mbedtls/, lfs.h and lfs_util.h. The stub directory must in turn come before hal/src/gcc/littlefs
# so that the stub filesystem_impl.h is used
target_include_directories(${target_name}
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

target_link_libraries(${target_name}
  device_key_manager_mbedtls
)

catch_discover_tests(${target_name}
  TEST_PREFIX ${target_name}_
)
