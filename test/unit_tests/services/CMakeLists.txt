set(target_name services)

# Create test executable
add_executable( ${target_name}
  ${DEVICE_OS_DIR}/services/src/str_util.cpp
  str_util.cpp
  varint.cpp
  main.cpp
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
  PRIVATE ${DEVICE_OS_DIR}/services/inc
)

# Link against dependencies specific to target

# Add tests to `test` target
catch_discover_tests( ${target_name}
  TEST_PREFIX ${target_name}_
)
