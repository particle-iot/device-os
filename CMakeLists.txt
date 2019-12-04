cmake_minimum_required(VERSION 3.16)

project(libparticle)

# Create namespaced target for mbedtls
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third_party/mbedtls/mbedtls)

#[=[
Extend third-party CMake to expose headers via build target.

The following code block (`target_include_directories`) can be eliminated once
the PR has been merged and refactored code has been propogated to the
third-party dependency.

GitHub PR: https://github.com/ARMmbed/mbedtls/pull/2949
#]=]
target_include_directories(
    mbedtls
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}/third_party/mbedtls/mbedtls/include
)

add_library(mbedtls::mbedtls ALIAS mbedtls)

# Create namespaced target for particle_crypto
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/crypto)
add_library(particle::crypto ALIAS particle_crypto)
