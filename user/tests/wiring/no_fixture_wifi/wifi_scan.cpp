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
#include "unit-test/unit-test.h"
#include "scope_guard.h"
#include <atomic>

#if !HAL_PLATFORM_WIFI_SCAN_ONLY

namespace {

const unsigned SCAN_ITERATIONS = 20;
const size_t MAX_SCAN_RESULTS = 20;

class RadioRecoveryLogHandler: public LogHandler {
public:
    RadioRecoveryLogHandler() :
            LogHandler(LOG_LEVEL_WARN) {
        LogManager::instance()->addHandler(this);
    }

    ~RadioRecoveryLogHandler() {
        LogManager::instance()->removeHandler(this);
    }

    void reset() {
        scanFailures_ = 0;
        radioResets_ = 0;
    }

    unsigned scanFailures() const {
        return scanFailures_;
    }

    unsigned radioResets() const {
        return radioResets_;
    }

protected:
    void logMessage(const char* msg, LogLevel level, const char* category, const LogAttributes& attr) override {
        if (!msg) {
            return;
        }
        if (!strncmp(msg, "Scan failed", strlen("Scan failed"))) {
            ++scanFailures_;
        } else if (!strncmp(msg, "Resetting radio", strlen("Resetting radio"))) {
            ++radioResets_;
        }
    }

private:
    std::atomic<unsigned> scanFailures_{0};
    std::atomic<unsigned> radioResets_{0};
};

RadioRecoveryLogHandler* logHandler = nullptr;

void scanAndExpectResults(unsigned iterations, bool checkBleAdvertising = false) {
    spark::Vector<WiFiAccessPoint> aps(MAX_SCAN_RESULTS);
    for (unsigned i = 0; i < iterations; i++) {
        const int found = WiFi.scan(aps.data(), MAX_SCAN_RESULTS);
        assertMoreOrEqual(found, 1);
#if Wiring_BLE == 1
        if (checkBleAdvertising) {
            assertTrue(BLE.advertising());
        }
#endif // Wiring_BLE == 1
    }
}

} // anonymous

test(WIFI_SCAN_00_prepare) {
    if (!logHandler) {
        logHandler = new RadioRecoveryLogHandler();
        assertTrue(logHandler);
    }
    WiFi.on();
    assertTrue(waitFor(WiFi.isOn, 30000));
    WiFi.connect();
    assertTrue(waitFor(WiFi.ready, 60000));
}

test(WIFI_SCAN_01_repeated_scans_while_connected_return_results) {
    logHandler->reset();
    scanAndExpectResults(SCAN_ITERATIONS);
    assertEqual(logHandler->radioResets(), 0);
}

test(WIFI_SCAN_02_repeated_scans_while_disconnected_return_results_without_recovery) {
    SCOPE_GUARD({
        WiFi.connect();
        waitFor(WiFi.ready, 60000);
    });
    Particle.disconnect();
    WiFi.disconnect();
    assertTrue(waitForNot(WiFi.ready, 30000));
    logHandler->reset();
    scanAndExpectResults(SCAN_ITERATIONS);
#if HAL_PLATFORM_RTL872X
    assertEqual(logHandler->scanFailures(), 0);
    assertEqual(logHandler->radioResets(), 0);
#endif // HAL_PLATFORM_RTL872X
}

#if Wiring_BLE == 1
test(WIFI_SCAN_03_scans_do_not_interrupt_ble_advertising) {
    SCOPE_GUARD({
        BLE.stopAdvertising();
        WiFi.connect();
        waitFor(WiFi.ready, 60000);
    });
    BLE.on();
    assertEqual(BLE.advertise(), (int)SYSTEM_ERROR_NONE);
    assertTrue(BLE.advertising());
    Particle.disconnect();
    WiFi.disconnect();
    assertTrue(waitForNot(WiFi.ready, 30000));
    logHandler->reset();
    scanAndExpectResults(SCAN_ITERATIONS, true /* checkBleAdvertising */);
#if HAL_PLATFORM_RTL872X
    assertEqual(logHandler->radioResets(), 0);
#endif // HAL_PLATFORM_RTL872X
}
#endif // Wiring_BLE == 1

test(WIFI_SCAN_04_restore_connection) {
    if (!Particle.connected()) {
        Particle.connect();
    }
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
}

#endif // !HAL_PLATFORM_WIFI_SCAN_ONLY
