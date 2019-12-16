cmake_minimum_required(VERSION 3.16)

project(libparticle_user)

add_library(
    particle_user
    src/application.cpp
)

target_compile_features(
    particle_hal
    PUBLIC cxx_std_14
)

target_include_directories(
    particle_user
    PUBLIC inc
)

target_link_libraries(
    particle_user
    PRIVATE particle::system
    PRIVATE particle::wiring
    PRIVATE particle::wiring_globals
)
