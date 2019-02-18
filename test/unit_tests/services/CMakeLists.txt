add_executable(
  services
  ${PROJECT_DIR}/services/src/str_util.cpp
  ${COMMON_DIR}/main.cpp
  str_util.cpp
)

include_directories(
  ${PROJECT_DIR}/services/inc
  ${COMMON_DIR}
)

target_link_libraries(services Catch2::Catch2)
catch_discover_tests(services)
