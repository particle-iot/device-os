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

/*
 *
 * Gen 3/P2 Wiring diagram
 *
 * Sleep20 Tester
 * Sleep20 Device
 *
 * Tester              Device
 * ----------          ----------
 * RESET   D5 -------> RST RESET
 * GPIO    D0 -------> D0
 * ADC     A1 -------> A1
 * WKP D8/D10 -------> D8/D10/WKP // Argon=D8, P2=D10
 * SERIAL1 TX -------> RX SERIAL1
 *        GND <------> GND
 *
 */

#include "application.h"
#include "unit-test/unit-test.h"
#include "util.h"
#include "random.h"

// BLE defines (borrowed from wiring/ble_central_peripheral test)
//---------------------------------------------------------------
#define SCAN_RESULT_COUNT       20
BleScanResult bleScanResults[SCAN_RESULT_COUNT];
BleCharacteristic peerCharRead;
BleCharacteristic peerCharWrite;
BleCharacteristic peerCharWriteWoRsp;
BleCharacteristic peerCharWriteAndWriteWoRsp;
BleCharacteristic peerCharNotify;
BleCharacteristic peerCharIndicate;
BleCharacteristic peerCharNotifyAndIndicate;
BlePeerDevice peer;
const String str1("05a9ae9588dd");
const String str2("4f62bd40046a");
const String str3("6619918032a2");
const String str4("359b4deb3f37");
const String str5("a2ef18ec6eaa");
const String str6("7223b5dd3342");
const String str7("d4b4249bbbe3");
bool str1Rec = false, str2Rec = false, str3Rec = false, str4Rec = false, str5Rec = false, str6Rec = false, str7Rec = false;
BleAddress peerAddr;
static void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
    String str((const char*)data, len);
    if (str == str1) {
        str1Rec = true;
    }
    if (str == str2) {
        str2Rec = true;
    }
    if (str == str3) {
        str3Rec = true;
    }
    if (str == str4) {
        str4Rec = true;
    }
    if (str == str5) {
        str5Rec = true;
    }
    if (str == str6) {
        str6Rec = true;
    }
    if (str == str7) {
        str7Rec = true;
    }
}
using namespace particle::test;
constexpr uint16_t LOCAL_DESIRED_ATT_MTU = 100;
constexpr uint16_t PEER_DESIRED_ATT_MTU = 123;
void bleConnect() {
    peerCharNotify.onDataReceived(onDataReceived, &peerCharNotify);
    peerCharIndicate.onDataReceived(onDataReceived, &peerCharIndicate);
    peerCharNotifyAndIndicate.onDataReceived(onDataReceived, &peerCharNotifyAndIndicate);

    int ret = BLE.setScanTimeout(100); // Scan timeout: 1s
    assertEqual(ret, 0);

    Log.trace("BLE starts scanning...");

    size_t wait = 20; // Scanning for 20s.
    while (!BLE.connected() && wait > 0) {
        int count = BLE.scan(bleScanResults, SCAN_RESULT_COUNT);
        if (count > 0) {
            for (int i = 0; i < count; i++) {
                BleUuid foundServiceUUID;
                size_t svcCount = bleScanResults[i].advertisingData().serviceUUID(&foundServiceUUID, 1);
#if defined(PARTICLE_TEST_RUNNER)
                if (bleScanResults[i].address() != BleAddress(getBleTestPeer().address, bleScanResults[i].address().type())) {
                    continue;
                }
#endif // PARTICLE_TEST_RUNNER
                if (svcCount > 0 && foundServiceUUID == "6E400000-B5A3-F393-E0A9-E50E24DCCA9E") {
                    assertTrue(bleScanResults[i].scanResponse().length() > 0);
                    BleUuid uuids[2];
                    assertEqual(bleScanResults[i].scanResponse().serviceUUID(uuids, 2), 2);
                    assertTrue(uuids[0] == 0x1234);
                    assertTrue(uuids[1] == 0x5678);
                    peerAddr = bleScanResults[i].address();
                    Log.trace("Connecting...");
                    peer = BLE.connect(peerAddr);
                    if (peer.connected()) {
                        assertTrue(peer.getCharacteristicByDescription(peerCharRead, "read"));
                        assertTrue(peer.getCharacteristicByDescription(peerCharWrite, "write"));
                        assertTrue(peer.getCharacteristicByDescription(peerCharWriteWoRsp, "write_wo_rsp"));
                        assertTrue(peer.getCharacteristicByDescription(peerCharWriteAndWriteWoRsp, "write_write_wo_rsp"));
                        assertTrue(peer.getCharacteristicByUUID(peerCharNotify, "6E400005-B5A3-F393-E0A9-E50E24DCCA9E"));
                        assertTrue(peer.getCharacteristicByUUID(peerCharIndicate, "6E400006-B5A3-F393-E0A9-E50E24DCCA9E"));
                        assertTrue(peer.getCharacteristicByUUID(peerCharNotifyAndIndicate, "6E400007-B5A3-F393-E0A9-E50E24DCCA9E"));
                    }
                    break;
                }
            }
        }
        wait--;
    }
    assertTrue(wait > 0);
    Log.trace("BLE connected.");
}

