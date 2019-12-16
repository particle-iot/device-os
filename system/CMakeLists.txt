cmake_minimum_required(VERSION 3.16)

project(libparticle_system)

add_library(
    particle_system
    src/control/proto/cellular.pb.c
    src/control/proto/cloud.pb.c
    src/control/proto/common.pb.c
    src/control/proto/config.pb.c
    src/control/proto/control.pb.c
    src/control/proto/extensions.pb.c
    src/control/proto/internal.pb.c
    src/control/proto/mesh.pb.c
    src/control/proto/network.pb.c
    src/control/proto/storage.pb.c
    src/control/proto/wifi_new.pb.c
    src/control/proto/wifi.pb.c
    src/control/cellular.cpp
    src/control/cloud.cpp
    src/control/common.cpp
    src/control/config.cpp
    src/control/mesh.cpp
    src/control/network.cpp
    src/control/storage.cpp
    src/control/wifi_new.cpp
    src/control/wifi.cpp
    src/active_object.cpp
    src/ble_control_request_channel.cpp
    src/ble_listening_mode_handler.cpp
    src/control_request_handler.cpp
    src/main.cpp
    src/system_cloud_connection_compat.cpp
    src/system_cloud_connection_posix.cpp
    src/system_cloud_connection.cpp
    src/system_cloud_internal_constants.c
    src/system_cloud_internal.cpp
    src/system_cloud.cpp
    src/system_commands.cpp
    src/system_control_internal.cpp
    src/system_event.cpp
    src/system_led_signal.cpp
    src/system_listening_mode.cpp
    src/system_mode.cpp
    src/system_network_cellular.cpp
    src/system_network_compat.cpp
    src/system_network_diagnostics.cpp
    src/system_network_internal.cpp
    src/system_network_manager_api.cpp
    src/system_network_manager.cpp
    src/system_openthread.cpp
    src/system_power_manager.cpp
    src/system_power.cpp
    src/system_publish_vitals.cpp
    src/system_setup.cpp
    src/system_sleep.cpp
    src/system_string_interpolate.cpp
    src/system_task.cpp
    src/system_threading.cpp
    src/system_update.cpp
    src/system_utilities.cpp
    src/system_ymodem.cpp
    src/usb_control_request_channel.cpp
)

target_compile_features(
    particle_system
    PRIVATE cxx_nullptr
)

target_include_directories(
    particle_system
    PUBLIC inc
#[=[
Items below this comment have been added to resolve a cyclic dependency.
Any items added below should have a related Clubhouse story identifying the
nature of the dependency and the steps required to resolve it.
#]=]
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../wiring/inc
)

target_link_libraries(
    particle_system
    PRIVATE nanopb::nanopb
# Leaking `protocol` implementation details into public interface
    PUBLIC particle::com
    PRIVATE particle::crypto
    PRIVATE particle::hal
)
