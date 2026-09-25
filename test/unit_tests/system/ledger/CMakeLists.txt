set(target_name ledger)

# Create test executable
add_executable( ${target_name}
  ${DEVICE_OS_DIR}/system/src/ledger/ledger_manager.cpp
  ${DEVICE_OS_DIR}/system/src/ledger/ledger.cpp
  ${DEVICE_OS_DIR}/system/src/ledger/ledger_util.cpp
  ${DEVICE_OS_DIR}/services/src/file_util.cpp
  ${DEVICE_OS_DIR}/services/src/nanopb_misc.c
  ${DEVICE_OS_DIR}/services/src/system_error.cpp
  ${DEVICE_OS_DIR}/services/src/call_once.cpp
  ${DEVICE_OS_DIR}/services/src/jsmn.c
  ${DEVICE_OS_DIR}/proto_defs/src/cloud/cloud.pb.c
  ${DEVICE_OS_DIR}/proto_defs/src/cloud/ledger.pb.c
  ${DEVICE_OS_DIR}/proto_defs/src/ledger.pb.c
  ${THIRD_PARTY_DIR}/nanopb/nanopb/pb_common.c
  ${THIRD_PARTY_DIR}/nanopb/nanopb/pb_encode.c
  ${THIRD_PARTY_DIR}/nanopb/nanopb/pb_decode.c
  ${THIRD_PARTY_DIR}/littlefs/littlefs/lfs.c
  # The real CoAP stack: the tests exchange real CoAP messages with the v2 channel
  ${DEVICE_OS_DIR}/communication/src/v2/coap_channel.cpp
  ${DEVICE_OS_DIR}/communication/src/v2/coap_payload.cpp
  ${DEVICE_OS_DIR}/communication/src/v2/coap_options.cpp
  ${DEVICE_OS_DIR}/communication/src/v2/coap_tag.cpp
  ${DEVICE_OS_DIR}/communication/src/v2/coap_api.cpp
  ${DEVICE_OS_DIR}/communication/src/coap_message_encoder.cpp
  ${DEVICE_OS_DIR}/communication/src/coap_message_decoder.cpp
  ${DEVICE_OS_DIR}/communication/src/coap_util.cpp
  ${DEVICE_OS_DIR}/communication/src/coap_defs.cpp
  ${DEVICE_OS_DIR}/communication/src/protocol_util.cpp
  ${DEVICE_OS_DIR}/communication/src/protocol_defs.cpp
  ${DEVICE_OS_DIR}/communication/src/coap.cpp
  ${DEVICE_OS_DIR}/communication/src/coap_channel.cpp
  ${DEVICE_OS_DIR}/communication/src/protocol.cpp
  ${DEVICE_OS_DIR}/communication/src/messages.cpp
  ${DEVICE_OS_DIR}/communication/src/subscriptions.cpp
  ${DEVICE_OS_DIR}/communication/src/description.cpp
  ${DEVICE_OS_DIR}/communication/src/publisher.cpp
  ${DEVICE_OS_DIR}/communication/src/variables.cpp
  ${DEVICE_OS_DIR}/communication/src/chunked_transfer.cpp
  ${DEVICE_OS_DIR}/communication/src/firmware_update.cpp
  ${DEVICE_OS_DIR}/communication/src/communication_diagnostic.cpp
  ${DEVICE_OS_DIR}/wiring/src/spark_wiring_json.cpp
  ${TEST_DIR}/communication/util/coap_message.cpp
  ${TEST_DIR}/communication/util/coap_message_channel.cpp
  util/coap_api_fake.cpp
  util/filesystem_ram.cpp
  util/system_fakes.cpp
  util/ledger_pb.cpp
  ledger_manager.cpp
  main.cpp
)

# Set defines specific to target
target_compile_definitions( ${target_name}
  PRIVATE PLATFORM_ID=3
  PRIVATE HAL_PLATFORM_ERROR_MESSAGES=1
  PRIVATE HAL_PLATFORM_OTA_PROTOCOL_V3=1
  PRIVATE MBEDTLS_SSL_MAX_CONTENT_LEN=1500
  PRIVATE LFS_CONFIG=lfs_config.h
  PRIVATE LFS_NO_DEBUG
  PRIVATE LFS_NO_WARN
  PRIVATE LFS_NO_ERROR
  PRIVATE LFS_NO_ASSERT
)

# Set compiler flags specific to target
target_compile_options( ${target_name}
  PRIVATE ${COVERAGE_CFLAGS}
)

# Set include path specific to target
# NOTE: The real LittleFS headers must come before ${TEST_DIR}/stub, which has its own lfs.h and
# lfs_util.h. The stub directory must in turn come before hal/src/gcc/littlefs so that the stub
# filesystem_impl.h is used. communication/src must come before system/src: protocol.h includes
# firmware_update.h, which is ambiguous otherwise
target_include_directories( ${target_name}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE ${THIRD_PARTY_DIR}/littlefs/littlefs
  PRIVATE ${TEST_DIR}/stub
  PRIVATE ${DEVICE_OS_DIR}/hal/src/gcc/littlefs
  PRIVATE ${TEST_DIR}
  PRIVATE ${TEST_DIR}/communication/util
  PRIVATE ${DEVICE_OS_DIR}/communication/inc
  PRIVATE ${DEVICE_OS_DIR}/communication/src
  PRIVATE ${DEVICE_OS_DIR}/crypto/inc
  PRIVATE ${DEVICE_OS_DIR}/system/inc
  PRIVATE ${DEVICE_OS_DIR}/system/src
  PRIVATE ${DEVICE_OS_DIR}/services/inc
  PRIVATE ${DEVICE_OS_DIR}/hal/inc
  PRIVATE ${DEVICE_OS_DIR}/hal/shared
  PRIVATE ${DEVICE_OS_DIR}/hal/src/gcc
  PRIVATE ${DEVICE_OS_DIR}/wiring/inc
  PRIVATE ${DEVICE_OS_DIR}/dynalib/inc
  PRIVATE ${DEVICE_OS_DIR}/proto_defs/src
  PRIVATE ${THIRD_PARTY_DIR}/nanopb/nanopb
)

# Add tests to `test` target
catch_discover_tests( ${target_name}
  TEST_PREFIX ${target_name}_
)