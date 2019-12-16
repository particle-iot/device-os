cmake_minimum_required(VERSION 3.16)

project(particle_arm ASM)

add_library(
    particle_arm
    startup/startup_stm32f10x_md.S
)

target_include_directories(
    particle_arm
    PRIVATE startup
)

target_compile_options(
    particle_arm
    PRIVATE -fmessage-length=0
    PRIVATE -Wa,--defsym
    PRIVATE -Wa,SPARK_INIT_STARTUP=1
    PRIVATE -x assembler-with-cpp
)

target_link_options(
    particle_arm
    INTERFACE -L${CMAKE_CURRENT_LIST_DIR}/linker
    INTERFACE -T${CMAKE_CURRENT_LIST_DIR}/linker/linker_stm32f10x_md.ld
)
