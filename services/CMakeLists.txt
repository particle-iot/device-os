cmake_minimum_required(VERSION 3.16)

project(libparticle_services)

add_library(
    particle_services
    src/addr_util.cpp
    src/atclient.cpp
    src/completion_handler.cpp
    src/debug.c
    src/diagnostics.cpp
    src/file_util.cpp
    src/jsmn.c
    src/led_service.cpp
    src/logging.cpp
    src/nanopb_misc.c
    src/panic.c
    src/rgbled.c
    src/services_dynalib.c
    src/str_util.cpp
    src/stream_util.cpp
    src/stream.cpp
    src/system_error.cpp
    src/thread_runner.cpp
    src/tlv_file.cpp
    src/xmodem_sender.cpp
)

target_compile_features(
    particle_services
    PRIVATE cxx_return_type_deduction
)

target_compile_options(
    particle_services
    INTERFACE -fshort-enums
)

target_include_directories(
    particle_services
    PUBLIC inc
#[=[
Items below this comment have been added to resolve a cyclic dependency.
Any items added below should have a related Clubhouse story identifying the
nature of the dependency and the steps required to resolve it.
#]=]
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../hal/inc
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../hal/shared
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../hal/src/core
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../hal/src/stm32
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../wiring/inc
)

target_link_libraries(
    particle_services
    PRIVATE particle::dynalib
    PUBLIC particle::platform
    PRIVATE nanopb::nanopb
)
