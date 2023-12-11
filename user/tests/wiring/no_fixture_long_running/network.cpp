/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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
#include "scope_guard.h"
#include "random.h"
#include <memory>
#include "mbedtls_config.h"

namespace {

struct NetworkState {
    volatile bool disconnected = false;
};
NetworkState networkState;

template <typename T, typename DT>
T divRoundClosest(T n, DT d) {
    return ((n + (d / 2)) / d);
}

template <typename T>
T bisect(T value, T& min, T& max, bool forward) {
    if (forward) {
        size_t halfInterval = divRoundClosest(max - value, 2);
        min = value;
        return value + halfInterval;
    } else {
        size_t halfInterval = divRoundClosest(value - min, 2);
        max = value;
        return value - halfInterval;
    }
}

bool udpEchoTest(UDP* udp, const IPAddress& ip, uint16_t port, const uint8_t* sendBuf, size_t len, unsigned retries, system_tick_t timeout) {
    while (retries-- > 0) {
        auto snd = udp->sendPacket(sendBuf, len, ip, port);
        if (snd == (int)len) {
            auto recvd = udp->parsePacket(timeout);
            if (recvd == (int)len) {
                return !memcmp(udp->buffer(), sendBuf, len);
            }
            // Failed to receive reply, retry
        } else {
            // Failed to send, abort immediately
            break;
        }
    }

    return false;
}

#ifdef stringify
#undef stringify
#endif
#ifdef __stringify
#undef __stringify
#endif
#define stringify(x) __stringify(x)
#define __stringify(x) #x

#ifndef UDP_ECHO_SERVER_HOSTNAME
#define UDP_ECHO_SERVER_HOSTNAME not_defined
#endif

const char udpEchoServerHostname[] = stringify(UDP_ECHO_SERVER_HOSTNAME);

} // anonymous

test(NETWORK_01_LargePacketsDontCauseIssues_ResolveMtu) {
    // If server not defined, skip test
    if (!strcmp(udpEchoServerHostname, "not_defined") || !strcmp(udpEchoServerHostname, "")) {
        Serial.printlnf("Command line option UDP_ECHO_SERVER_HOSTNAME not defined! Usage: UDP_ECHO_SERVER_HOSTNAME=hostname make clean all TEST=...");
        skip();
        return;
    }
    Serial.printlnf("Using Echo Server: [%s]", udpEchoServerHostname);

    // 15 min gives the device time to go through a 10 min timeout & power cycle
    const system_tick_t WAIT_TIMEOUT = 15 * 60 * 1000;

    Network.on();
    Network.connect();

    SCOPE_GUARD({
        Network.disconnect();
        Network.off();
    });

    waitFor(Network.ready, WAIT_TIMEOUT);
    assertTrue(Network.ready());

    auto evHandler = [](system_event_t event, int param, void* ctx) {
        if (event == network_status && param == network_status_disconnected) {
            networkState.disconnected = true;
        }
    };

    System.on(network_status, evHandler);
    SCOPE_GUARD({
        System.off(network_status, evHandler);
    });

    const size_t IPV4_HEADER_LENGTH = 20;
    const size_t UDP_HEADER_LENGTH = 8;
    const size_t IPV4_PLUS_UDP_HEADER_LENGTH = IPV4_HEADER_LENGTH + UDP_HEADER_LENGTH;
    // Start a bit lower than standard 1500
    const size_t MAX_MTU = 1400;
    const size_t MIN_MTU = IPV4_PLUS_UDP_HEADER_LENGTH;
    const system_tick_t UDP_ECHO_REPLY_WAIT_TIME = 10000;
    const unsigned UDP_ECHO_RETRIES = 5;
    const system_tick_t MINIMUM_TEST_TIME = 60000;

    const uint16_t UDP_ECHO_PORT = 40000;

    // Resolve UDP echo server hostname to ip address, so that DNS resolutions
    // no longer affect us after this point
    IPAddress udpEchoIp;
    for (auto begin = millis(); millis() - begin < 60000;) {
        udpEchoIp = Network.resolve(udpEchoServerHostname);
        if (udpEchoIp) {
            break;
        }
        delay(3000);
    }
    assertTrue(udpEchoIp);

    // Create UDP client
    std::unique_ptr<UDP> udp(new UDP());
    assertTrue((bool)udp);

    std::unique_ptr<uint8_t[]> sendBuffer(new uint8_t[MAX_MTU]);
    assertTrue((bool)sendBuffer);
    std::unique_ptr<uint8_t[]> recvBuffer(new uint8_t[MAX_MTU]);
    assertTrue((bool)recvBuffer);

    udp->setBuffer(MAX_MTU, recvBuffer.get());

    particle::Random rand;

    system_tick_t start = millis();

    udp->begin(UDP_ECHO_PORT);
    size_t mtu = MAX_MTU;
    size_t minMtu = MIN_MTU;
    size_t maxMtu = MAX_MTU;
    while (mtu > IPV4_PLUS_UDP_HEADER_LENGTH) {
        // Fille send buffer with random data
        const size_t payloadSize = mtu - IPV4_PLUS_UDP_HEADER_LENGTH;
        rand.gen((char*)sendBuffer.get(), payloadSize);
        auto res = udpEchoTest(udp.get(), udpEchoIp, UDP_ECHO_PORT, sendBuffer.get(), payloadSize, UDP_ECHO_RETRIES, UDP_ECHO_REPLY_WAIT_TIME);
        Serial.printlnf("Test MTU: %u (%s)", mtu, res ? "OK" : "FAIL");
        size_t newMtu = bisect(mtu, minMtu, maxMtu, res);
        if (std::abs((int)newMtu - (int)mtu) <= 1 && res) {
            // Converged
            break;
        }
        mtu = newMtu;
    }

    Serial.printlnf("Resolved MTU: %u", mtu);

    // The test should be running for at least a minute, just in case
    if (millis() - start < MINIMUM_TEST_TIME) {
        delay(millis() - start);
    }
    assertFalse((bool)networkState.disconnected);
#if PLATFORM_ID != PLATFORM_BORON && PLATFORM_ID != PLATFORM_BSOM
    assertMoreOrEqual((mtu - IPV4_PLUS_UDP_HEADER_LENGTH), MBEDTLS_SSL_MAX_CONTENT_LEN);
#else
    // We've reduced MTU on LTE Boron and B SoMs with R410 running modem firwmare <= 02.03
    if ((mtu - IPV4_PLUS_UDP_HEADER_LENGTH) < MBEDTLS_SSL_MAX_CONTENT_LEN) {
        assertMoreOrEqual(mtu, 990);
    } else {
        assertMoreOrEqual((mtu - IPV4_PLUS_UDP_HEADER_LENGTH), MBEDTLS_SSL_MAX_CONTENT_LEN);
    }
#endif // PLATFORM_ID != PLATFORM_BORON && PLATFORM_ID != PLATFORM_BSOM
}

