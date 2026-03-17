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
#include "test_suite.h"
#include "check.h"
#include "muon_test_util.h"

// Serial1LogHandler logHandler(115200, LOG_LEVEL_ALL, {
//     // { "comm.coap", LOG_LEVEL_ALL },
//     // { "app", LOG_LEVEL_ALL }
// });

namespace {

#if HAL_PLATFORM_ETHERNET

retained bool skipEthernet = false;

bool isEthernetPresent() {
    if_t iface = nullptr;
    if (if_get_by_index(NETWORK_INTERFACE_ETHERNET, &iface)) {
        return false;
    }

    unsigned flags = 0;
    if_get_flags(iface, &flags);

    if (flags & (IFF_LOWER_UP | IFF_UP) == (IFF_LOWER_UP | IFF_UP)) {
        return true;
    }

    Ethernet.on();
    Ethernet.connect();

    bool ok = false;
    for (auto start = millis(); millis() - start <= 10000;) {
        flags = 0;
        if_get_flags(iface, &flags);
        if (flags & IFF_LOWER_UP) {
            ok = true;
            break;
        }
    }

    Ethernet.disconnect();
    Ethernet.off();

    return ok;
}

#endif // HAL_PLATFORM_ETHERNET

#if HAL_PLATFORM_BLE
int validateBleScan() {
    CHECK(BLE.on());

    BleScanParams params = {};
    params.size = sizeof(BleScanParams);
    params.timeout = 500; // *10ms = 5s overall duration
    params.interval = 8000; // *0.625ms = 5s
    params.window = 8000; // *0.625 = 5s
    params.active = true; // Send scan request
    params.filter_policy = BLE_SCAN_FP_ACCEPT_ALL;
    CHECK(BLE.setScanParameters(&params));

    struct BleScanData {
        size_t results;
    };
    BleScanData data = {};

    for (int i = 0; i < 10; i++) {
        auto r = BLE.scan(+[](const BleScanResult *result, void *context) -> void {
            auto data = (BleScanData*)context;
            data->results++;
        }, &data);
        if (r != 0) {
            return r;
        }
    }
    return data.results;
}

const system_tick_t LISTENING_MODE_STATE_CHANGE_TIMEOUT = 30000;

bool waitListening(bool state, system_tick_t timeout = LISTENING_MODE_STATE_CHANGE_TIMEOUT) {
    if (state) {
        return waitFor(Network.listening, timeout);
    } else {
        return waitForNot(Network.listening, timeout);
    }
}

#endif // HAL_PLATFORM_BLE

enum class FirmwareUpdateStatus {
    NONE,
    STARTED,
    SUCCESS,
    ERROR
};

auto firmwareUpdateStatus = FirmwareUpdateStatus::NONE;
std::atomic<int> firmwareUpdateProgressCount;

void firmwareUpdateEventHandler(system_event_t, int data, void*) {
    switch (data) {
    case firmware_update_begin:
        Test::out->println("firmware_update_begin");
        firmwareUpdateStatus = FirmwareUpdateStatus::STARTED;
        break;
    case firmware_update_complete:
        Test::out->println("firmware_update_complete");
        firmwareUpdateStatus = FirmwareUpdateStatus::SUCCESS;
        break;
    case firmware_update_progress:
        ++firmwareUpdateProgressCount;
        break;
    default:
        Test::out->printlnf("Unexpected firmware update status: %d", data);
        firmwareUpdateStatus = FirmwareUpdateStatus::ERROR;
        break;
    }
}

void prepareForFirmwareUpdate() {
    System.disableReset();
    System.on(firmware_update, firmwareUpdateEventHandler);
    firmwareUpdateStatus = FirmwareUpdateStatus::NONE;
    firmwareUpdateProgressCount = 0;
}

void completeFirmwareUpdate(bool expectSafeMode = false) {
    bool ok = false;
    auto t1 = millis();
    for (;;) {
        Particle.process();
        if (firmwareUpdateStatus == FirmwareUpdateStatus::SUCCESS) {
            ok = true;
            break;
        }
        if (firmwareUpdateStatus == FirmwareUpdateStatus::ERROR) {
            Test::out->println("Firmware update failed");
            break;
        }
        // The JS part of the test waits until the OTA completes so the timeout here is for
        // finalizing the update on the device
        if (millis() - t1 >= 30000) {
            Test::out->println("Firmware update timeout");
            break;
        }
    }
    Test::out->printlnf("firmware_update_progress count: %d", firmwareUpdateProgressCount.load());
    System.off(firmware_update);
    if (ok) {
        if (expectSafeMode) {
            TestRunner::instance()->expectSafeMode();
        } else {
            TestRunner::instance()->expectSystemReset();
        }
    }
    assertTrue(ok);
    System.enableReset();
}

} // anonymous

