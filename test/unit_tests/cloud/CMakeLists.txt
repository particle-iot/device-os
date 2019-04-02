add_definitions(-DLOG_DISABLE)

include_directories(
  ${THIRD_PARTY_DIR}/catch2/catch2/single_include/
  ${THIRD_PARTY_DIR}/fakeit/fakeit/single_header/catch/
  ${PROJECT_DIR}/test/
  ${PROJECT_DIR}/hal/shared/
  ${PROJECT_DIR}/services/inc/
  ${PROJECT_DIR}/system/src/
)

add_executable( publish_vitals
  ${PROJECT_DIR}/system/src/system_publish_vitals.cpp
  publish_vitals.cpp
)

catch_discover_tests(publish_vitals)
