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
#include "random.h"
#include "scope_guard.h"

#if Wiring_Cellular == 1
bool skip_r410 = false;

/**
 * Returns current modem type:
 * DEV_UNKNOWN, DEV_SARA_G350, DEV_SARA_U260, DEV_SARA_U270, DEV_SARA_U201, DEV_SARA_R410
 */
int cellular_modem_type() {
    CellularDevice device = {};
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

Vector<sock_handle_t> consume_all_sockets(uint8_t protocol)
{
    static int port = 9000;
    Vector<sock_handle_t> socks;
    for (;;) {
        auto sock = socket_create(AF_INET, SOCK_STREAM, protocol==IPPROTO_UDP ? IPPROTO_UDP : IPPROTO_TCP, port++, NIF_DEFAULT);
        if (!socket_handle_valid(sock)) {
            break;
        }
        socks.append(sock);
    }

    return socks;
}

void close_consumed_sockets(const Vector<sock_handle_t>& socks)
{
    for (const auto sock: socks) {
        socket_close(sock);
    }
}

test(CELLULAR_01_device_will_connect_to_the_cloud_when_all_tcp_sockets_consumed) {
    //Serial.println("the device will connect to the cloud when all tcp sockets are consumed");
    // Given the device is currently disconnected from the Cloud
    disconnect_from_cloud(30*1000);
    // When all available TCP sockets are consumed
    auto socks = consume_all_sockets(IPPROTO_TCP);
    SCOPE_GUARD({
        // Close sockets
        close_consumed_sockets(socks);
    });
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
    auto socks = consume_all_sockets(IPPROTO_UDP);
    SCOPE_GUARD({
        // Close sockets
        close_consumed_sockets(socks);
    });
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

// Collects cellular signal strength and quality and validates Accesstechnology (RAT)
test(CELLULAR_08_sigstr_is_valid) {
    connect_to_cloud(6*60*1000);
    assertTrue(Particle.connected());
    CellularSignal s;
    bool values_in_range = false;
    const int num_retries = 10;
    // Checks 10 times with 500ms gap to get valid signal strength
    for (int x = 0; x < num_retries; x++) {
        s = Cellular.RSSI();
        // Verify that strength and quality values are in range for the given AccessTechnology
        switch (s.getAccessTechnology()) {
            case NET_ACCESS_TECHNOLOGY_GSM:     // GSM strength [-111, -48] and quality [-3.70, -0.60]
                if ((s.getStrengthValue() <= -48.0f && s.getStrengthValue() >= -111.0f)
                    && (s.getQualityValue() <= -0.6f && s.getQualityValue() >= -3.7f)) {
                        values_in_range = true;
                        x = num_retries;
                        break;
                }
                Serial.printlnf("StrV: %.2f, QualV: %.2f, RAT: %d", s.getStrengthValue(), s.getQualityValue(), s.getAccessTechnology());
                break;
            case NET_ACCESS_TECHNOLOGY_EDGE:     // EDGE strength [-111, -48] and quality [-3.70, -0.60]
                if ((s.getStrengthValue() <= -48.0f && s.getStrengthValue() >= -111.0f)
                    && (s.getQualityValue() <= -0.6f && s.getQualityValue() >= -3.7f)) {
                        values_in_range = true;
                        x = num_retries;
                        break;
                }
                Serial.printlnf("StrV: %.2f, QualV: %.2f, RAT: %d", s.getStrengthValue(), s.getQualityValue(), s.getAccessTechnology());
                break;
            case NET_ACCESS_TECHNOLOGY_UTRAN:     // UTRAN strength [-121, -25] and quality [-24.5, 0]
                if ((s.getStrengthValue() <= -25.0f && s.getStrengthValue() >= -121.0f)
                    && (s.getQualityValue() <= -0.0f && s.getQualityValue() >= -25.4f)) {
                        values_in_range = true;
                        x = num_retries;
                        break;
                }
                Serial.printlnf("StrV: %.2f, QualV: %.2f, RAT: %d", s.getStrengthValue(), s.getQualityValue(), s.getAccessTechnology());
                break;
            case NET_ACCESS_TECHNOLOGY_LTE:
            case NET_ACCESS_TECHNOLOGY_LTE_CAT_M1:  // LTE CAT-M1 strength [-141, -44] and quality [-20, -3]
            case NET_ACCESS_TECHNOLOGY_LTE_CAT_NB1:
                if ((s.getStrengthValue() <= -44.0f && s.getStrengthValue() >= -141.0f)
                    && (s.getQualityValue() <= -3.0f && s.getQualityValue() >= -20.0f)) {
                        values_in_range = true;
                        x = num_retries;
                        break;
                }
                Serial.printlnf("StrV: %.2f, QualV: %.2f, RAT: %d", s.getStrengthValue(), s.getQualityValue(), s.getAccessTechnology());
                break;
            default:
                break;
        }
        delay(500);
    }
    assertFalse((s.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_UNKNOWN) || (s.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_NONE));
    assertTrue(values_in_range);
}

test(MDM_01_socket_writes_with_length_more_than_1023_work_correctly) {

#if HAL_PLATFORM_NCP
    // CH32073
    if (cellular_modem_type() == DEV_SARA_R410) {
        skip();
        return;
    }
#endif // HAL_PLATFORM_NCP

    // https://github.com/spark/firmware/issues/1104

    const int dataSize = 1024;
    const int bufferSize = 2048;
    auto randData = std::make_unique<char[]>(dataSize);
    assertTrue((bool)randData);

    Random rand;
    rand.genBase32(randData.get(), dataSize);

    static const char requestFormat[] =
        "POST /post HTTP/1.1\r\n"
        "Host: httpbin.org\r\n"
        "Connection: close\r\n"
        "Content-Type: multipart/form-data; boundary=-------------aaaaaaaa\r\n"
        "Content-Length: 1124\r\n"
        "\r\n"
        "---------------aaaaaaaa\r\n"
        "Content-Disposition: form-data; name=\"field\"\r\n"
        "\r\n"
        "%s\r\n"
        "---------------aaaaaaaa--\r\n";

    auto request = std::make_unique<char[]>(bufferSize);
    assertTrue((bool)request);
    memset(request.get(), 0, bufferSize);

    snprintf(request.get(), bufferSize, requestFormat, randData.get());

    const int requestSize = strlen(request.get());

    Cellular.connect();
    waitFor(Cellular.ready, 120000);

    TCPClient c;
    int res = c.connect("httpbin.org", 80);
    (void)res;

    int sz = c.write((const uint8_t*)request.get(), requestSize);
    assertEqual(sz, requestSize);

    auto responseBuf = std::make_unique<char[]>(bufferSize);
    assertTrue((bool)responseBuf);
    memset(responseBuf.get(), 0, bufferSize);
    int responseSize = 0;
    uint32_t mil = millis();
    while(1) {
        while (c.available() && responseSize < bufferSize) {
            responseBuf.get()[responseSize++] = c.read();
        }
        if (!c.connected() && c.available() <= 0)
            break;
        if (millis() - mil >= 20000UL) {
            if (c.connected()) {
                c.stop();
            }
            break;
        }
    }

    bool contains = false;
    if (responseSize > 0 && !c.connected()) {
        contains = strstr(responseBuf.get(), randData.get()) != nullptr;
        if (!contains) {
            // A workaround for httpbin.org sometimes returning an error for large requests
            contains = strstr(responseBuf.get(), "Request Body Too Large") != nullptr;
        }
    }

    assertTrue(contains);
}

static int atCallback(int type, const char* buf, int len, int* lines) {
    if (len && type == TYPE_UNKNOWN)
        (*lines)++;
    return WAIT;
}

test(MDM_02_at_commands_with_long_response_are_correctly_parsed_and_flow_controlled) {
    if (cellular_modem_type() == DEV_QUECTEL_BG96 || \
        cellular_modem_type() == DEV_QUECTEL_EG91_E || \
        cellular_modem_type() == DEV_QUECTEL_EG91_NA || \
        cellular_modem_type() == DEV_QUECTEL_EG91_EX) {
        Serial.println("TODO: find a command with long response on Quectel NCP");
        skip();
        return;
    }

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

test(MDM_03_restore_cloud_connection) {
    connect_to_cloud(6*60*1000);
}

#endif // Wiring_Cellular == 1
