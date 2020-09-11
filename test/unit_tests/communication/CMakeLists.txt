set(target_name communication)

# Create test executable
add_executable( ${target_name}
  ${DEVICE_OS_DIR}/communication/src/chunked_transfer.cpp
  ${DEVICE_OS_DIR}/communication/src/coap.cpp
  ${DEVICE_OS_DIR}/communication/src/coap_channel.cpp
  ${DEVICE_OS_DIR}/communication/src/communication_diagnostic.cpp
  ${DEVICE_OS_DIR}/communication/src/events.cpp
  ${DEVICE_OS_DIR}/communication/src/messages.cpp
  ${DEVICE_OS_DIR}/communication/src/protocol.cpp
  ${DEVICE_OS_DIR}/communication/src/publisher.cpp
  ${DEVICE_OS_DIR}/communication/src/variables.cpp
  ${DEVICE_OS_DIR}/communication/src/coap_defs.cpp
  ${DEVICE_OS_DIR}/communication/src/coap_message_encoder.cpp
  ${DEVICE_OS_DIR}/communication/src/coap_message_decoder.cpp
  ${DEVICE_OS_DIR}/communication/src/firmware_update.cpp
  ${DEVICE_OS_DIR}/communication/src/protocol_util.cpp
  ${DEVICE_OS_DIR}/services/src/system_error.cpp
  ${DEVICE_OS_DIR}/services/src/jsmn.c
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_json.cpp
  util/coap_message.cpp
  util/coap_message_channel.cpp
  util/protocol_callbacks.cpp
  coap_reliability.cpp
  coap.cpp
  forward_message_channel.cpp
  hal_stubs.cpp
  messages.cpp
  ping.cpp
  protocol.cpp
  publisher.cpp
  coap_message_encoder.cpp
  coap_message_decoder.cpp
  firmware_update.cpp
)

# Set defines specific to target
target_compile_definitions( ${target_name}
  PRIVATE PLATFORM_ID=3
  PRIVATE HAL_PLATFORM_OTA_PROTOCOL_V3=1
  PRIVATE HAL_PLATFORM_ERROR_MESSAGES=1
  PRIVATE MBEDTLS_SSL_MAX_CONTENT_LEN=4096
)

# Set compiler flags specific to target
target_compile_options( ${target_name}
  PRIVATE ${COVERAGE_CFLAGS}
)

# Set include path specific to target
target_include_directories( ${target_name}
  PRIVATE ${TEST_DIR}/communication
  PRIVATE ${TEST_DIR}/stub
  PRIVATE ${THIRD_PARTY_DIR}/fakeit/fakeit/single_header/catch
  PRIVATE ${DEVICE_OS_DIR}/communication/inc
  PRIVATE ${DEVICE_OS_DIR}/communication/src
  PRIVATE ${DEVICE_OS_DIR}/hal/inc
  PRIVATE ${DEVICE_OS_DIR}/hal/shared
  PRIVATE ${DEVICE_OS_DIR}/hal/src/gcc
  PRIVATE ${DEVICE_OS_DIR}/services/inc
  PRIVATE ${DEVICE_OS_DIR}/wiring/inc
)

# Link against dependencies specific to target

# Add tests to `test` target
catch_discover_tests( ${target_name}
  TEST_PREFIX ${target_name}_
)
