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

#define PARTICLE_USE_UNSTABLE_API

#include "application.h"
#include "unit-test/unit-test.h"
#include "wifi_config_backup.h"
#include "storage_hal.h"
#include <atomic>

#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY

using particle::test::WifiConfigBackup;
using particle::test::WIFI_SUITE_BACKUP;

#if HAL_PLATFORM_RTL872X
extern uintptr_t platform_ram_start;
extern uintptr_t platform_ram_end;
#endif // HAL_PLATFORM_RTL872X

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
const system_tick_t WIFI_OFF_TIMEOUT = 60000;

#if HAL_PLATFORM_RTL872X
const uint16_t RX_STRESS_PORT = 50100;
const uint16_t RX_STRESS_BROADCAST_PORT = 50101;
const system_tick_t RX_STRESS_START_TIMEOUT = 60000;
const system_tick_t RX_STRESS_DURATION = 3 * 60 * 1000;
const system_tick_t RX_STRESS_POLL_INTERVAL = 10;
const system_tick_t RX_STALL_TIMEOUT = 15000;
const system_tick_t RX_STALL_RECOVERY_TIMEOUT = 30000;
const size_t RX_STALL_SCAN_MAX_APS = 20;
const uint16_t BLE_SCAN_TIMEOUT = 0; // indefinite
const size_t RX_STRESS_STATS_SIZE = 256;
const system_tick_t RX_STRESS_MAILBOX_TIMEOUT = 30000;
const system_tick_t BLE_SCAN_RESTART_DELAY = 100;
const system_tick_t BLE_SCAN_STOP_POLL_INTERVAL = 10;

const size_t SRAM_HOG_CHUNK_SIZE = 4096;
const size_t SRAM_HOG_MAX_CHUNKS = 128;
const size_t SRAM_HOG_HEADROOM_CHUNKS = 2;

const uintptr_t FLASH_READ_ADDRESS = EXTERNAL_FLASH_SIZE - sizeof(uint32_t);
const unsigned FLASH_READ_BURST = 256;
const system_tick_t FLASH_READ_IDLE_MS = 1;
const os_thread_prio_t FLASH_READ_THREAD_PRIORITY = OS_THREAD_PRIORITY_DEFAULT - 1;
#endif // HAL_PLATFORM_RTL872X

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

#if HAL_PLATFORM_RTL872X
UDP s_rxStressUdp;
UDP s_rxStressBroadcastUdp;
std::atomic<unsigned> s_flashReads{0};

void* s_sramHog[SRAM_HOG_MAX_CHUNKS] = {};

bool isSramAddress(const void* p) {
    return (uintptr_t)p >= (uintptr_t)&platform_ram_start && (uintptr_t)p < (uintptr_t)&platform_ram_end;
}

size_t hogSramHeap() {
    size_t count = 0;
    while (count < SRAM_HOG_MAX_CHUNKS) {
        void* p = malloc(SRAM_HOG_CHUNK_SIZE);
        if (!p) {
            break;
        }
        if (!isSramAddress(p)) {
            free(p);
            break;
        }
        s_sramHog[count++] = p;
    }
    for (size_t i = 0; i < SRAM_HOG_HEADROOM_CHUNKS && count > 0; i++) {
        free(s_sramHog[--count]);
        s_sramHog[count] = nullptr;
    }
    return count;
}

void releaseSramHeap() {
    for (auto& p: s_sramHog) {
        free(p);
        p = nullptr;
    }
}

Thread* s_flashReadThread = nullptr;
std::atomic<bool> s_flashReadStop{false};

bool startFlashReads() {
    s_flashReads = 0;
    s_flashReadStop = false;
    s_flashReadThread = new Thread("flash_read", [](void* param) -> os_thread_return_t {
        uint32_t word = 0;
        while (!s_flashReadStop) {
            for (unsigned i = 0; i < FLASH_READ_BURST; i++) {
                if (hal_storage_read(HAL_STORAGE_ID_EXTERNAL_FLASH, FLASH_READ_ADDRESS, (uint8_t*)&word, sizeof(word)) == (int)sizeof(word)) {
                    ++s_flashReads;
                }
            }
            delay(FLASH_READ_IDLE_MS);
        }
    }, nullptr, FLASH_READ_THREAD_PRIORITY);
    return s_flashReadThread != nullptr;
}

void stopFlashReads() {
    if (s_flashReadThread) {
        s_flashReadStop = true;
        s_flashReadThread->join();
        delete s_flashReadThread;
        s_flashReadThread = nullptr;
    }
}

unsigned receivePackets(UDP& udp) {
    unsigned count = 0;
    while (udp.parsePacket() > 0) {
        ++count;
    }
    return count;
}

void formatRxStressStats(char* buf, size_t size, unsigned received, unsigned broadcasts) {
    snprintf(buf, size, "%u packets and %u broadcasts received, %u flash reads, Wi-Fi ready=%d, cloud connected=%d",
            received, broadcasts, (unsigned)s_flashReads, (int)WiFi.ready(), (int)Particle.connected());
}

