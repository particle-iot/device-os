set(target_name hal)

# Create test executable
add_executable( ${target_name}
  inflate.cpp
  sparse_buffer.cpp
  ${DEVICE_OS_DIR}/hal/shared/inflate.cpp
  ${DEVICE_OS_DIR}/hal/shared/inflate_impl.cpp
  ${DEVICE_OS_DIR}/third_party/miniz/miniz/miniz_tinfl.c
)

# Set defines specific to target
target_compile_definitions( ${target_name}
  PRIVATE PLATFORM_ID=3
  PRIVATE HAL_PLATFORM_COMPRESSED_OTA=1
)

# Set include path specific to target
target_include_directories( ${target_name}
  PRIVATE ${TEST_DIR}
  PRIVATE ${DEVICE_OS_DIR}/hal/inc
  PRIVATE ${DEVICE_OS_DIR}/hal/shared
  PRIVATE ${DEVICE_OS_DIR}/hal/src/nRF52840
  PRIVATE ${DEVICE_OS_DIR}/hal/src/gcc
  PRIVATE ${DEVICE_OS_DIR}/services/inc
  PRIVATE ${DEVICE_OS_DIR}/third_party/miniz/miniz
)

# Link against dependencies specific to target
target_link_libraries( ${target_name}
  z
)

# Add tests to `test` target
catch_discover_tests( ${target_name}
  TEST_PREFIX ${target_name}_
)

add_subdirectory(simple_ntp_client)
