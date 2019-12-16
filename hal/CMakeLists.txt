cmake_minimum_required(VERSION 3.16)

project(libparticle_hal)

add_library(
    particle_hal
    src/core/adc_hal.c
    src/core/bkpreg_hal.c
    src/core/core_hal.c
    src/core/core_subsys_hal.c
    src/core/dac_hal.c
    src/core/delay_hal.c
    src/core/deviceid_hal.cpp
    src/core/gpio_hal.c
    src/core/hal_dynalib_export.c
    src/core/i2c_hal.c
    src/core/inet_hal.c
    src/core/interrupts_hal.c
    src/core/memory_hal.cpp
    src/core/newlib_stubs.cpp
    src/core/ota_flash_hal.c
    src/core/pinmap_hal.c
    src/core/product_store_hal.c
    src/core/rng_hal.c
    src/core/rtc_hal.c
    src/core/servo_hal.c
    src/core/socket_hal.c
    src/core/spi_hal.c
    src/core/stm32_it.c
    src/core/syshealth_hal.c
    src/core/timer_hal.c
    src/core/tone_hal.c
    src/core/usart_hal.c
    src/core/usb_desc.c
    src/core/usb_hal.c
    src/core/usb_istr.c
    src/core/usb_prop.c
    src/core/wlan_hal.c
    src/stm32/eeprom_hal.cpp
    src/stm32/hal_event.cpp
    src/stm32/newlib.cpp
    src/stm32/ota_module.cpp
    src/stm32/pwm_hal.c
    src/stm32/system_interrupts.cpp
)

target_compile_features(
    particle_hal
    PUBLIC cxx_constexpr
)

target_include_directories(
    particle_hal
    PUBLIC inc
    PUBLIC shared
    PUBLIC src/core
    PUBLIC src/stm32
)

target_link_libraries(
    particle_hal
    PUBLIC particle::dynalib
    PUBLIC particle::services
)
