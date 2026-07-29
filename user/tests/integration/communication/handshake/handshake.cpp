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
#include "test.h"

namespace {

const unsigned MAX_SYNC_RTT_DURING_HANDSHAKE = 2000; // ms
const unsigned SYNC_PROBE_INTERVAL = 50; // ms

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

test(01_handshake_nonblocking_sync_rtt_during_full_handshake) {
    if (system_thread_get_state(nullptr) != spark::feature::ENABLED) {
        skip();
        return;
    }
    maxSyncRtt = 0;
    measuredDuringHandshake = false;

    Particle.disconnect();
    assertTrue(waitFor(Particle.disconnected, 30000));
    Particle.disconnect(CloudDisconnectOptions().clearSession(true));
    assertTrue(waitFor(Particle.disconnected, 30000));

    Particle.connect();
    auto connectStart = millis();
    while (!Particle.connected() && millis() - connectStart < HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME) {
        auto rtt = measureSyncRtt();
        if (rtt > 0) {
            measuredDuringHandshake = true;
            if (rtt > maxSyncRtt) {
                maxSyncRtt = rtt;
            }
        }
        Particle.process();
        delay(SYNC_PROBE_INTERVAL);
    }
    assertTrue(Particle.connected());
}

test(02_handshake_nonblocking_sync_rtt_within_bounds) {
    if (system_thread_get_state(nullptr) != spark::feature::ENABLED) {
        skip();
        return;
    }
    assertTrue(measuredDuringHandshake);
    assertMore(maxSyncRtt, 0);
    assertLess(maxSyncRtt, MAX_SYNC_RTT_DURING_HANDSHAKE);
}

test(03_handshake_nonblocking_session_resume_not_blocked) {
    if (system_thread_get_state(nullptr) != spark::feature::ENABLED) {
        skip();
        return;
    }
    Particle.disconnect();
    assertTrue(waitFor(Particle.disconnected, 30000));

    maxSyncRtt = 0;
    measuredDuringHandshake = false;

    Particle.connect();
    auto connectStart = millis();
    while (!Particle.connected() && millis() - connectStart < HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME) {
        auto rtt = measureSyncRtt();
        if (rtt > 0) {
            measuredDuringHandshake = true;
            if (rtt > maxSyncRtt) {
                maxSyncRtt = rtt;
            }
        }
        Particle.process();
        delay(SYNC_PROBE_INTERVAL);
    }
    assertTrue(Particle.connected());
    assertTrue(measuredDuringHandshake);
    assertLess(maxSyncRtt, MAX_SYNC_RTT_DURING_HANDSHAKE);
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