//TODO: Run user/tests/app/tracker_wakeup to verify the additional wakeup sources for platform tracker.

SYSTEM_MODE(SEMI_AUTOMATIC);
STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY));

namespace {

static retained uint32_t magick = 0;
static retained uint32_t phase = 0;

static retained time32_t enterTime = 0;
static          time32_t exitTime = 0;

constexpr system_tick_t CLOUD_CONNECT_TIMEOUT = 10 * 60 * 1000;
constexpr system_tick_t CLOUD_DISCONNECT_TIMEOUT = 1 * 60 * 1000;

time32_t sNetworkOffTimestamp = 0;

constexpr int SLEEP_PIN_RESET = D5;
constexpr int SLEEP_PIN_GPIO_WAKE_UP = D0;
constexpr int SLEEP_PIN_ADC_WAKE_UP = A1;
constexpr int SLEEP_PIN_WKP_WAKE_UP = WKP; // Argon=D8, P2=D10

} // anonymous

void updateTime() {
    exitTime = Time.now();
}

STARTUP(updateTime());

void assertWakeUp(int pin) {
    switch (pin) {
        case SLEEP_PIN_RESET: {
            hal_gpio_write(SLEEP_PIN_RESET, 0);
            delay(1000);
            hal_gpio_write(SLEEP_PIN_RESET, 1);
            delay(1000);
            break;
        }
        case SLEEP_PIN_GPIO_WAKE_UP: {
            hal_gpio_write(SLEEP_PIN_GPIO_WAKE_UP, 1);
            delay(500);
            hal_gpio_write(SLEEP_PIN_GPIO_WAKE_UP, 0);
            delay(500);
            break;
        }
        case SLEEP_PIN_ADC_WAKE_UP: {
            hal_gpio_write(SLEEP_PIN_ADC_WAKE_UP, 1);
            delay(500);
            hal_gpio_write(SLEEP_PIN_ADC_WAKE_UP, 0);
            delay(500);
            break;
        }
        case SLEEP_PIN_WKP_WAKE_UP: {
            hal_gpio_write(SLEEP_PIN_WKP_WAKE_UP, 1);
            delay(500);
            hal_gpio_write(SLEEP_PIN_WKP_WAKE_UP, 0);
            delay(500);
            break;
        }
        default: {
            break;
        }
    }
}

test(000_System_Sleep_Central_Cloud_Connect) {
    subscribeEvents(BLE_ROLE_CENTRAL);
    Particle.connect();
    assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));
    assertTrue(publishBlePeerInfo());
    assertEqual(BLE.setDesiredAttMtu(LOCAL_DESIRED_ATT_MTU), (int)SYSTEM_ERROR_NONE);
}

