set(target_name simple_ntp_client_posix)

# Create test executable
add_executable( ${target_name}
  simple_ntp_client.cpp
  hal_stubs.cpp
  ${DEVICE_OS_DIR}/hal/network/util/simple_ntp_client.cpp
)

# Set defines specific to target
target_compile_definitions( ${target_name}
  PRIVATE PLATFORM_ID=3
  PRIVATE HAL_USE_SOCKET_HAL_POSIX=1
  PRIVATE HAL_USE_INET_HAL_POSIX=1
  PRIVATE HAL_USE_SOCKET_HAL_COMPAT=0
  PRIVATE HAL_USE_INET_HAL_COMPAT=0
)

# Set include path specific to target
target_include_directories( ${target_name}
  PRIVATE ${DEVICE_OS_DIR}/hal/inc
  PRIVATE ${DEVICE_OS_DIR}/hal/shared
  PRIVATE ${DEVICE_OS_DIR}/hal/src/gcc
  PRIVATE ${DEVICE_OS_DIR}/services/inc
  PRIVATE ${DEVICE_OS_DIR}/hal/network/api
  PRIVATE ${DEVICE_OS_DIR}/hal/network/util
  PRIVATE ${THIRD_PARTY_DIR}/hippomocks
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add tests to `test` target
catch_discover_tests( ${target_name}
  TEST_PREFIX ${target_name}_
)
