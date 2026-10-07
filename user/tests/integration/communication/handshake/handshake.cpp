/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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
#include "scope_guard.h"
#include "test.h"

namespace {

const unsigned MAX_SYNC_RTT_DURING_HANDSHAKE = 500;
const unsigned SYNC_PROBE_INTERVAL = 50; // ms
const int GET_CLOUD_SOCKET_HANDLE_INTERNAL_ID = 3;

struct HandshakeState {
    volatile int type = -1;

    bool operator()() const {
        return type != -1;
    }

    void reset() {
        type = -1;
    }
};

HandshakeState handshakeState;

void cloudStatusHandler(system_event_t event, int param, void*) {
    if (event == cloud_status && (param == cloud_status_handshake ||
            param == cloud_status_session_resume) && handshakeState.type == -1) {
        handshakeState.type = param;
    }
}

sock_handle_t cloudSocket() {
    return (sock_handle_t)system_internal(GET_CLOUD_SOCKET_HANDLE_INTERNAL_ID, nullptr);
}

unsigned measureSyncRtt() {
    auto t1 = millis();
    int result = Particle.getKeepAlive();
    auto t2 = millis();
    if (result <= 0) {
        return 0;
    }
    return t2 - t1;
}

unsigned maxSyncRtt = 0;
bool measuredDuringHandshake = false;

} // namespace

test(01_full_handshake_nonblocking_sync_rtt) {
    if (system_thread_get_state(nullptr) != spark::feature::ENABLED) {
        skip();
        return;
    }
    maxSyncRtt = 0;
    measuredDuringHandshake = false;
    handshakeState.reset();

    System.on(cloud_status, cloudStatusHandler);
    SCOPE_GUARD({
        System.off(cloud_status, cloudStatusHandler);
    });

    Particle.disconnect();
    assertTrue(waitFor(Particle.disconnected, 30000));
    Particle.disconnect(CloudDisconnectOptions().clearSession(true));
    assertTrue(waitFor(Particle.disconnected, 30000));

    Particle.connect();
    auto connectStart = millis();
    while (!handshakeState() && millis() - connectStart < HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME) {
        if (socket_handle_valid(cloudSocket())) {
            auto rtt = measureSyncRtt();
            if (rtt > 0) {
                measuredDuringHandshake = true;
                if (rtt > maxSyncRtt) {
                    maxSyncRtt = rtt;
                }
            }
        }
        Particle.process();
        delay(SYNC_PROBE_INTERVAL);
    }
    assertTrue(handshakeState());
    assertEqual((int)cloud_status_handshake, (int)handshakeState.type);
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
}

test(02_full_handshake_sync_rtt_within_bounds) {
    if (system_thread_get_state(nullptr) != spark::feature::ENABLED) {
        skip();
        return;
    }
    assertTrue(measuredDuringHandshake);
    assertMore(maxSyncRtt, 0);
    assertLess(maxSyncRtt, MAX_SYNC_RTT_DURING_HANDSHAKE);
}

test(03_handshake_session_resumes_after_socket_failure) {
    if (system_thread_get_state(nullptr) != spark::feature::ENABLED) {
        skip();
        return;
    }
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

    handshakeState.reset();
    System.on(cloud_status, cloudStatusHandler);
    SCOPE_GUARD({
        System.off(cloud_status, cloudStatusHandler);
    });

    const auto sock = cloudSocket();
    assertTrue(socket_handle_valid(sock));
#if HAL_USE_SOCKET_HAL_POSIX
    assertEqual(0, sock_close(sock));
#else
    assertEqual(0, socket_close(sock));
#endif
    (void)Particle.publish("test", "test");

    assertTrue(waitFor(handshakeState, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertEqual((int)cloud_status_session_resume, (int)handshakeState.type);
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
}

test(04_handshake_nonblocking_disconnect_during_handshake) {
    Particle.disconnect();
    assertTrue(waitFor(Particle.disconnected, 30000));
    Particle.disconnect(CloudDisconnectOptions().clearSession(true));
    assertTrue(waitFor(Particle.disconnected, 30000));

    Particle.connect();
    delay(2000);

    Particle.disconnect();
    assertTrue(waitFor(Particle.disconnected, 30000));
    assertFalse(Particle.connected());

    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
}
