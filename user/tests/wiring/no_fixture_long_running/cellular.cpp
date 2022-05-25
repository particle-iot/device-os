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
    connect_to_cloud(HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
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
    connect_to_cloud(HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
    // Then the device overcomes this socket obstacle and connects to the Cloud
    assertEqual(Particle.connected(), true);
}
#endif // !HAL_USE_SOCKET_HAL_POSIX

#endif // Wiring_Cellular
