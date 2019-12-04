cmake_minimum_required(VERSION 3.16)

project(libparticle_crypto)

add_library(
    particle_crypto
    src/mbedtls_util.cpp
)

target_include_directories(
    particle_crypto
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/inc
#[=[
Items below this comment should be removed in favor of consuming Particle
libraries via the `target_link_libraries` command. Currently, the dependency
graph is cyclic and this cannot be reasonably achieved at this time.
#]=]
)

target_link_libraries(
    particle_crypto
    PRIVATE mbedtls::mbedtls
)
