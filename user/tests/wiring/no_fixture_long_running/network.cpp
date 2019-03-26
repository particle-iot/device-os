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

} // anonymous

test(NETWORK_01_LargePacketsDontCauseIssues_ResolveMtu) {
    Network.on();
    Network.connect();
    waitFor(Network.ready, 60000);
    assertTrue(Network.ready());

    struct State {
        volatile bool disconnected;
    };
    State state = {};

    auto evHandler = [](system_event_t event, int param, void* ctx) {
        State* state = static_cast<State*>(ctx);
        if (event == network_status && param == network_status_disconnected) {
            state->disconnected = true;
        }
    };

    System.on(network_status, evHandler);
    SCOPE_GUARD({
        System.off(network_status, evHandler);
    });

    const size_t IPV4_HEADER_LENGTH = 20;
    const size_t UDP_HEADER_LENGTH = 8;
    const size_t IPV4_PLUS_UDP_HEADER_LENGTH = IPV4_HEADER_LENGTH + UDP_HEADER_LENGTH;
    const size_t MAX_MTU = 1000;
    const size_t MIN_MTU = IPV4_PLUS_UDP_HEADER_LENGTH;
    const system_tick_t UDP_ECHO_REPLY_WAIT_TIME = 5000;
    const unsigned UDP_ECHO_RETRIES = 5;
    const system_tick_t MINIMUM_TEST_TIME = 60000;

    // FIXME: Hosted by @avtolstoy, should be changed to something else
    const char UDP_ECHO_SERVER[] = "particle-udp-echo.rltm.org";
    uint16_t UDP_ECHO_PORT = 40000;

    // Resolve UDP echo server hostname to ip address, so that DNS resolutions
    // no longer affect us after this point
    const auto udpEchoIp = Network.resolve(UDP_ECHO_SERVER);
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
        Serial.printlnf("Test MTU: %lu (%s)", mtu, res ? "OK" : "FAIL");
        size_t newMtu = bisect(mtu, minMtu, maxMtu, res);
        if (std::abs((int)newMtu - (int)mtu) <= 1 && res) {
            // Converged
            break;
        }
        mtu = newMtu;
    }

    Serial.printlnf("Resolved MTU: %lu", mtu);

    // The test should be running for at least a minute, just in case
    if (millis() - start < MINIMUM_TEST_TIME) {
        delay(millis() - start);
    }
    assertFalse((bool)state.disconnected);

    assertTrue((mtu - IPV4_PLUS_UDP_HEADER_LENGTH) >= MBEDTLS_SSL_MAX_CONTENT_LEN);

    SCOPE_GUARD({
        Network.disconnect();
        Network.off();
    });
}
