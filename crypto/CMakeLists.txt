cmake_minimum_required(VERSION 3.16)

project(libparticle_crypto)

add_library(
    particle_crypto
    src/mbedtls_util.cpp
)

target_include_directories(
    particle_crypto
    PUBLIC inc
)

target_link_libraries(
    particle_crypto
# Leaking `mbedtls` implementation details into public interface
    PUBLIC mbedtls::mbedtls
    PRIVATE particle::hal
)
