cmake_minimum_required(VERSION 3.16)

project(libparticle_dynalib)

add_library(
    particle_dynalib INTERFACE
)

target_include_directories(
    particle_dynalib
    INTERFACE inc
)
