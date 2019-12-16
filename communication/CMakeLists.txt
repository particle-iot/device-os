cmake_minimum_required(VERSION 3.16)

project(libparticle_com)

add_library(
    particle_com
    src/chunked_transfer.cpp
    src/coap_channel.cpp
    src/coap.cpp
    src/communication_diagnostic.cpp
    src/communication_dynalib.cpp
    src/core_protocol.cpp
    src/dsakeygen.cpp
    src/dtls_message_channel.cpp
    src/dtls_protocol.cpp
    src/eckeygen.cpp
    src/events.cpp
    src/handshake.cpp
    src/lightssl_message_channel.cpp
    src/lightssl_protocol.cpp
    src/mbedtls_communication.cpp
    src/mesh.cpp
    src/messages.cpp
    src/protocol_defs.cpp
    src/protocol.cpp
    src/publisher.cpp
    src/spark_protocol_functions.cpp
)

target_compile_features(
    particle_com
    PRIVATE cxx_lambdas
)

target_include_directories(
    particle_com
    PUBLIC inc
#[=[
Items below this comment have been added to resolve a cyclic dependency.
Any items added below should have a related Clubhouse story identifying the
nature of the dependency and the steps required to resolve it.
#]=]
    PRIVATE src
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../wiring/inc
)

target_link_libraries(
    particle_com
    PRIVATE particle::crypto
    PRIVATE particle::hal
)