#if HAL_PLATFORM_BLE
test(01_particle_ble_enable_init) {
    // Just in case
    System.disableFeature(FEATURE_DISABLE_LISTENING_MODE);
    System.clearEnv(false /* reset */);
    expectSystemReset();
    System.reset();
}

test(02_particle_ble_enable_default) {
    auto vars = System.listEnv();
    assertEqual(vars.size(), 0);

    assertFalse(System.hasEnv("PARTICLE_BLUETOOTH_ENABLE"));
    assertMore(validateBleScan(), 0);

    Network.listen();
    SCOPE_GUARD({
        Network.listen(false);
        assertTrue(waitListening(false));
    });
    assertTrue(waitListening(true));
    assertTrue(BLE.advertising());
}

test(03_particle_ble_enable_true) {
    assertTrue(System.hasEnv("PARTICLE_BLUETOOTH_ENABLE"));
    assertEqual(System.getEnv("PARTICLE_BLUETOOTH_ENABLE"), String("true"));
    assertMore(validateBleScan(), 0);

    Network.listen();
    SCOPE_GUARD({
        Network.listen(false);
        assertTrue(waitListening(false));
    });
    assertTrue(waitListening(true));
    assertTrue(BLE.advertising());
}

test(04_particle_ble_enable_false) {
    assertTrue(System.hasEnv("PARTICLE_BLUETOOTH_ENABLE"));
    assertEqual(System.getEnv("PARTICLE_BLUETOOTH_ENABLE"), String("false"));
    assertLess(validateBleScan(), 0);

    Network.listen();
    SCOPE_GUARD({
        Network.listen(false);
        assertTrue(waitListening(false));
    });
    assertTrue(waitListening(true));
    assertFalse(BLE.advertising());
    Network.listen(false);
    assertTrue(waitListening(false));

#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
    if (TestSuite::instance()->network() == NETWORK_INTERFACE_WIFI_STA ||
        TestSuite::instance()->network() == NETWORK_INTERFACE_ALL) {
        // WiFi is not affected
        System.disableUpdates();
        SCOPE_GUARD({
            Particle.disconnect();
            waitForNot(Particle.connected, 1000);
            System.enableUpdates();
        });

        WiFi.on();
        assertTrue(waitFor(WiFi.isOn, 5000));
        WiFi.connect();
        Particle.connect();
        assertTrue(waitFor(WiFi.ready, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
        assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    }
#endif // HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
}

#endif // HAL_PLATFORM_BLE

#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
test(05_particle_wifi_enable_init) {
    System.disableFeature(FEATURE_DISABLE_LISTENING_MODE);
    System.enableFeature(FEATURE_ETHERNET_DETECTION);
#if HAL_PLATFORM_HW_FORM_FACTOR_SOM
    if (particle::test::detectMuonBoard()) {
        particle::test::configureMuonEthernet();
    }
#endif // HAL_PLATFORM_HW_FORM_FACTOR_SOM
    System.clearEnv(false /* reset */);
    expectSystemReset();
    System.reset();
}

test(06_particle_wifi_enable_default) {
    auto vars = System.listEnv();
    assertEqual(vars.size(), 0);

    assertFalse(System.hasEnv("PARTICLE_WIFI_ENABLE"));

    assertEqual((int)TestSuite::instance()->network(), (int)NETWORK_INTERFACE_WIFI_STA);

    System.disableUpdates();
    SCOPE_GUARD({
        Particle.disconnect();
        waitForNot(Particle.connected, 1000);
        System.enableUpdates();
    });

    WiFi.on();
    assertTrue(waitFor(WiFi.isOn, 5000));
    WiFi.connect();
    Particle.connect();
    assertTrue(waitFor(WiFi.ready, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

#if HAL_PLATFORM_BLE
    // BLE is not affected
    assertMore(validateBleScan(), 0);
    Network.listen();
    SCOPE_GUARD({
        Network.listen(false);
        assertTrue(waitListening(false));
    });
    assertTrue(waitListening(true));
    assertTrue(BLE.advertising());
#endif // HAL_PLATFORM_BLE
}

test(07_particle_wifi_enable_true) {
    assertTrue(System.hasEnv("PARTICLE_WIFI_ENABLE"));
    assertEqual(System.getEnv("PARTICLE_WIFI_ENABLE"), String("true"));

    assertEqual((int)TestSuite::instance()->network(), (int)NETWORK_INTERFACE_WIFI_STA);

    System.disableUpdates();
    SCOPE_GUARD({
        Particle.disconnect();
        waitForNot(Particle.connected, 1000);
        System.enableUpdates();
    });

    WiFi.on();
    assertTrue(waitFor(WiFi.isOn, 5000));
    WiFi.connect();
    Particle.connect();
    assertTrue(waitFor(WiFi.ready, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

#if HAL_PLATFORM_BLE
    // BLE is not affected
    assertMore(validateBleScan(), 0);
    Network.listen();
    SCOPE_GUARD({
        Network.listen(false);
        assertTrue(waitListening(false));
    });
    assertTrue(waitListening(true));
    assertTrue(BLE.advertising());
#endif // HAL_PLATFORM_BLE
}

test(08_particle_wifi_enable_false) {
    assertTrue(System.hasEnv("PARTICLE_WIFI_ENABLE"));
    assertEqual(System.getEnv("PARTICLE_WIFI_ENABLE"), String("false"));

    assertEqual((int)TestSuite::instance()->network(), (int)NETWORK_INTERFACE_WIFI_STA);

    System.disableUpdates();
    SCOPE_GUARD({
        Particle.disconnect();
        waitForNot(Particle.connected, 1000);
        System.enableUpdates();
    });

    WiFi.on();
    assertFalse(waitFor(WiFi.isOn, 5000));
    WiFi.connect();
    Particle.connect();
    assertFalse(waitFor(WiFi.ready, 5000));
    assertFalse(Particle.connected());

#if HAL_PLATFORM_BLE
    // BLE is not affected
    assertMore(validateBleScan(), 0);
    Network.listen();
    SCOPE_GUARD({
        Network.listen(false);
        assertTrue(waitListening(false));
    });
    assertTrue(waitListening(true));
    assertTrue(BLE.advertising());
#endif // HAL_PLATFORM_BLE
}

test(09_particle_wifi_enable_false_connect_through_other_ifaces) {
    bool shouldConnect = false;
#if HAL_PLATFORM_CELLULAR
    shouldConnect = TestSuite::instance()->network() == NETWORK_INTERFACE_CELLULAR || TestSuite::instance()->network() == NETWORK_INTERFACE_ALL;
#endif // HAL_PLATFORM_CELLULAR
#if HAL_PLATFORM_ETHERNET
    if (isEthernetPresent()) {
        shouldConnect = true;
    }
#endif // HAL_PLATFORM_ETHERNET

    if (!shouldConnect) {
        skip();
        return;
    }

    assertTrue(System.hasEnv("PARTICLE_WIFI_ENABLE"));
    assertEqual(System.getEnv("PARTICLE_WIFI_ENABLE"), String("false"));

    assertNotEqual((int)TestSuite::instance()->network(), (int)NETWORK_INTERFACE_WIFI_STA);

    System.disableUpdates();
    SCOPE_GUARD({
        Particle.disconnect();
        waitForNot(Particle.connected, 1000);
        System.enableUpdates();
    });

    Network.on();
    Network.connect();
    Particle.connect();
    assertFalse(waitFor(WiFi.isOn, 5000));
    assertFalse(waitFor(WiFi.ready, 5000));

    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

#if HAL_PLATFORM_BLE
    // BLE is not affected
    assertMore(validateBleScan(), 0);
    Network.listen();
    SCOPE_GUARD({
        Network.listen(false);
        assertTrue(waitListening(false));
    });
    assertTrue(waitListening(true));
    assertTrue(BLE.advertising());
#endif // HAL_PLATFORM_BLE
}

test(10_particle_wifi_enable_cleanup) {
#if HAL_PLATFORM_ETHERNET
    System.disableFeature(FEATURE_ETHERNET_DETECTION);
#if HAL_PLATFORM_HW_FORM_FACTOR_SOM
    if (particle::test::detectMuonBoard()) {
        particle::test::unconfigureMuonEthernet();
    }
#endif // HAL_PLATFORM_HW_FORM_FACTOR_SOM
    expectSystemReset();
    System.reset();
#endif // HAL_PLATFORM_ETHERNET
}
#endif // HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY

#if HAL_PLATFORM_ETHERNET
test(11_particle_ethernet_enable_init) {
    System.disableFeature(FEATURE_DISABLE_LISTENING_MODE);
    System.enableFeature(FEATURE_ETHERNET_DETECTION);
#if HAL_PLATFORM_HW_FORM_FACTOR_SOM
    if (particle::test::detectMuonBoard()) {
        particle::test::configureMuonEthernet();
    }
#endif // HAL_PLATFORM_HW_FORM_FACTOR_SOM
    skipEthernet = false;
    System.clearEnv(false /* reset */);
    expectSystemReset();
    System.reset();
}

test(12_particle_ethernet_enable_default) {
    if (!isEthernetPresent()) {
        skipEthernet = true;
        skip();
        return;
    }
    auto vars = System.listEnv();
    assertEqual(vars.size(), 0);

    assertFalse(System.hasEnv("PARTICLE_ETHERNET_ENABLE"));

    assertEqual((int)TestSuite::instance()->network(), (int)NETWORK_INTERFACE_ETHERNET);

    System.disableUpdates();
    SCOPE_GUARD({
        Particle.disconnect();
        waitForNot(Particle.connected, 1000);
        System.enableUpdates();
    });

    Ethernet.on();
    assertTrue(waitFor(Ethernet.isOn, 5000));
    Ethernet.connect();
    Particle.connect();
    assertTrue(waitFor(Ethernet.ready, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
}

test(13_particle_ethernet_enable_true) {
    if (skipEthernet) {
        skip();
        return;
    }
    assertTrue(System.hasEnv("PARTICLE_ETHERNET_ENABLE"));
    assertEqual(System.getEnv("PARTICLE_ETHERNET_ENABLE"), String("true"));

    assertEqual((int)TestSuite::instance()->network(), (int)NETWORK_INTERFACE_ETHERNET);

    System.disableUpdates();
    SCOPE_GUARD({
        Particle.disconnect();
        waitForNot(Particle.connected, 1000);
        System.enableUpdates();
    });

    Ethernet.on();
    assertTrue(waitFor(Ethernet.isOn, 5000));
    Ethernet.connect();
    Particle.connect();
    assertTrue(waitFor(Ethernet.ready, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
}

test(14_particle_ethernet_enable_false) {
    if (skipEthernet) {
        skip();
        return;
    }
    assertTrue(System.hasEnv("PARTICLE_ETHERNET_ENABLE"));
    assertEqual(System.getEnv("PARTICLE_ETHERNET_ENABLE"), String("false"));

    assertEqual((int)TestSuite::instance()->network(), (int)NETWORK_INTERFACE_ETHERNET);

    assertFalse(isEthernetPresent());
}

test(15_particle_ethernet_enable_false_connect_through_other_ifaces) {
    if (skipEthernet) {
        skip();
        return;
    }
    assertTrue(System.hasEnv("PARTICLE_ETHERNET_ENABLE"));
    assertEqual(System.getEnv("PARTICLE_ETHERNET_ENABLE"), String("false"));

    assertEqual((int)TestSuite::instance()->network(), (int)0);

    assertFalse(isEthernetPresent());

    System.disableUpdates();
    SCOPE_GUARD({
        Particle.disconnect();
        waitForNot(Particle.connected, 1000);
        System.enableUpdates();
    });

    Network.on();
    Network.connect();
    Particle.connect();
    assertFalse(Ethernet.isOn());
    assertFalse(Ethernet.ready());

    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
}

test(16_particle_ethernet_enable_cleanup) {
    System.disableFeature(FEATURE_ETHERNET_DETECTION);
#if HAL_PLATFORM_HW_FORM_FACTOR_SOM
    if (particle::test::detectMuonBoard()) {
        particle::test::configureMuonEthernet();
    }
#endif // HAL_PLATFORM_HW_FORM_FACTOR_SOM
    expectSystemReset();
    System.reset();
}
#endif // HAL_PLATFORM_ETHERNET

test(97_cleanup) {
    System.clearEnv(false /* reset */);
    unlink("/sys/env_app");
    unlink("/sys/env_app.staged");
    unlink("/sys/env_snapshot");
    unlink("/sys/env_snapshot.staged");
    expectSystemReset();
    System.reset();
}

test(98_cleanup) {
    prepareForFirmwareUpdate();
    Particle.disconnect(CloudDisconnectOptions().clearSession(true));
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    // We are supposed to get an empty env
}

test(99_cleanup_1) {
    completeFirmwareUpdate();
}

test(99_cleanup_2) {
    if (firmwareUpdateStatus != FirmwareUpdateStatus::SUCCESS) {
        expectSystemReset();
        System.enableReset();
        // Should normally be unreachable
        delay(5000);
        System.reset();
    }
}
