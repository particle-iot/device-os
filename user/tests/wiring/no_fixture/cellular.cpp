/**
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include "application.h"
#include "unit-test/unit-test.h"
#include "socket_hal.h"

#if Wiring_Cellular == 1
bool skip_r410 = false;

/**
 * Returns current modem type:
 * DEV_UNKNOWN, DEV_SARA_G350, DEV_SARA_U260, DEV_SARA_U270, DEV_SARA_U201, DEV_SARA_R410
 */
int cellular_modem_type() {
    CellularDevice device;
    memset(&device, 0, sizeof(device));
    device.size = sizeof(device);
    cellular_device_info(&device, NULL);

    return device.dev;
}

/* Scenario: The device will connect to the Cloud even when all
 *           TCP socket types are consumed
 *
 * Given the device is currently disconnected from the Cloud
 * When all available TCP sockets are consumed
 * And the device attempts to connect to the Cloud
 * Then the device overcomes this socket obstacle and connects to the Cloud
 */
void disconnect_from_cloud(system_tick_t timeout, bool detach = false)
{
    Particle.disconnect();
    waitFor(Particle.disconnected, timeout);

    Cellular.disconnect();
    // Avoids some sort of race condition in AUTOMATIC mode
    delay(1000);

    if (detach) {
        Cellular.command(timeout, "AT+COPS=2,2\r\n");
    }
}
void connect_to_cloud(system_tick_t timeout)
{
    Particle.connect();
    waitFor(Particle.connected, timeout);
}

#if !HAL_USE_SOCKET_HAL_POSIX

void consume_all_sockets(uint8_t protocol)
{
    static int port = 9000;
    int socket_handle;
    do {
        socket_handle = socket_create(AF_INET, SOCK_STREAM, protocol==IPPROTO_UDP ? IPPROTO_UDP : IPPROTO_TCP, port++, NIF_DEFAULT);
    } while(socket_handle_valid(socket_handle));
}
test(CELLULAR_01_device_will_connect_to_the_cloud_when_all_tcp_sockets_consumed) {
    //Serial.println("the device will connect to the cloud when all tcp sockets are consumed");
    // Given the device is currently disconnected from the Cloud
    disconnect_from_cloud(30*1000);
    // When all available TCP sockets are consumed
    consume_all_sockets(IPPROTO_TCP);
    // And the device attempts to connect to the Cloud
    connect_to_cloud(6*60*1000);
    // Then the device overcomes this socket obstacle and connects to the Cloud
    assertEqual(Particle.connected(), true);
}
/* Scenario: The device will connect to the Cloud even when all
 *           UDP socket types are consumed
 *
 * Given the device is currently disconnected from the Cloud
 * When all available UDP sockets are consumed
 * And the device attempts to connect to the Cloud
 * Then the device overcomes this socket obstacle and connects to the Cloud
 */
test(CELLULAR_02_device_will_connect_to_the_cloud_when_all_udp_sockets_consumed) {
    //Serial.println("the device will connect to the cloud when all udp sockets are consumed");
    // Given the device is currently disconnected from the Cloud
    disconnect_from_cloud(30*1000);
    // When all available UDP sockets are consumed
    consume_all_sockets(IPPROTO_UDP);
    // And the device attempts to connect to the Cloud
    connect_to_cloud(6*60*1000);
    // Then the device overcomes this socket obstacle and connects to the Cloud
    assertEqual(Particle.connected(), true);
}

test(CELLULAR_03_close_consumed_sockets) {
    for (int i = 0; i < 7; i++) {
        if (socket_handle_valid(i))
            socket_close(i);
    }
}

#endif // !HAL_USE_SOCKET_HAL_POSIX

void checkIPAddress(const char* name, const IPAddress& address)
{
    if (address.version()==0 || address[0]==0)
    {
        Serial.print("address failed:");
        Serial.println(name);
        assertNotEqual(address.version(), 0);
        assertNotEqual(address[0], 0);
    }
}

test(CELLULAR_04_local_ip_cellular_config)
{
    connect_to_cloud(6*60*1000);
    checkIPAddress("local", Cellular.localIP());
}

test(CELLULAR_05_resolve) {
    connect_to_cloud(6*60*1000);
    checkIPAddress("www.google.com", Cellular.resolve("www.google.com"));
}

test(CELLULAR_06_resolve) {
    connect_to_cloud(6*60*1000);
    IPAddress addr = Cellular.resolve("this.is.not.a.real.host");
    assertEqual(addr, 0);
}