test(000_System_Sleep_Prepare) {
    hal_gpio_config_t conf = {
        .size = sizeof(conf),
        .version = HAL_GPIO_VERSION,
        .mode = OUTPUT_OPEN_DRAIN,
        .set_value = true,
        .value = 1,
        .drive_strength = HAL_GPIO_DRIVE_DEFAULT
    };
    hal_gpio_configure(SLEEP_PIN_RESET, &conf, nullptr); // default HI-Z, active LOW
    pinMode(SLEEP_PIN_GPIO_WAKE_UP, OUTPUT); // default LOW, active HIGH
    pinMode(SLEEP_PIN_ADC_WAKE_UP, OUTPUT); // default LOW, active HIGH
    pinMode(SLEEP_PIN_WKP_WAKE_UP, OUTPUT); // default LOW, active HIGH

#ifndef PARTICLE_TEST_RUNNER
    for (int i = 0; i < 60; i++) {
        assertTrue(publishBlePeerInfo());
        if (getBleTestPeer().isValid()) {
            break;
        }
        delay(1000);
    }
    assertTrue(getBleTestPeer().isValid());
#else
    assertTrue(waitFor(getBleTestPeer().isValid, 60 * 1000));
#endif // PARTICLE_TEST_RUNNER
}

test(01_System_Sleep_With_Configuration_Object_Hibernate_Mode_Without_Wakeup_1) {
    delay(5000); // wait for device to sleep
    assertWakeUp(SLEEP_PIN_RESET);
}
test(01_System_Sleep_With_Configuration_Object_Hibernate_Mode_Without_Wakeup_2) {
}

test(02_System_Sleep_Mode_Deep_Without_Wakeup_1) {
    delay(5000); // wait for device to sleep
    assertWakeUp(SLEEP_PIN_RESET);
}
test(02_System_Sleep_Mode_Deep_Without_Wakeup_2) {
}

#if !HAL_PLATFORM_RTL872X
test(03_System_Sleep_With_Configuration_Object_Hibernate_Mode_Wakeup_By_D0_1) {
    delay(5000); // wait for device to sleep
    assertWakeUp(SLEEP_PIN_GPIO_WAKE_UP);
}
test(03_System_Sleep_With_Configuration_Object_Hibernate_Mode_Wakeup_By_D0_2) {
}

test(04_System_Sleep_Mode_Deep_Wakeup_By_WKP_Pin_1) {
    delay(5000); // wait for device to sleep
    assertWakeUp(SLEEP_PIN_WKP_WAKE_UP);
}
test(04_System_Sleep_Mode_Deep_Wakeup_By_WKP_Pin_2) {
}

test(05_System_Sleep_With_Configuration_Object_Hibernate_Mode_Wakeup_By_Analog_Pin_1) {
    delay(5000); // wait for device to sleep
    assertWakeUp(SLEEP_PIN_ADC_WAKE_UP);
}
test(05_System_Sleep_With_Configuration_Object_Hibernate_Mode_Wakeup_By_Analog_Pin_2) {
}

// Tracker support waking up device from hibernate mode by external RTC
#if HAL_PLATFORM_EXTERNAL_RTC
test(06_System_Sleep_With_Configuration_Object_Hibernate_Mode_Wakeup_By_External_Rtc) {
}

test(07_System_Sleep_Mode_Deep_Wakeup_By_External_Rtc) {
}
#endif // HAL_PLATFORM_EXTERNAL_RTC
#endif // !HAL_PLATFORM_RTL872X

#if HAL_PLATFORM_RTL872X
test(08_System_Sleep_With_Configuration_Object_Hibernate_Mode_Wakeup_By_Wkp_Pin_1) {
    delay(5000); // wait for device to sleep
    assertWakeUp(SLEEP_PIN_WKP_WAKE_UP);
}
test(08_System_Sleep_With_Configuration_Object_Hibernate_Mode_Wakeup_By_Wkp_Pin_2) {
}

test(09_System_Sleep_With_Configuration_Object_Hibernate_Mode_Wakeup_By_Rtc) {
}

test(10_System_Sleep_Mode_Deep_Wakeup_By_Wkp_Pin_1) {
    delay(5000); // wait for device to sleep
    assertWakeUp(SLEEP_PIN_WKP_WAKE_UP);
}
test(10_System_Sleep_Mode_Deep_Wakeup_By_Wkp_Pin_2) {
}

