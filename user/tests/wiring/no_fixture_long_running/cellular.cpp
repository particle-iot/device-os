/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "application.h"
#include "unit-test/unit-test.h"

#if Wiring_Cellular

#include "socket_hal.h"
#include "random.h"
#include "scope_guard.h"

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
    // Account for exponential backoff and potentially long registration
    connect_to_cloud(7*60*1000);
    // Then the device overcomes this socket obstacle and connects to the Cloud
    assertEqual(Particle.connected(), true);
}
#endif // !HAL_USE_SOCKET_HAL_POSIX

#if !HAL_PLATFORM_NCP_AT

test(CELLULAR_03_device_will_reconnect_to_the_cloud_within_2mins_when_unexpected_modem_brown_out_occurs) {
    // Serial.println("the device will reconnect to the cloud within 2 min when unexpected modem brown out occurs");
    // Given the device is currently connected to the Cloud
    connect_to_cloud(6*60*1000);
    // When the device modem suffers a simulated brown out (modem reset and powered back on with no initialization)
    digitalWrite(RESET_UC, 0);
    delay(10000);
    digitalWrite(RESET_UC, 1);
    do {
        // SARA-U2/LISA-U2 50..80us
        digitalWrite(PWR_UC, 0);
        delay(50);
        digitalWrite(PWR_UC, 1);
        delay(10);

        // SARA-G35 >5ms, LISA-C2 > 150ms, LEON-G2 >5ms, SARA-R4 >= 150ms
        digitalWrite(PWR_UC, 0);
        delay(150);
        digitalWrite(PWR_UC, 1);
        delay(100);
    } while (RESP_OK != Cellular.command(1000, "AT\r\n"));
    // Then the device overcomes this brown out obstacle and reconnects to the Cloud within 2 minutes
    system_tick_t start = millis();
    waitFor(Particle.disconnected, 60*1000);
    assertEqual(Particle.disconnected, true);
    waitFor(Particle.connected, 2*60*1000 - (millis() - start));
    assertEqual(Particle.connected(), true);
}

test(CELLULAR_04_device_will_reconnect_to_the_cloud_within_2mins_when_unexpected_modem_power_off_occurs) {
    // Serial.println("the device will reconnect to the cloud when unexpected modem power off occurs, within 2min");
    // Given the device is currently connected to the Cloud
    connect_to_cloud(6*60*1000);
    // When the device modem powers off unexpectedly
    digitalWrite(PWR_UC, 0);
    // >1.5 seconds on SARA R410M
    // >1 second on SARA U2
    delay(1600);
    digitalWrite(PWR_UC, 1);
    // Then the device overcomes this reset obstacle and reconnects to the Cloud within 2 minutes
    system_tick_t start = millis();
    waitFor(Particle.disconnected, 60*1000);
    assertEqual(Particle.disconnected, true);
    waitFor(Particle.connected, 2*60*1000 - (millis() - start));
    assertEqual(Particle.connected(), true);
}

#endif // !HAL_PLATFORM_NCP_AT

#endif // Wiring_Cellular