#if Wiring_BLE == 1
Thread* s_bleScanThread = nullptr;
std::atomic<bool> s_bleScanStop{false};
std::atomic<bool> s_bleScanDone{false};

bool startBleScanning() {
    s_bleScanStop = false;
    s_bleScanDone = false;
    s_bleScanThread = new Thread("ble_scan", [](void* param) -> os_thread_return_t {
        while (!s_bleScanStop) {
            BLE.scan(+[](const BleScanResult* result, void* context) -> void {
            }, nullptr);
            delay(BLE_SCAN_RESTART_DELAY);
        }
        s_bleScanDone = true;
    }, nullptr);
    return s_bleScanThread != nullptr;
}

void stopBleScanning() {
    if (!s_bleScanThread) {
        return;
    }
    s_bleScanStop = true;
    while (!s_bleScanDone) {
        BLE.stopScanning();
        delay(BLE_SCAN_STOP_POLL_INTERVAL);
    }
    s_bleScanThread->join();
    delete s_bleScanThread;
    s_bleScanThread = nullptr;
}
#endif // Wiring_BLE == 1
#endif // HAL_PLATFORM_RTL872X

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

#if HAL_PLATFORM_RTL872X
test(WIFI_30_rx_stress_prepare) {
    Particle.disconnect();
    assertTrue(waitFor(Particle.disconnected, CLOUD_CONNECT_TIMEOUT));
#if Wiring_BLE == 1
    BLE.off();
#endif // Wiring_BLE == 1
    WiFi.off();
    assertTrue(waitFor(WiFi.isOff, WIFI_OFF_TIMEOUT));
    assertTrue(hogSramHeap() > 0);
    WiFi.on();
    WiFi.connect();
    assertTrue(waitFor(WiFi.ready, WIFI_CONNECT_TIMEOUT));
    Particle.connect();
    assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));
#if Wiring_BLE == 1
    BLE.on();
    assertEqual(BLE.advertise(), (int)SYSTEM_ERROR_NONE);
    assertEqual(BLE.setScanTimeout(BLE_SCAN_TIMEOUT), (int)SYSTEM_ERROR_NONE);
    assertTrue(startBleScanning());
#endif // Wiring_BLE == 1
    assertTrue(startFlashReads());
    assertTrue(s_rxStressUdp.begin(RX_STRESS_PORT));
    assertTrue(s_rxStressBroadcastUdp.begin(RX_STRESS_BROADCAST_PORT));
}

test(WIFI_31_rx_stress_keeps_receiving) {
    unsigned received = 0;
    unsigned broadcasts = 0;
    if (!System.waitCondition([&received]() {
        received += receivePackets(s_rxStressUdp);
        return received > 0;
    }, RX_STRESS_START_TIMEOUT)) {
        Test::out->printlnf("No RX stress packets in %u s", (unsigned)(RX_STRESS_START_TIMEOUT / 1000));
        fail();
        return;
    }
    static char stats[RX_STRESS_STATS_SIZE] = {};
    const auto start = millis();
    auto lastRx = start;
    for (auto now = start; now - start < RX_STRESS_DURATION; now = millis()) {
        const unsigned count = receivePackets(s_rxStressUdp);
        const unsigned broadcastCount = receivePackets(s_rxStressBroadcastUdp);
        received += count;
        broadcasts += broadcastCount;
        if (count || broadcastCount) {
            lastRx = now;
        } else if (now - lastRx >= RX_STALL_TIMEOUT) {
            formatRxStressStats(stats, sizeof(stats), received, broadcasts);
            const auto stalled = millis();
            unsigned resumed = 0;
            System.waitCondition([&resumed]() {
                resumed += receivePackets(s_rxStressUdp);
                return resumed > 0;
            }, RX_STALL_RECOVERY_TIMEOUT);
            const auto waited = millis() - stalled;
            static WiFiAccessPoint aps[RX_STALL_SCAN_MAX_APS] = {};
            const int found = WiFi.scan(aps, RX_STALL_SCAN_MAX_APS);
            Test::out->printlnf("RX stalled %u s into the stress: %s. After %u s RX %s, scan found %d APs",
                    (unsigned)((lastRx - start) / 1000), stats, (unsigned)(waited / 1000),
                    resumed ? "resumed" : "still stalled", found);
            fail();
            return;
        }
        delay(RX_STRESS_POLL_INTERVAL);
    }
    formatRxStressStats(stats, sizeof(stats), received, broadcasts);
    if (!s_flashReads || !broadcasts) {
        Test::out->printlnf("No flash reads or no broadcasts during the RX stress: %s", stats);
        fail();
        return;
    }
    assertEqual(0, pushMailboxMsg(stats, RX_STRESS_MAILBOX_TIMEOUT));
}

test(WIFI_32_rx_stress_cleanup) {
    s_rxStressUdp.stop();
    s_rxStressBroadcastUdp.stop();
#if Wiring_BLE == 1
    stopBleScanning();
#endif // Wiring_BLE == 1
    stopFlashReads();
    releaseSramHeap();
}
#endif // HAL_PLATFORM_RTL872X

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