test(11_System_Sleep_Mode_Deep_Wakeup_By_Rtc) {
}

test(12_System_Sleep_With_Configuration_Object_Hibernate_Mode_Bypass_Network_Off_Execution_Time_1) {
    // TODO: WAIT FOR DEVICE TO BE CONNECTED TO THE CLOUD
    delay(20000); // wait for device to connect to cloud, sleep and wake
}
test(12_System_Sleep_With_Configuration_Object_Hibernate_Mode_Bypass_Network_Off_Execution_Time_2) {
}
#endif // HAL_PLATFORM_RTL872X

test(13_System_Sleep_With_Configuration_Object_Stop_Mode_Without_Wakeup) {
}

test(14_System_Sleep_With_Configuration_Object_Stop_Mode_Without_Wakeup) {
}

test(15_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Without_Wakeup) {
}

test(16_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_D0_1) {
    delay(5000); // wait for device to sleep
    assertWakeUp(SLEEP_PIN_GPIO_WAKE_UP);
}
test(16_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_D0_2) {
}

test(17_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_Rtc) {
}

#if HAL_PLATFORM_BLE && !HAL_PLATFORM_RTL872X
test(18_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_Ble_1) {
    bleConnect();
}
test(18_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_Ble_2) {
    delay(5000);
    BLE.disconnect(peer);
}
#endif // HAL_PLATFORM_BLE && !HAL_PLATFORM_RTL872X

test(19_System_Sleep_Mode_Stop_Wakeup_By_D0_1) {
    delay(5000); // wait for device to sleep
    assertWakeUp(SLEEP_PIN_GPIO_WAKE_UP);
}
test(19_System_Sleep_Mode_Stop_Wakeup_By_D0_2) {
}

test(20_System_Sleep_Mode_Stop_Wakeup_By_Rtc) {
    delay(5000); // wait for device to sleep and wake
}
test(20_System_Sleep_Mode_Stop_Wakeup_By_Rtc_2) {
}

test(21_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_D0_1) {
    delay(5000); // wait for device to sleep
    assertWakeUp(SLEEP_PIN_GPIO_WAKE_UP);
}
test(21_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_D0_2) {

}

test(22_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_Rtc) {
}

#if !HAL_PLATFORM_RTL872X

#if HAL_PLATFORM_BLE
test(23_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_Ble_1) {
    bleConnect();
}
test(23_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_Ble_2) {
    delay(5000);
    BLE.disconnect(peer);
}
#endif // HAL_PLATFORM_BLE

test(24_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_Analog_Pin_1) {
    delay(5000); // wait for device to sleep
    assertWakeUp(SLEEP_PIN_ADC_WAKE_UP);
}
test(24_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_Analog_Pin_2) {
}

test(25_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_Analog_Pin_1) {
    delay(5000); // wait for device to sleep
    assertWakeUp(SLEEP_PIN_ADC_WAKE_UP);
}
test(25_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_Analog_Pin_2) {
}

test(26_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_Usart_1) {
    delay(5000);
    Serial1.begin(115200);
    Serial1.write('t');
    Serial1.end();
}
test(26_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_Usart_2) {
}

SystemSleepResult result27;
test(27_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_Usart_1) {
    delay(5000);
    Serial1.begin(115200);
    Serial1.write('t');
    Serial1.end();
}
test(27_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_Usart_2) {
}

#if HAL_PLATFORM_CELLULAR
test(28_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_Cellular_1) {
    if (Particle.connected()) {
        Particle.disconnect();
        assertTrue(waitFor(Particle.disconnected, CLOUD_DISCONNECT_TIMEOUT));
    }
    subscribePeerEvents(BLE_ROLE_CENTRAL);
    Particle.connect();
    assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));

    // wait for device to connect to cloud, publish a ready message "test=28", and sleep
    auto s = millis();
    while (true) {
        Particle.process();
        delay(100);
        if (getBleTestPeer().test == 28) {
            break;
        }
        if (millis() - s > CLOUD_CONNECT_TIMEOUT) {
            assertEqual(strcmp("sleep20_tester timeout waiting for sleep20_device to connect to cloud!",""), 0);
        }
    }
    delay(10000); // wait for message flush and sleep
    assertTrue(publishBlePeerInfo(28)); // respond with test=28 to wake the device
}
test(28_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_Cellular_2) {
    resetBleTestPeerTest();
}

