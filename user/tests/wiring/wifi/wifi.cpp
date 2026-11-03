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
#include "wifi_config_backup.h"
#include <atomic>

#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY

using particle::test::WifiConfigBackup;
using particle::test::WIFI_SUITE_BACKUP;

namespace {

const char* const MISSING_NETWORK_SSID = "device-os-test-missing";
const char* const MISSING_NETWORK_PASSPHRASE = "device-os-test-missing";
const system_tick_t WIFI_CONNECT_TIMEOUT = 120000;
const system_tick_t WIFI_DISCONNECT_TIMEOUT = 30000;
const system_tick_t CLOUD_CONNECT_TIMEOUT = 180000;
const system_tick_t AFTER_FRAME_DELAY = 10000;
const system_tick_t RECONNECT_TIMEOUT = 150000;
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

ConnectionLogHandler& logHandler() {
    static ConnectionLogHandler handler;
    return handler;
}

bool connectedAfterFrame() {
    delay(AFTER_FRAME_DELAY);
    return waitFor(WiFi.ready, WIFI_CONNECT_TIMEOUT) && waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT);
}

} // anonymous

test(WIFI_00_prepare) {
    Particle.disconnect();
    WifiConfigBackup backup(WIFI_SUITE_BACKUP);
    assertTrue(backup.restore());
    assertTrue(backup.save());
    WiFi.off();
    assertTrue(waitFor(WiFi.isOff, 60000));
    WiFi.clearCredentials();
    assertFalse(WiFi.hasCredentials());
    WiFi.on();
    assertTrue(waitFor(WiFi.isOn, 30000));
}

test(WIFI_02_connect_to_cloud_through_ap) {
    assertTrue(WiFi.hasCredentials());
    WiFi.connect();
    assertTrue(waitFor(WiFi.ready, WIFI_CONNECT_TIMEOUT));
    Particle.connect();
    assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));
}

test(WIFI_11_connected_after_btm_request_with_bss_termination) {
    assertTrue(connectedAfterFrame());
}

test(WIFI_13_connected_after_btm_request_with_bad_url_length) {
    assertTrue(connectedAfterFrame());
}

test(WIFI_15_connected_after_beacon_request) {
    assertTrue(connectedAfterFrame());
}

test(WIFI_17_connected_after_beacon_request_with_oversized_ssid) {
    assertTrue(connectedAfterFrame());
}

test(WIFI_21_no_coverage_connection_attempts_back_off_without_radio_reset) {
#if Wiring_BLE == 1
    BLE.on();
    assertEqual(BLE.advertise(), (int)SYSTEM_ERROR_NONE);
    assertTrue(BLE.advertising());
#endif // Wiring_BLE == 1
    assertTrue(System.waitCondition([]() {
        return !WiFi.ready();
    }, WIFI_DISCONNECT_TIMEOUT));
    logHandler().reset();
    for (auto start = millis(); millis() - start < BACKOFF_OBSERVATION_TIME;) {
        assertFalse(WiFi.ready());
#if Wiring_BLE == 1
        assertTrue(BLE.advertising());
#endif // Wiring_BLE == 1
        assertEqual(logHandler().radioResets(), 0);
        delay(500);
    }
    assertMoreOrEqual(logHandler().failedAttempts(), MIN_EXPECTED_ATTEMPTS);
    assertLessOrEqual(logHandler().failedAttempts(), MAX_EXPECTED_ATTEMPTS);
}

test(WIFI_22_no_coverage_new_credentials_reset_backoff) {
    const unsigned initialDelays = logHandler().initialDelays();
    assertTrue(WiFi.setCredentials(MISSING_NETWORK_SSID, MISSING_NETWORK_PASSPHRASE, WLAN_SEC_WPA2, WLAN_CIPHER_AES));
    assertTrue(System.waitCondition([initialDelays]() {
        return logHandler().initialDelays() > initialDelays;
    }, NEW_CREDENTIALS_ATTEMPT_TIMEOUT));
    assertEqual(logHandler().radioResets(), 0);
}

test(WIFI_24_reconnect_when_ap_returns) {
    assertTrue(waitFor(WiFi.ready, RECONNECT_TIMEOUT));
    assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));
    assertEqual(logHandler().radioResets(), 0);
}

test(WIFI_97_reset) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 20000));
    System.reset();
}

test(WIFI_98_cleanup) {
    WifiConfigBackup backup(WIFI_SUITE_BACKUP);
    assertTrue(backup.restore());
}

test(WIFI_99_reconnect_with_original_credentials) {
#if !HAL_PLATFORM_CELLULAR
    if (!WiFi.hasCredentials()) {
        skip();
        return;
    }
#endif // !HAL_PLATFORM_CELLULAR
    Particle.connect();
    assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));
}

#endif // HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
