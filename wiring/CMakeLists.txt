cmake_minimum_required(VERSION 3.16)

project(libparticle_wiring)

add_library(
    particle_wiring
    src/rtt_output_stream.cpp
    src/spark_wiring_async.cpp
    src/spark_wiring_ble.cpp
    src/spark_wiring_can.cpp
    src/spark_wiring_cellular_printable.cpp
    src/spark_wiring_cellular.cpp
    src/spark_wiring_cloud.cpp
    src/spark_wiring_eeprom.cpp
    src/spark_wiring_ethernet.cpp
    src/spark_wiring_fuel.cpp
    src/spark_wiring_i2c.cpp
    src/spark_wiring_interrupts.cpp
    src/spark_wiring_ipaddress.cpp
    src/spark_wiring_json.cpp
    src/spark_wiring_led.cpp
    src/spark_wiring_logging.cpp
    src/spark_wiring_mesh.cpp
    src/spark_wiring_network.cpp
    src/spark_wiring_nfc.cpp
    src/spark_wiring_power.cpp
    src/spark_wiring_print.cpp
    src/spark_wiring_random.cpp
    src/spark_wiring_rgb.cpp
    src/spark_wiring_servo.cpp
    src/spark_wiring_spi.cpp
    src/spark_wiring_stream.cpp
    src/spark_wiring_string.cpp
    src/spark_wiring_system.cpp
    src/spark_wiring_tcpclient_posix.cpp
    src/spark_wiring_tcpclient.cpp
    src/spark_wiring_tcpserver_posix.cpp
    src/spark_wiring_tcpserver.cpp
    src/spark_wiring_time.cpp
    src/spark_wiring_tone.cpp
    src/spark_wiring_udp_posix.cpp
    src/spark_wiring_udp.cpp
    src/spark_wiring_usartserial.cpp
    src/spark_wiring_usbkeyboard.cpp
    src/spark_wiring_usbmouse.cpp
    src/spark_wiring_usbserial.cpp
    src/spark_wiring_wifi.cpp
    src/spark_wiring_wifitester.cpp
    src/spark_wiring.cpp
    src/string_convert.cpp
    src/user.cpp
)

target_include_directories(
    particle_wiring
    PUBLIC inc
)

target_link_libraries(
    particle_wiring
# Leaking `interrupts_hal.h` implementation details into public interface
    PUBLIC particle::hal
# Leaking `system_network.h` implementation details into public interface
    PUBLIC particle::system
)

target_compile_features(
    particle_wiring
    PUBLIC cxx_variadic_templates
)
