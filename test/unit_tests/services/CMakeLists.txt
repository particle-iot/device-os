include_directories(
  ${DEVICE_OS_DIR}/services/inc
)

add_executable(
  services
  ${DEVICE_OS_DIR}/services/src/str_util.cpp
  str_util.cpp
)

target_link_libraries( services
  Catch2::Catch2
)

catch_discover_tests( services
  TEST_PREFIX services_str_util_
)