// FIXME: This test is failing on Gen 3 Boron / B SoM as of 1.1.0 or earlier
test(CELLULAR_07_rssi_is_valid) {
    connect_to_cloud(6*60*1000);
    CellularSignal s;
    for (int x = 0; x < 10; x++) {
        s = Cellular.RSSI();
        if (s.rssi < 0) {
            break;
        }
        Serial.println(s);
        delay(5000);
    }
    assertLessOrEqual(s.rssi, -20);
    assertMoreOrEqual(s.rssi, -150);
}

#define LOREM "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Quisque ut elit nec mi bibendum mollis. Nam nec nisl mi. Donec dignissim iaculis purus, ut condimentum arcu semper quis. Phasellus efficitur ut arcu ac dignissim. In interdum sem id dictum luctus. Ut nec mattis sem. Nullam in aliquet lacus. Donec egestas nisi volutpat lobortis sodales. Aenean elementum magna ipsum, vitae pretium tellus lacinia eu. Phasellus commodo nisi at quam tincidunt, tempor gravida mauris facilisis. Duis tristique ligula ac pulvinar consectetur. Cras aliquam, leo ut eleifend molestie, arcu odio semper odio, quis sollicitudin metus libero et lorem. Donec venenatis congue commodo. Vivamus mattis elit metus, sed fringilla neque viverra eu. Phasellus leo urna, elementum vel pharetra sit amet, auctor non sapien. Phasellus at justo ac augue rutrum vulputate. In hac habitasse platea dictumst. Pellentesque nibh eros, placerat id laoreet sed, dapibus efficitur augue. Praesent pretium diam ac sem varius fermentum. Nunc suscipit dui risus sed"

test(MDM_01_socket_writes_with_length_more_than_1023_work_correctly) {
    // https://github.com/spark/firmware/issues/1104
    const char request[] =
        "POST /post HTTP/1.1\r\n"
        "Host: httpbin.org\r\n"
        "Connection: close\r\n"
        "Content-Type: multipart/form-data; boundary=-------------aaaaaaaa\r\n"
        "Content-Length: 1124\r\n"
        "\r\n"
        "---------------aaaaaaaa\r\n"
        "Content-Disposition: form-data; name=\"field\"\r\n"
        "\r\n"
        LOREM "\r\n"
        "---------------aaaaaaaa--\r\n";
    const int requestSize = sizeof(request) - 1;

    Cellular.connect();
    waitFor(Cellular.ready, 120000);

    TCPClient c;
    int res = c.connect("httpbin.org", 80);
    (void)res;

    int sz = c.write((const uint8_t*)request, requestSize);
    assertEqual(sz, requestSize);

    char* responseBuf = new char[2048];
    memset(responseBuf, 0, 2048);
    int responseSize = 0;
    uint32_t mil = millis();
    while(1) {
        while (c.available()) {
            responseBuf[responseSize++] = c.read();
        }
        if (!c.connected())
            break;
        if (millis() - mil >= 60000) {
            break;
        }
    }

    bool contains = false;
    if (responseSize > 0 && !c.connected()) {
        contains = strstr(responseBuf, LOREM) != nullptr;
    }

    delete responseBuf;

    assertTrue(contains);
}

// TODO: Cellular.command() is not implemented on Gen 3 devices
#if !HAL_PLATFORM_NCP

static int atCallback(int type, const char* buf, int len, int* lines) {
    if (len && type == TYPE_UNKNOWN)
        (*lines)++;
    return WAIT;
}

test(MDM_02_at_commands_with_long_response_are_correctly_parsed_and_flow_controlled) {
    // TODO: Add back this test when SARA R410 supports HW Flow Control?
    if (cellular_modem_type() == DEV_SARA_R410) {
        skip_r410 = true;
        Serial.println("TODO: Add back this test when SARA R410 supports HW Flow Control?");
        skip();
        return;
    }
    // https://github.com/spark/firmware/issues/1138
    int lines = 0;
    int ret = -99999;
    // Disconnected from the Cloud so we are not dealing with any other command responses
    Particle.disconnect();
    waitFor(Particle.disconnected, 30000);

    while ((ret = Cellular.command(atCallback, &lines, 10000, "AT+CLAC\r\n")) == WAIT);
    assertEqual(ret, (int)RESP_OK);
    assertMoreOrEqual(lines, 200);
}

#endif // !HAL_PLATFORM_NCP

#endif // Wiring_Cellular == 1
