cmake_minimum_required(VERSION 3.16)

project(libparticle_newlib)

add_library(
    particle_newlib
    src/malloc.cpp
    src/mallocr.c
#[=[
Items below this comment have been added to resolve a cyclic dependency.
Any items added below should have a related Clubhouse story identifying the
nature of the dependency and the steps required to resolve it.
#]=]
)

target_link_libraries(
    particle_newlib
    PRIVATE particle::hal
)
