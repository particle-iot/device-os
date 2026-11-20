set(target_name dtls)

file(GLOB MBEDTLS_SOURCES ${THIRD_PARTY_DIR}/mbedtls/mbedtls/library/*.c)

# Create test executable
add_executable( ${target_name}
  ${MBEDTLS_SOURCES}
  ${DEVICE_OS_DIR}/crypto/src/mbedtls_util.cpp
  ${DEVICE_OS_DIR}/communication/src/dtls_message_channel.cpp
  ${DEVICE_OS_DIR}/communication/src/chunked_transfer.cpp
  ${DEVICE_OS_DIR}/communication/src/coap.cpp
  ${DEVICE_OS_DIR}/communication/src/coap_channel.cpp
  ${DEVICE_OS_DIR}/communication/src/communication_diagnostic.cpp
  ${DEVICE_OS_DIR}/communication/src/subscriptions.cpp
  ${DEVICE_OS_DIR}/communication/src/messages.cpp
  ${DEVICE_OS_DIR}/communication/src/protocol.cpp
  ${DEVICE_OS_DIR}/communication/src/publisher.cpp
  ${DEVICE_OS_DIR}/communication/src/variables.cpp
  ${DEVICE_OS_DIR}/communication/src/coap_defs.cpp
  ${DEVICE_OS_DIR}/communication/src/coap_util.cpp
  ${DEVICE_OS_DIR}/communication/src/coap_message_encoder.cpp
  ${DEVICE_OS_DIR}/communication/src/coap_message_decoder.cpp
  ${DEVICE_OS_DIR}/communication/src/firmware_update.cpp
  ${DEVICE_OS_DIR}/communication/src/description.cpp
  ${DEVICE_OS_DIR}/communication/src/protocol_util.cpp
  ${DEVICE_OS_DIR}/communication/src/protocol_defs.cpp
  ${DEVICE_OS_DIR}/communication/src/v2/coap_channel.cpp
  ${DEVICE_OS_DIR}/communication/src/v2/coap_payload.cpp
  ${DEVICE_OS_DIR}/communication/src/v2/coap_options.cpp
  ${DEVICE_OS_DIR}/communication/src/v2/coap_tag.cpp
  ${DEVICE_OS_DIR}/services/src/system_error.cpp
  ${DEVICE_OS_DIR}/services/src/jsmn.c
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_json.cpp
  ${TEST_DIR}/stub/filesystem.cpp
  ${TEST_DIR}/communication/util/coap_message.cpp
  ${TEST_DIR}/communication/hal_stubs.cpp
  dtls_message_channel.cpp
  main.cpp
)

# Set defines specific to target
target_compile_definitions( ${target_name}
  PRIVATE PLATFORM_ID=3
  PRIVATE HAL_PLATFORM_OTA_PROTOCOL_V3=1
  PRIVATE HAL_PLATFORM_ERROR_MESSAGES=1
  PRIVATE "MBEDTLS_CONFIG_FILE=\"mbedtls_config.h\""
  PRIVATE "MBEDTLS_USER_CONFIG_FILE=\"mbedtls_test_config.h\""
)

# Set compiler flags specific to target
target_compile_options( ${target_name}
  PRIVATE ${COVERAGE_CFLAGS}
)

# Set include path specific to target
# NOTE: The real mbedtls headers must come before ${TEST_DIR}/stub, which has its own mbedtls_config.h
# and mbedtls/*.h
target_include_directories( ${target_name}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE ${DEVICE_OS_DIR}/crypto/inc
  PRIVATE ${DEVICE_OS_DIR}/hal/src/gcc/mbedtls
  PRIVATE ${THIRD_PARTY_DIR}/mbedtls/mbedtls/include
  PRIVATE ${TEST_DIR}/communication
  PRIVATE ${TEST_DIR}/stub
  PRIVATE ${TEST_DIR}
  PRIVATE ${DEVICE_OS_DIR}/communication/inc
  PRIVATE ${DEVICE_OS_DIR}/communication/src
  PRIVATE ${DEVICE_OS_DIR}/hal/inc
  PRIVATE ${DEVICE_OS_DIR}/hal/shared
  PRIVATE ${DEVICE_OS_DIR}/hal/src/gcc
  PRIVATE ${DEVICE_OS_DIR}/services/inc
  PRIVATE ${DEVICE_OS_DIR}/wiring/inc
  PRIVATE ${DEVICE_OS_DIR}/system/inc
  PRIVATE ${DEVICE_OS_DIR}/dynalib/inc
  PRIVATE ${DEVICE_OS_DIR}/platform/MCU/gcc/inc
)

# Add tests to `test` target
catch_discover_tests( ${target_name}
  TEST_PREFIX ${target_name}_
)
