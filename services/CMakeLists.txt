cmake_minimum_required(VERSION 3.16)

project(libparticle_services)

add_library(
    particle_services
    src/logging.cpp
)

target_include_directories(
    particle_services
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/inc
)