test(29_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_Cellular_1) {
    if (Particle.connected()) {
        Particle.disconnect();
        assertTrue(waitFor(Particle.disconnected, CLOUD_DISCONNECT_TIMEOUT));
    }
    subscribePeerEvents(BLE_ROLE_CENTRAL);
    Particle.connect();
    assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));

    // wait for device to connect to cloud, publish a ready message "test=29", and sleep
    auto s = millis();
    while (true) {
        Particle.process();
        delay(100);
        if (getBleTestPeer().test == 29) {
            break;
        }
        if (millis() - s > CLOUD_CONNECT_TIMEOUT) {
            assertEqual(strcmp("sleep20_tester timeout waiting for sleep20_device to connect to cloud!",""), 0);
        }
    }
    delay(10000); // wait for message flush and sleep
    assertTrue(publishBlePeerInfo(29)); // respond with test=29 to wake the device
}
test(29_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_Cellular_2) {
    resetBleTestPeerTest();
}
#endif // HAL_PLATFORM_CELLULAR

#if HAL_PLATFORM_WIFI
test(30_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_WiFi_1) {
    if (Particle.connected()) {
        Particle.disconnect();
        assertTrue(waitFor(Particle.disconnected, CLOUD_DISCONNECT_TIMEOUT));
    }
    subscribePeerEvents(BLE_ROLE_CENTRAL);
    Particle.connect();
    assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));

    // wait for device to connect to cloud, publish a ready message "test=30", and sleep
    auto s = millis();
    while (true) {
        Particle.process();
        delay(100);
        if (getBleTestPeer().test == 30) {
            break;
        }
        if (millis() - s > CLOUD_CONNECT_TIMEOUT) {
            assertEqual(strcmp("sleep20_tester timeout waiting for sleep20_device to connect to cloud!",""), 0);
        }
    }
    delay(15000); // wait for message flush and sleep
    assertTrue(publishBlePeerInfo(30)); // respond with test=30 to wake the device
}
test(30_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_WiFi_2) {
    resetBleTestPeerTest();
}

test(31_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_WiFi_1) {
    if (Particle.connected()) {
        Particle.disconnect();
        assertTrue(waitFor(Particle.disconnected, CLOUD_DISCONNECT_TIMEOUT));
    }
    subscribePeerEvents(BLE_ROLE_CENTRAL);
    Particle.connect();
    assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));

    // wait for device to connect to cloud, publish a ready message "test=31", and sleep
    auto s = millis();
    while (true) {
        Particle.process();
        delay(100);
        if (getBleTestPeer().test == 31) {
            break;
        }
        if (millis() - s > CLOUD_CONNECT_TIMEOUT) {
            assertEqual(strcmp("sleep20_tester timeout waiting for sleep20_device to connect to cloud!",""), 0);
        }
    }
    delay(15000); // wait for message flush and sleep
    assertTrue(publishBlePeerInfo(31)); // respond with test=31 to wake the device
}
test(31_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_WiFi_2) {
    resetBleTestPeerTest();
}
#endif // HAL_PLATFORM_WIFI

#endif // !HAL_PLATFORM_RTL872X

test(32_System_Sleep_With_Configuration_Object_Execution_Time_Prepare) {
    /* This test should only be run with threading disabled */
    if (system_thread_get_state(nullptr) == spark::feature::ENABLED) {
        skip();
        return;
    }
}

test(33_System_Sleep_With_Configuration_Object_Stop_Mode_Execution_Time) {
}

test(34_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_Execution_Time) {
}

test(35_System_Sleep_With_Configuration_Object_Network_Power_State_Consistent_On) {
}

test(36_System_Sleep_With_Configuration_Object_Network_Power_State_Consistent_Off) {
}
