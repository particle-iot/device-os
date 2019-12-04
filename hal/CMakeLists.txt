cmake_minimum_required(VERSION 3.16)

project(libparticle_hal)

add_library(
    particle_hal
    src/core/rng_hal.c
    src/electron/rng_hal.c
#    src/gcc/rng_hal.cpp
#    src/mesh-virtual/rng_hal.cpp
    src/nRF52840/rng_hal.cpp
    src/stm32f2xx/rng_hal.c
)

target_include_directories(
    particle_hal
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/inc
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/boron
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../platform/MCU/nRF52840/inc
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../third_party/nrf5_sdk
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../third_party/nrf5_sdk/nrf5_sdk/components/libraries/util
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../third_party/nrf5_sdk/nrf5_sdk/components/softdevice/s140/headers
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../third_party/nrf5_sdk/nrf5_sdk/integration/nrfx/legacy
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../third_party/nrf5_sdk/nrf5_sdk/modules/nrfx
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../third_party/nrf5_sdk/nrf5_sdk/modules/nrfx/drivers/include
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../third_party/nrf5_sdk/nrf5_sdk/modules/nrfx/mdk
#[=[
Items below this comment should be removed in favor of consuming Particle
libraries via the `target_link_libraries` command. Currently, the dependency
graph is cyclic and this cannot be reasonably achieved at this time.
#]=]
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../dynalib/inc
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../services/inc
)

target_compile_definitions(
    particle_hal
    PRIVATE LOG_DISABLE
)
