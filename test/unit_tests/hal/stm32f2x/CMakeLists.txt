set(target_name hal_stm32f2x)

# Create test executable
add_executable( ${target_name}
  ${TEST_DIR}/stub/stm32f2xx_rtc.c
  ${DEVICE_OS_DIR}/platform/MCU/STM32F2xx/SPARK_Firmware_Driver/src/system_flags_impl.c
  platform_stm32f2xx.cpp
  main.cpp
)

# Set defines specific to target
target_compile_definitions( ${target_name}
  PRIVATE PLATFORM_ID=3
)

# Set include path specific to target
target_include_directories( ${target_name}
  PRIVATE ${TEST_DIR}
  PRIVATE ${TEST_DIR}/stub
  PRIVATE ${DEVICE_OS_DIR}/hal/inc
  PRIVATE ${DEVICE_OS_DIR}/hal/shared
  PRIVATE ${DEVICE_OS_DIR}/services/inc
  PRIVATE ${DEVICE_OS_DIR}/platform/MCU/STM32F2xx/SPARK_Firmware_Driver/inc
  PRIVATE ${THIRD_PARTY_DIR}/hippomocks
)

# Link against dependencies specific to target
target_link_libraries( ${target_name}
)

# Add tests to `test` target
catch_discover_tests( ${target_name}
  TEST_PREFIX ${target_name}_
)
