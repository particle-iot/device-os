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

#if !HAL_PLATFORM_NCP_AT

bool modemPowerState() {
    auto state = HAL_GPIO_Read(RI_UC);
    if (state) {
        // RI is high, which means that V_INT is high as well
        // We are definitely powered on
        return true;
    }
    // We are still not sure whether we are off or not,
    // because RI may be held low for over a second under some conditions
    // Sleeping for 1.1s and checking again
    HAL_Delay_Milliseconds(1100);
    state = HAL_GPIO_Read(RI_UC);
    // If RI is still low - we are off
    // If RI is high - we are on
    return state;
}

void busyDelay(system_tick_t m) {
    for (auto t = millis(); millis() < (t + m););
}

test(CELLULAR_03_device_will_reconnect_to_the_cloud_within_2mins_when_unexpected_modem_brown_out_occurs) {
    // Serial.println("the device will reconnect to the cloud within 2 min when unexpected modem brown out occurs");
    // Given the device is currently connected to the Cloud
    connect_to_cloud(HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
    // Using single threaded section to prevent the system from executing any modem operations
    SINGLE_THREADED_BLOCK() {
        // Disable UART interrupts so that we are not receiving any new data from the modem
        NVIC_DisableIRQ(USART3_IRQn);
        SCOPE_GUARD({
            NVIC_EnableIRQ(USART3_IRQn);
        });
        // When the device modem suffers a simulated brown out (modem reset and powered back on with no initialization)
        digitalWrite(RESET_UC, 0);
        // NOTE: we cannot use delay() here as it will run background system loop in non-threaded mode, which isn't really what we want
        // We also cannot use OS-based delays under single-threaded section
        busyDelay(10000);
        digitalWrite(RESET_UC, 1);
        busyDelay(1000);
        auto start = millis();
        do {
            // SARA-U2/LISA-U2 50..80us
            digitalWrite(PWR_UC, 0);
            busyDelay(50);
            digitalWrite(PWR_UC, 1);
            busyDelay(10);

            // SARA-G35 >5ms, LISA-C2 > 150ms, LEON-G2 >5ms, SARA-R4 >= 150ms
            digitalWrite(PWR_UC, 0);
            busyDelay(150);
            digitalWrite(PWR_UC, 1);
            busyDelay(100);
        } while (!modemPowerState() && ((millis() - start) < 60000));
        assertTrue(modemPowerState());
    }
    // Then the device overcomes this brown out obstacle and reconnects to the Cloud within 2 minutes
    system_tick_t start = millis();
    waitFor(Particle.disconnected, 60*1000);
    assertEqual(Particle.disconnected, true);
    waitFor(Particle.connected, 2*60*1000 - (millis() - start));
    assertEqual(Particle.connected(), true);
}

test(CELLULAR_04_device_will_reconnect_to_the_cloud_within_some_minutes_when_unexpected_modem_power_off_occurs) {
    // Serial.println("the device will reconnect to the cloud when unexpected modem power off occurs, within 2min");
    // Given the device is currently connected to the Cloud
    connect_to_cloud(HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
    assertTrue(Particle.connected());

    CellularDevice dev;
    cellular_device_info(&dev, nullptr);

    // FIXME: SARA G350 does not support power-off with PWR_ON pin
    if (dev.dev == DEV_SARA_G350) {
        skip();
        return;
    }

    // Using single threaded section to prevent the system from executing any modem operations
    SINGLE_THREADED_BLOCK() {
        // Disable UART interrupts so that we are not receiving any new data from the modem
        NVIC_DisableIRQ(USART3_IRQn);
        SCOPE_GUARD({
            NVIC_EnableIRQ(USART3_IRQn);
        });

        // When the device modem powers off unexpectedly
        digitalWrite(PWR_UC, 0);
        // >1.5 seconds on SARA R410M
        // >1 second on SARA U2
        // NOTE: we cannot use delay() here as it will run background system loop in non-threaded mode, which isn't really what we want
        // We also cannot use OS-based delays under single-threaded section
        busyDelay(1600);
        digitalWrite(PWR_UC, 1);

        auto start = millis();
        while (modemPowerState() && (millis() - start) < 60000);
        assertFalse(modemPowerState());
    }
    // Then the device overcomes this reset obstacle and reconnects to the Cloud within 2 minutes
    system_tick_t start = millis();
    waitFor(Particle.disconnected, 60*1000);
    assertEqual(Particle.disconnected, true);
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
}

test(CELLULAR_05_device_will_poweroff_when_modem_is_not_responsive) {
    connect_to_cloud(HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
    assertTrue(Particle.connected());

    // Using single threaded section to prevent the system from executing any modem operations
    SINGLE_THREADED_BLOCK() {
        // Disable UART interrupts so that we are not receiving any new data from the modem
        NVIC_DisableIRQ(USART3_IRQn);
    }

    // Request power-off
    Particle.disconnect();
    Cellular.off();

    // Validate that it powers off
    auto start = millis();
    while (modemPowerState() && (millis() - start) < 60000);
    assertFalse(modemPowerState());

    // Make sure the device can reconnect
    connect_to_cloud(HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
    assertTrue(Particle.connected());
}

#endif // !HAL_PLATFORM_NCP_AT

#endif // Wiring_Cellular
