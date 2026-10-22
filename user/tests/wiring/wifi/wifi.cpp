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
#include <atomic>

#if !HAL_PLATFORM_WIFI_SCAN_ONLY

namespace {

const char* const MISSING_NETWORK_PASSWORD = "particle-test";
const system_tick_t BACKOFF_OBSERVATION_TIME = 180000;
const unsigned MIN_EXPECTED_ATTEMPTS = 4;
const unsigned MAX_EXPECTED_ATTEMPTS = 8;
const system_tick_t NEW_CREDENTIALS_ATTEMPT_TIMEOUT = 60000;
const system_tick_t MAX_INITIAL_BACKOFF_DELAY = 12000;
const char* const NEXT_ATTEMPT_PREFIX = "Next connection attempt in ";

class ConnectionLogHandler: public LogHandler {
public:
    ConnectionLogHandler() :
            LogHandler(LOG_LEVEL_ALL) {
        LogManager::instance()->addHandler(this);
    }

    ~ConnectionLogHandler() {
        LogManager::instance()->removeHandler(this);
    }

    void reset() {
        failedAttempts_ = 0;
        radioResets_ = 0;
        initialDelays_ = 0;
    }

    unsigned failedAttempts() const {
        return failedAttempts_;
    }

    unsigned radioResets() const {
        return radioResets_;
    }

    unsigned initialDelays() const {
        return initialDelays_;
    }

protected:
    void logMessage(const char* msg, LogLevel level, const char* category, const LogAttributes& attr) override {
        if (!msg) {
            return;
        }
        if (!strncmp(msg, NEXT_ATTEMPT_PREFIX, strlen(NEXT_ATTEMPT_PREFIX))) {
            if (strtoul(msg + strlen(NEXT_ATTEMPT_PREFIX), nullptr, 10) <= MAX_INITIAL_BACKOFF_DELAY) {
                ++initialDelays_;
            }
            ++failedAttempts_;
        } else if (!strncmp(msg, "Resetting radio", strlen("Resetting radio"))) {
            ++radioResets_;
        }
    }

private:
    std::atomic<unsigned> failedAttempts_{0};
    std::atomic<unsigned> radioResets_{0};
    std::atomic<unsigned> initialDelays_{0};
};

ConnectionLogHandler* logHandler = nullptr;

String missingNetworkSsid(const char* suffix) {
    return String::format("particle-test-missing-%s-%s", System.deviceID().substring(18).c_str(), suffix);
}

bool setMissingNetworkCredentials(const char* suffix) {
    const auto ssid = missingNetworkSsid(suffix);
    return WiFi.setCredentials(ssid.c_str(), MISSING_NETWORK_PASSWORD, WLAN_SEC_WPA2, WLAN_CIPHER_AES);
}

} // anonymous

test(WIFI_00_prepare) {
    if (!logHandler) {
        logHandler = new ConnectionLogHandler();
        assertTrue(logHandler);
    }
    Particle.disconnect();
    WiFi.disconnect();
    WiFi.on();
    assertTrue(waitFor(WiFi.isOn, 30000));
    WiFi.clearCredentials();
    assertFalse(WiFi.hasCredentials());
    assertTrue(setMissingNetworkCredentials("a"));
    assertTrue(WiFi.hasCredentials());
}

test(WIFI_01_no_coverage_connection_attempts_back_off_without_radio_reset) {
#if Wiring_BLE == 1
    BLE.on();
    assertEqual(BLE.advertise(), (int)SYSTEM_ERROR_NONE);
    assertTrue(BLE.advertising());
#endif // Wiring_BLE == 1
    logHandler->reset();
    WiFi.connect();
    for (auto start = millis(); millis() - start < BACKOFF_OBSERVATION_TIME;) {
        assertFalse(WiFi.ready());
#if Wiring_BLE == 1
        assertTrue(BLE.advertising());
#endif // Wiring_BLE == 1
        assertEqual(logHandler->radioResets(), 0);
        delay(500);
    }
    assertMoreOrEqual(logHandler->failedAttempts(), MIN_EXPECTED_ATTEMPTS);
    assertLessOrEqual(logHandler->failedAttempts(), MAX_EXPECTED_ATTEMPTS);
}

test(WIFI_02_no_coverage_new_credentials_reset_backoff) {
    const unsigned initialDelays = logHandler->initialDelays();
    assertTrue(setMissingNetworkCredentials("b"));
    assertTrue(System.waitCondition([initialDelays]() {
        return logHandler->initialDelays() > initialDelays;
    }, NEW_CREDENTIALS_ATTEMPT_TIMEOUT));
    assertEqual(logHandler->radioResets(), 0);
}

test(WIFI_99_cleanup) {
#if Wiring_BLE == 1
    BLE.stopAdvertising();
#endif // Wiring_BLE == 1
    WiFi.disconnect();
    WiFi.clearCredentials();
    assertFalse(WiFi.hasCredentials());
}

#endif // !HAL_PLATFORM_WIFI_SCAN_ONLY
