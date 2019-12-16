cmake_minimum_required(VERSION 3.16)

project(particle_main)

add_executable(
    particle_main
    src/module_info.c
)

target_compile_definitions(
    particle_main
    PRIVATE MODULE_VERSION=1406
    PRIVATE MODULE_FUNCTION=3
    PRIVATE MODULE_DEPENDENCY=0,0,0
    PRIVATE MODULE_DEPENDENCY2=0,0,0
)

target_link_options(
    particle_main
    PRIVATE -Wl,-Map,main.map
)

target_link_libraries(
    particle_main
    PRIVATE particle::arm
    PRIVATE particle::dynalib
    PRIVATE particle::services
)
