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
  coap_reliability.cpp
  coap.cpp
  forward_message_channel.cpp
  hal_stubs.cpp
  messages.cpp
  ping.cpp
  protocol.cpp
  publisher.cpp
)

# Set defines specific to target
target_compile_definitions( ${target_name}
  PRIVATE PLATFORM_ID=3
)

# Set compiler flags specific to target
target_compile_options( ${target_name}
  PRIVATE -fno-inline -fprofile-arcs -ftest-coverage -O0 -g
)

# Set include path specific to target
target_include_directories( ${target_name}
  PRIVATE ${THIRD_PARTY_DIR}/fakeit/fakeit/single_header/catch/
  PRIVATE ${DEVICE_OS_DIR}/communication/inc/
  PRIVATE ${DEVICE_OS_DIR}/communication/src/
  PRIVATE ${DEVICE_OS_DIR}/hal/inc/
  PRIVATE ${DEVICE_OS_DIR}/hal/shared/
  PRIVATE ${DEVICE_OS_DIR}/hal/src/gcc/
  PRIVATE ${DEVICE_OS_DIR}/services/inc/
  PRIVATE ${DEVICE_OS_DIR}/wiring/inc/
  PRIVATE ${TEST_DIR}/
)

# Link against dependencies specific to target

# Add tests to `test` target
catch_discover_tests( ${target_name}
  TEST_PREFIX ${target_name}_
)
