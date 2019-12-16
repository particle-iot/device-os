cmake_minimum_required(VERSION 3.16)

project(libparticle_wiring_globals)

add_library(
    particle_wiring_globals
    src/spark_wiring_gpio.cpp
    src/spark_wiring_watchdog.cpp
    src/wiring_globals_cloud.cpp
    src/wiring_globals_eeprom.cpp
    src/wiring_globals_i2c.cpp
    src/wiring_globals_ipaddress.cpp
    src/wiring_globals_spi.cpp
)

target_link_libraries(
    particle_wiring_globals
    PRIVATE particle::wiring
)
