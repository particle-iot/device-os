# Global defines
add_definitions(-DLOG_DISABLE)

include_directories(
  ${THIRD_PARTY_DIR}/fakeit/fakeit/single_header/catch/
  ${DEVICE_OS_DIR}/communication/src/
  ${DEVICE_OS_DIR}/hal/inc/
  ${DEVICE_OS_DIR}/hal/shared/
  ${DEVICE_OS_DIR}/hal/src/stm32/
  ${DEVICE_OS_DIR}/services/inc/
  ${DEVICE_OS_DIR}/system/inc/
  ${DEVICE_OS_DIR}/system/src/
  ${DEVICE_OS_DIR}/test/
  ${DEVICE_OS_DIR}/wiring/inc/
)

add_executable( publish_vitals
  ${DEVICE_OS_DIR}/system/src/system_publish_vitals.cpp
  publish_vitals.cpp
)

target_link_libraries( publish_vitals
  Catch2::Catch2
)

catch_discover_tests( publish_vitals
  TEST_PREFIX cloud_publish_vitals_
)