#if HAL_PLATFORM_NCP_AT || HAL_PLATFORM_CELLULAR

test(NETWORK_02_network_connection_recovers_after_ncp_failure) {
    // 20 min gives the device time to go through a 10-15 min timeout & power cycle
    const system_tick_t WAIT_TIMEOUT = 20 * 60 * 1000;
    const system_tick_t NCP_FAILURE_TIMEOUT = 15000;

    Network.on();
    Network.connect();
    Particle.connect();
    waitFor(Particle.connected, WAIT_TIMEOUT);
    assertTrue(Particle.connected());

    // Simulate NCP failure by reconfiguring the NCP serial port with a baudrate
    // setting different from expected.
    // FIXME: when a new platform is added not using HAL_PLATFORM_CELLULAR_SERIAL or not using
    // UART for talking to NCP.
    SINGLE_THREADED_BLOCK() {
#if HAL_PLATFORM_NCP_AT
#if HAL_PLATFORM_CELLULAR
        hal_usart_end(HAL_PLATFORM_CELLULAR_SERIAL);
        hal_usart_begin_config(HAL_PLATFORM_CELLULAR_SERIAL, 57600, SERIAL_8N1, nullptr);
#else
        hal_usart_end(HAL_PLATFORM_WIFI_SERIAL);
        hal_usart_begin_config(HAL_PLATFORM_WIFI_SERIAL, 57600, SERIAL_8N1, nullptr);
#endif // HAL_PLATFORM_CELLULAR
#endif // HAL_PLATFORM_NCP_AT
    }

    delay(NCP_FAILURE_TIMEOUT);

    // Eventually cloud connection is going to be restored and we should receive an ACK to a publish
    auto start = millis();
    bool published = false;
    while (millis() - start <= WAIT_TIMEOUT) {
        if (Particle.connected()) {
            published = Particle.publish("test", "123", WITH_ACK);
        }
        if (published) {
            break;
        }
        delay(5000);
    }

    assertTrue(published);
}

#endif // HAL_PLATFORM_NCP_AT || HAL_PLATFORM_CELLULAR

#if HAL_PLATFORM_NCP_AT

static bool s_networkStatusChanged = false;

test(NETWORK_03_network_connection_recovers_after_ncp_uart_sleep) {
    // 20 min gives the device time to go through a 10-15 min timeout & power cycle
    const system_tick_t WAIT_TIMEOUT = 20 * 60 * 1000;

    SCOPE_GUARD({
        Particle.disconnect();
        Network.disconnect();
        Network.off();
    });

    Particle.connect();
    waitFor(Particle.connected, WAIT_TIMEOUT);
    assertTrue(Particle.connected());

    auto handler = [](system_event_t ev, int, void*) -> void {
        s_networkStatusChanged = true;
    };

    System.on(network_status, handler);
    SCOPE_GUARD({
        System.off(network_status, handler);
    });

    SINGLE_THREADED_BLOCK() {
#if HAL_PLATFORM_CELLULAR
        assertEqual(0, hal_usart_sleep(HAL_PLATFORM_CELLULAR_SERIAL, true, nullptr));
        assertEqual(0, hal_usart_sleep(HAL_PLATFORM_CELLULAR_SERIAL, false, nullptr));
#else
        assertEqual(0, hal_usart_sleep(HAL_PLATFORM_WIFI_SERIAL, true, nullptr));
        assertEqual(0, hal_usart_sleep(HAL_PLATFORM_WIFI_SERIAL, false, nullptr));
#endif // HAL_PLATFORM_CELLULAR
    }

    delay(1000);

    // Eventually cloud connection is going to be restored and we should receive an ACK to a publish
    auto start = millis();
    bool published = false;
    while (millis() - start <= WAIT_TIMEOUT) {
        if (Particle.connected()) {
            published = Particle.publish("test", "123", WITH_ACK);
        }
        if (published) {
            break;
        }
        delay(5000);
    }

    assertTrue(published);
    assertFalse(s_networkStatusChanged);
}

#endif // HAL_PLATFORM_NCP_AT
