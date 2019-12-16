cmake_minimum_required(VERSION 3.16)

option(PARTICLE_COMPILE_LTO "Enable link time optimization" OFF)
option(PARTICLE_RELEASE_BUILD "Generate release build" ON)

project(libparticle)

set(libparticle_VERSION_MAJOR 1)
set(libparticle_VERSION_MINOR 4)
set(libparticle_VERSION_PATCH 4)

message(STATUS "Loaded toolchain for Particle platform: ${PARTICLE_PLATFORM}")

# Create namespaced target for particle_dynalib
set(PARTICLE_DYNALIB_DIR ${CMAKE_CURRENT_LIST_DIR}/dynalib)
add_subdirectory(${PARTICLE_DYNALIB_DIR})
add_library(particle::dynalib ALIAS particle_dynalib)

# Create namespaced target for particle_platform
set(PARTICLE_PLATFORM_DIR ${CMAKE_CURRENT_LIST_DIR}/platform)
add_subdirectory(${PARTICLE_PLATFORM_DIR})
add_library(particle::platform ALIAS particle_platform)

# Create namespaced target for particle_hal
set(PARTICLE_HAL_DIR ${CMAKE_CURRENT_LIST_DIR}/hal)
add_subdirectory(${PARTICLE_HAL_DIR})
add_library(particle::hal ALIAS particle_hal)

# Create namespaced target for particle_crypto
set(PARTICLE_CRYPTO_DIR ${CMAKE_CURRENT_LIST_DIR}/crypto)
add_subdirectory(${PARTICLE_CRYPTO_DIR})
add_library(particle::crypto ALIAS particle_crypto)

# Create namespaced target for nanopb
add_library(nanopb INTERFACE)
target_include_directories(
    nanopb
    INTERFACE third_party/nanopb/nanopb
)
add_library(nanopb::nanopb ALIAS nanopb)

# Create namespaced target for particle_services
set(PARTICLE_SERVICES_DIR ${CMAKE_CURRENT_LIST_DIR}/services)
add_subdirectory(${PARTICLE_SERVICES_DIR})
add_library(particle::services ALIAS particle_services)

# Create namespaced target for particle_newlib
set(PARTICLE_NEWLIB_DIR ${CMAKE_CURRENT_LIST_DIR}/newlib_nano)
add_subdirectory(${PARTICLE_NEWLIB_DIR})
add_library(particle::newlib ALIAS particle_newlib)

# Create namespaced target for mbedtls
option(ENABLE_PROGRAMS OFF)
option(ENABLE_TESTING OFF)
add_subdirectory(third_party/mbedtls/mbedtls)
unset(ENABLE_PROGRAMS)
unset(ENABLE_TESTING)
#[=[
Extend third-party CMake to expose headers via build target.

The following code block (`target_include_directories`) can be eliminated once
the PR has been merged and refactored code has been propogated to the
third-party dependency.

GitHub PR: https://github.com/ARMmbed/mbedtls/pull/2949
#]=]
target_include_directories(
    mbedtls
    PUBLIC third_party/mbedtls/mbedtls/include
    PRIVATE crypto/inc
)

target_link_libraries(
    mbedtls
    particle::newlib
)

add_library(mbedtls::mbedtls ALIAS mbedtls)

# Create namespaced target for particle_com
set(PARTICLE_COM_DIR ${CMAKE_CURRENT_LIST_DIR}/communication)
add_subdirectory(${PARTICLE_COM_DIR})
add_library(particle::com ALIAS particle_com)

# Create namespaced target for particle_system
set(PARTICLE_SYSTEM_DIR ${CMAKE_CURRENT_LIST_DIR}/system)
add_subdirectory(${PARTICLE_SYSTEM_DIR})
add_library(particle::system ALIAS particle_system)

# Create namespaced target for particle_wiring
set(PARTICLE_WIRING_DIR ${CMAKE_CURRENT_LIST_DIR}/wiring)
add_subdirectory(${PARTICLE_WIRING_DIR})
add_library(particle::wiring ALIAS particle_wiring)

# Create namespaced target for particle_wiring_globals
set(PARTICLE_WIRING_GLOBALS_DIR ${CMAKE_CURRENT_LIST_DIR}/wiring_globals)
add_subdirectory(${PARTICLE_WIRING_GLOBALS_DIR})
add_library(particle::wiring_globals ALIAS particle_wiring_globals)

# Create namespaced target for particle_user
set(PARTICLE_USER_DIR ${CMAKE_CURRENT_LIST_DIR}/user)
add_subdirectory(${PARTICLE_USER_DIR})
add_library(particle::user ALIAS particle_user)

# Create namespaced target for particle_arm_linker
set(PARTICLE_ARM_DIR ${CMAKE_CURRENT_LIST_DIR}/build/arm)
add_subdirectory(${PARTICLE_ARM_DIR})
add_library(particle::arm ALIAS particle_arm)

# Create monolithic executible
set(PARTICLE_MAIN_DIR ${CMAKE_CURRENT_LIST_DIR}/main)
add_subdirectory(${PARTICLE_MAIN_DIR})
add_executable(particle::main ALIAS particle_main)
