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
void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);
const char* serviceUuid = "6E400000-B5A3-F393-E0A9-E50E24DCCA9E";
const char* charReadUuid = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
const char* charWriteUuid = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
const char* charWriteWoRspUuid = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";
const char* charWriteAndWriteWoRspUuid = "6E400004-B5A3-F393-E0A9-E50E24DCCA9E";
const char* charNotifyUuid = "6E400005-B5A3-F393-E0A9-E50E24DCCA9E";
const char* charIndicateUuid = "6E400006-B5A3-F393-E0A9-E50E24DCCA9E";
const char* charNotifyAndIndicateUuid = "6E400007-B5A3-F393-E0A9-E50E24DCCA9E";
const String str1("6b4bf92a37f3");
const String str2("df3b41caedac");
const String str3("2ad4bffbb8c7");
const String str4("203a02992be0");
const String str5("86d7a840079f");
const String str6("77982c283c65");
const String str7("21ec57d28a0c");
bool str1Rec = false, str2Rec = false, str3Rec = false, str4Rec = false, str5Rec = false, str6Rec = false, str7Rec = false;
bool pairingStarted = false;
BleCharacteristic charRead("read", BleCharacteristicProperty::READ, charReadUuid, serviceUuid);
BleCharacteristic charWrite("write", BleCharacteristicProperty::WRITE, charWriteUuid, serviceUuid, onDataReceived, &charWrite);
BleCharacteristic charWriteWoRsp("write_wo_rsp", BleCharacteristicProperty::WRITE_WO_RSP, charWriteWoRspUuid, serviceUuid, onDataReceived, &charWriteWoRsp);
BleCharacteristic charWriteAndWriteWoRsp("write_write_wo_rsp", BleCharacteristicProperty::WRITE_WO_RSP | BleCharacteristicProperty::WRITE, charWriteAndWriteWoRspUuid, serviceUuid, onDataReceived, &charWriteAndWriteWoRsp);
BleCharacteristic charNotify("notify", BleCharacteristicProperty::NOTIFY, charNotifyUuid, serviceUuid);
BleCharacteristic charIndicate("indicate", BleCharacteristicProperty::INDICATE, charIndicateUuid, serviceUuid);
BleCharacteristic charNotifyAndIndicate("notify_indicate", BleCharacteristicProperty::NOTIFY | BleCharacteristicProperty::INDICATE, charNotifyAndIndicateUuid, serviceUuid);
void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
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
    if (str == "deadbeef") {
        pairingStarted = true;
    }
}
using namespace particle::test;
constexpr uint16_t LOCAL_DESIRED_ATT_MTU = 123;
constexpr uint16_t PEER_DESIRED_ATT_MTU = 100;
bool bleConfigured = false;
void bleConfigureAndStartAdvertising() {
    int ret;
    BleCharacteristic temp;

    temp = BLE.addCharacteristic(charRead);
    assertTrue(temp.isValid());
    ret = charRead.setValue("6dd902629e1d");
    assertEqual(ret, 12);
    temp = BLE.addCharacteristic(charWrite);
    assertTrue(temp.isValid());
    temp = BLE.addCharacteristic(charWriteWoRsp);
    assertTrue(temp.isValid());
    temp = BLE.addCharacteristic(charWriteAndWriteWoRsp);
    assertTrue(temp.isValid());
    temp = BLE.addCharacteristic(charNotify);
    assertTrue(temp.isValid());
    temp = BLE.addCharacteristic(charIndicate);
    assertTrue(temp.isValid());
    temp = BLE.addCharacteristic(charNotifyAndIndicate);
    assertTrue(temp.isValid());

    BleAdvertisingData advData;
    advData.appendServiceUUID(serviceUuid);
    BleAdvertisingData srData;
    uint8_t uuids[] = {0x34, 0x12, 0x78, 0x56}; // little endian, i.e. 0x1234 and 0x5678.
    srData.append(BleAdvertisingDataType::SERVICE_UUID_16BIT_MORE_AVAILABLE, uuids, sizeof(uuids));
    ret = BLE.advertise(&advData, &srData);
    assertEqual(ret, 0);

    BLE.onConnected([](const BlePeerDevice& peer) {
        Log.trace("Connected.");
    });
    BLE.onDisconnected([](const BlePeerDevice& peer) {
        Log.trace("Disconnected.");
    });

    Log.trace("BLE starts advertising...");
    bleConfigured = true;
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

constexpr uint32_t SLEEP_DURATION_S = 3;

#define MAX_CELLULAR_OFF_TIME (3*60*1000)

} // anonymous

void updateTime() {
    exitTime = Time.now();
}

STARTUP(updateTime());

test(000_System_Sleep_Peripheral_Cloud_Connect) {
    subscribeEvents(BLE_ROLE_PERIPHERAL);
    Particle.connect();
    assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));
    assertTrue(publishBlePeerInfo());
}

test(000_System_Sleep_Prepare) {
    hal_gpio_config_t conf = {
        .size = sizeof(conf),
        .version = HAL_GPIO_VERSION,
        .mode = OUTPUT,
        .set_value = true,
        .value = 1,
        .drive_strength = HAL_GPIO_DRIVE_DEFAULT
    };
    hal_gpio_configure(SLEEP_PIN_RESET, &conf, nullptr); // default HIGH, active LOW
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
    assertEqual(BLE.setDesiredAttMtu(LOCAL_DESIRED_ATT_MTU), (int)SYSTEM_ERROR_NONE);
}

test(01_System_Sleep_With_Configuration_Object_Hibernate_Mode_Without_Wakeup_1) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::HIBERNATE);
    SystemSleepResult result = System.sleep(config);
    assertEqual(result.error(), SYSTEM_ERROR_NONE);
}
test(01_System_Sleep_With_Configuration_Object_Hibernate_Mode_Without_Wakeup_2) {
    assertTrue(System.resetReason() == RESET_REASON_PIN_RESET || System.resetReason() == RESET_REASON_POWER_DOWN);
}

test(02_System_Sleep_Mode_Deep_Without_Wakeup_1) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));

    SystemSleepResult result = System.sleep(SLEEP_MODE_DEEP, SLEEP_DISABLE_WKP_PIN);
    assertEqual(result.error(), SYSTEM_ERROR_NONE);
}
test(02_System_Sleep_Mode_Deep_Without_Wakeup_2) {
    assertTrue(System.resetReason() == RESET_REASON_PIN_RESET || System.resetReason() == RESET_REASON_POWER_DOWN);
}

#if !HAL_PLATFORM_RTL872X
test(03_System_Sleep_With_Configuration_Object_Hibernate_Mode_Wakeup_By_D0_1) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::HIBERNATE)
          .gpio(SLEEP_PIN_GPIO_WAKE_UP, RISING);
    SystemSleepResult result = System.sleep(config);
    assertEqual(result.error(), SYSTEM_ERROR_NONE);
}
test(03_System_Sleep_With_Configuration_Object_Hibernate_Mode_Wakeup_By_D0_2) {
    assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);
}

// TODO: Move to wiring/sleep as this is a Sleep API 1.0 test
test(04_System_Sleep_Mode_Deep_Wakeup_By_WKP_Pin_1) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));

    SleepResult result = {};

// Tracker supports waking up device from hibernate mode by external RTC
#if !HAL_PLATFORM_EXTERNAL_RTC
    result = System.sleep(SLEEP_MODE_DEEP, 3s);
    assertNotEqual(result.error(), SYSTEM_ERROR_NONE); // Gen3 doesn't support RTC wakeup source.
#endif

    result = System.sleep(SLEEP_MODE_DEEP);
    assertEqual(result.error(), SYSTEM_ERROR_NONE);
}
test(04_System_Sleep_Mode_Deep_Wakeup_By_WKP_Pin_2) {
    assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);
}

test(05_System_Sleep_With_Configuration_Object_Hibernate_Mode_Wakeup_By_Analog_Pin_1) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::HIBERNATE)
          .analog(SLEEP_PIN_ADC_WAKE_UP, 1500, AnalogInterruptMode::CROSS);
    SystemSleepResult result = System.sleep(config);
    assertEqual(result.error(), SYSTEM_ERROR_NONE);
}
test(05_System_Sleep_With_Configuration_Object_Hibernate_Mode_Wakeup_By_Analog_Pin_2) {
    assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);
}

// Tracker support waking up device from hibernate mode by external RTC
#if HAL_PLATFORM_EXTERNAL_RTC
test(06_System_Sleep_With_Configuration_Object_Hibernate_Mode_Wakeup_By_External_Rtc) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::HIBERNATE)
          .duration(3s);
    SystemSleepResult result = System.sleep(config);

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);
}

test(07_System_Sleep_Mode_Deep_Wakeup_By_External_Rtc) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));

    SystemSleepResult result = System.sleep(SLEEP_MODE_DEEP, 3s);

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);
}
#endif // HAL_PLATFORM_EXTERNAL_RTC
#endif // !HAL_PLATFORM_RTL872X

#if HAL_PLATFORM_RTL872X
test(08_System_Sleep_With_Configuration_Object_Hibernate_Mode_Wakeup_By_Wkp_Pin_1) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::HIBERNATE)
          .gpio(SLEEP_PIN_WKP_WAKE_UP, RISING);
    SystemSleepResult result = System.sleep(config);
    assertEqual(result.error(), SYSTEM_ERROR_NONE);
}
test(08_System_Sleep_With_Configuration_Object_Hibernate_Mode_Wakeup_By_Wkp_Pin_2) {
    assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);
}

test(09_System_Sleep_With_Configuration_Object_Hibernate_Mode_Wakeup_By_Rtc) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::HIBERNATE)
          .duration(3s);
    SystemSleepResult result = System.sleep(config);

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);
}

test(10_System_Sleep_Mode_Deep_Wakeup_By_Wkp_Pin_1) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));

    SystemSleepResult result = System.sleep(SLEEP_MODE_DEEP);
    assertEqual(result.error(), SYSTEM_ERROR_NONE);
}
test(10_System_Sleep_Mode_Deep_Wakeup_By_Wkp_Pin_2) {
    assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);
}

test(11_System_Sleep_Mode_Deep_Wakeup_By_Rtc) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));

    SystemSleepResult result = System.sleep(SLEEP_MODE_DEEP, 3s, SLEEP_DISABLE_WKP_PIN); // Disable WKP pin.

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);
}

test(12_System_Sleep_With_Configuration_Object_Hibernate_Mode_Bypass_Network_Off_Execution_Time_1) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));

    Network.on();
    Network.connect(); // to finally power on the modem. The Network.on() won't do that for us on Gen3 as for now.
    assertTrue(waitFor(Network.ready, CLOUD_CONNECT_TIMEOUT));

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::HIBERNATE)
          .duration(SLEEP_DURATION_S * 1000);
    // P2 doesn't support using network as wakeup source
    // config.network(WiFi, SystemSleepNetworkFlag::INACTIVE_STANDBY);

    enterTime = Time.now();
    SystemSleepResult result = System.sleep(config);
    assertEqual(result.error(), SYSTEM_ERROR_NONE);
}
test(12_System_Sleep_With_Configuration_Object_Hibernate_Mode_Bypass_Network_Off_Execution_Time_2) {
    assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);
    assertLessOrEqual(exitTime - enterTime, SLEEP_DURATION_S + 5);
}
#endif // HAL_PLATFORM_RTL872X

test(13_System_Sleep_With_Configuration_Object_Stop_Mode_Without_Wakeup) {
    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP);
#if HAL_PLATFORM_CELLULAR
    config.network(Cellular, SystemSleepNetworkFlag::INACTIVE_STANDBY);
#endif
#if HAL_PLATFORM_WIFI
    config.network(WiFi, SystemSleepNetworkFlag::INACTIVE_STANDBY);
#endif
    SystemSleepResult result = System.sleep(config);
    assertNotEqual(result.error(), SYSTEM_ERROR_NONE);
}

// TODO: Move to wiring/sleep as this is a Sleep API 1.0 test
test(14_System_Sleep_With_Configuration_Object_Stop_Mode_Without_Wakeup) {
    SleepResult result = System.sleep(nullptr, 0, nullptr, 0, 0);
    assertNotEqual(result.error(), SYSTEM_ERROR_NONE);
}

test(15_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Without_Wakeup) {
    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::ULTRA_LOW_POWER);
#if HAL_PLATFORM_CELLULAR
    config.network(Cellular, SystemSleepNetworkFlag::INACTIVE_STANDBY);
#endif
#if HAL_PLATFORM_WIFI
    config.network(WiFi, SystemSleepNetworkFlag::INACTIVE_STANDBY);
#endif
    SystemSleepResult result = System.sleep(config);
    assertNotEqual(result.error(), SYSTEM_ERROR_NONE);
}

SystemSleepResult result16;
test(16_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_D0_1) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP)
          .gpio(SLEEP_PIN_GPIO_WAKE_UP, RISING);
    result16 = System.sleep(config);

    delay(1s); // FIXME: P2 specific due to USB thread
}
test(16_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_D0_2) {
    assertEqual(result16.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result16.wakeupReason(), (int)SystemSleepWakeupReason::BY_GPIO);
    assertEqual(result16.wakeupPin(), D0);
}

SystemSleepResult result17;
test(17_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_Rtc) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP)
          .duration(3s);
    result17 = System.sleep(config);

    delay(1s); // FIXME: P2 specific due to USB thread

    assertEqual(result17.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result17.wakeupReason(), (int)SystemSleepWakeupReason::BY_RTC);
}

#if HAL_PLATFORM_BLE && !HAL_PLATFORM_RTL872X
SystemSleepResult result18;
test(18_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_Ble_1) {

    if (!bleConfigured) {
        bleConfigureAndStartAdvertising();
    } else {
        BLE.advertise();
    }

    assertTrue(BLE.advertising());

    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));
    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP)
          .ble();
    result18 = System.sleep(config);
}
test(18_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_Ble_2) {
    assertEqual(result18.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result18.wakeupReason(), (int)SystemSleepWakeupReason::BY_BLE);
    assertTrue(BLE.connected());
    BLE.stopAdvertising();
}
#endif // HAL_PLATFORM_BLE && !HAL_PLATFORM_RTL872X

// TODO: Move to wiring/sleep as this is a Sleep API 1.0 test
SleepResult result19;
test(19_System_Sleep_Mode_Stop_Wakeup_By_D0_1) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));

    result19 = System.sleep(D0, RISING);

    delay(1s); // FIXME: P2 specific due to USB thread
}
test(19_System_Sleep_Mode_Stop_Wakeup_By_D0_2) {
    assertEqual(result19.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result19.reason(), (int)WAKEUP_REASON_PIN);
    assertEqual(result19.pin(), D0);
}

// TODO: Move to wiring/sleep as this is a Sleep API 1.0 test
SleepResult result20;
test(20_System_Sleep_Mode_Stop_Wakeup_By_Rtc) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));

    result20 = System.sleep(nullptr, 0, nullptr, 0, 3s);

    delay(1s); // FIXME: P2 specific due to USB thread

    assertEqual(result20.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result20.reason(), (int)WAKEUP_REASON_RTC);
}

SystemSleepResult result21;
test(21_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_D0_1) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::ULTRA_LOW_POWER)
          .gpio(SLEEP_PIN_GPIO_WAKE_UP, RISING);
    result21 = System.sleep(config);

    delay(1s); // FIXME: P2 specific due to USB thread
}
test(21_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_D0_2) {
    assertEqual(result21.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result21.wakeupReason(), (int)SystemSleepWakeupReason::BY_GPIO);
    assertEqual(result21.wakeupPin(), D0);
}

SystemSleepResult result22;
test(22_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_Rtc) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::ULTRA_LOW_POWER)
          .duration(3s);
    result22 = System.sleep(config);

    delay(1s); // FIXME: P2 specific due to USB thread

    assertEqual(result22.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result22.wakeupReason(), (int)SystemSleepWakeupReason::BY_RTC);
}

#if !HAL_PLATFORM_RTL872X

#if HAL_PLATFORM_BLE
SystemSleepResult result23;
test(23_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_Ble_1) {

    if (!bleConfigured) {
        bleConfigureAndStartAdvertising();
    } else {
        BLE.advertise();
    }

    assertTrue(BLE.advertising());

    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));
    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::ULTRA_LOW_POWER)
          .ble();
    result23 = System.sleep(config);
}
test(23_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_Ble_2) {
    assertEqual(result23.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result23.wakeupReason(), (int)SystemSleepWakeupReason::BY_BLE);
    assertTrue(BLE.connected());
    BLE.stopAdvertising();
}
#endif // HAL_PLATFORM_BLE

SystemSleepResult result24;
test(24_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_Analog_Pin_1) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP)
          .analog(SLEEP_PIN_ADC_WAKE_UP, 1500, AnalogInterruptMode::CROSS);
    result24 = System.sleep(config);
}
test(24_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_Analog_Pin_2) {
    assertEqual(result24.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result24.wakeupReason(), (int)SystemSleepWakeupReason::BY_LPCOMP);
}

SystemSleepResult result25;
test(25_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_Analog_Pin_1) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::ULTRA_LOW_POWER)
          .analog(SLEEP_PIN_ADC_WAKE_UP, 1500, AnalogInterruptMode::CROSS);
    result25 = System.sleep(config);
}
test(25_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_Analog_Pin_2) {
    assertEqual(result25.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result25.wakeupReason(), (int)SystemSleepWakeupReason::BY_LPCOMP);
}

SystemSleepResult result26;
test(26_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_Usart_1) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));

    Serial1.begin(115200);

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP)
          .usart(Serial1);
    result26 = System.sleep(config);
}
test(26_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_Usart_2) {
    assertEqual(result26.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result26.wakeupReason(), (int)SystemSleepWakeupReason::BY_USART);
    Serial1.end();
}

SystemSleepResult result27;
test(27_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_Usart_1) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));

    Serial1.begin(115200);

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::ULTRA_LOW_POWER)
          .usart(Serial1);
    result27 = System.sleep(config);
}
test(27_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_Usart_2) {
    assertEqual(result27.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result27.wakeupReason(), (int)SystemSleepWakeupReason::BY_USART);
    Serial1.end();
}

#if HAL_PLATFORM_CELLULAR
SystemSleepResult result28;
test(28_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_Cellular_1) {

    if (Particle.connected()) {
        Particle.disconnect();
        assertTrue(waitFor(Particle.disconnected, CLOUD_DISCONNECT_TIMEOUT));
    }
    subscribePeerEvents(BLE_ROLE_PERIPHERAL);
    Particle.connect();
    assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));
    assertTrue(publishBlePeerInfo(28));  // publish test 28 is ready

    // Give us more than enough time to process all acknowledges from the Cloud before sleeping
    auto s = millis();
    while (millis() - s < 2000) {
        Particle.process();
        delay(100);
    }

    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 20000));
    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP)
          .network(Cellular);
    result28 = System.sleep(config);
    delay(5000);
}
test(28_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_Cellular_2) {
    assertEqual(result28.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result28.wakeupReason(), (int)SystemSleepWakeupReason::BY_NETWORK);
    assertEqual(getBleTestPeer().test, 28);
    resetBleTestPeerTest();
}

SystemSleepResult result29;
test(29_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_Cellular_1) {

    if (Particle.connected()) {
        Particle.disconnect();
        assertTrue(waitFor(Particle.disconnected, CLOUD_DISCONNECT_TIMEOUT));
    }
    subscribePeerEvents(BLE_ROLE_PERIPHERAL);
    Particle.connect();
    assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));
    assertTrue(publishBlePeerInfo(29));  // publish test 28 is ready

    // Give us more than enough time to process all acknowledges from the Cloud before sleeping
    auto s = millis();
    while (millis() - s < 2000) {
        Particle.process();
        delay(100);
    }

    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 20000));
    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::ULTRA_LOW_POWER)
          .network(Cellular);
    result29 = System.sleep(config);
    delay(5000);
}
test(29_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_Cellular_2) {
    assertEqual(result29.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result29.wakeupReason(), (int)SystemSleepWakeupReason::BY_NETWORK);
    assertEqual(getBleTestPeer().test, 29);
    resetBleTestPeerTest();
}
#endif // HAL_PLATFORM_CELLULAR

#if HAL_PLATFORM_WIFI
SystemSleepResult result30;
test(30_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_WiFi_1) {

    if (Particle.connected()) {
        Particle.disconnect();
        assertTrue(waitFor(Particle.disconnected, CLOUD_DISCONNECT_TIMEOUT));
    }
    subscribePeerEvents(BLE_ROLE_PERIPHERAL);
    Particle.connect();
    assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));
    delay(2000);
    assertTrue(publishBlePeerInfo(30));  // publish test 30 is ready

    // Give us more than enough time to process all acknowledges from the Cloud before sleeping
    auto s = millis();
    while (millis() - s < 2000) {
        Particle.process();
        delay(100);
    }

    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 20000));
    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP)
          .network(WiFi);
    result30 = System.sleep(config);
    delay(5000);
}
test(30_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_WiFi_2) {
    assertEqual(result30.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result30.wakeupReason(), (int)SystemSleepWakeupReason::BY_NETWORK);
    assertEqual(getBleTestPeer().test, 30);
    resetBleTestPeerTest();
}

SystemSleepResult result31;
test(31_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_WiFi_1) {

    if (Particle.connected()) {
        Particle.disconnect();
        assertTrue(waitFor(Particle.disconnected, CLOUD_DISCONNECT_TIMEOUT));
    }
    subscribePeerEvents(BLE_ROLE_PERIPHERAL);
    Particle.connect();
    assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));
    delay(2000);
    assertTrue(publishBlePeerInfo(31));  // publish test 31 is ready

    // Give us more than enough time to process all acknowledges from the Cloud before sleeping
    auto s = millis();
    while (millis() - s < 2000) {
        Particle.process();
        delay(100);
    }

    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 20000));
    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::ULTRA_LOW_POWER)
          .network(WiFi);
    result31 = System.sleep(config);
    delay(5000);
}
test(31_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_WiFi_2) {
    assertEqual(result31.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result31.wakeupReason(), (int)SystemSleepWakeupReason::BY_NETWORK);
    assertEqual(getBleTestPeer().test, 31);
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

    System.on(network_status, [](system_event_t ev, int data) -> void {
        if (ev == network_status && data == network_status_off && sNetworkOffTimestamp == 0) {
            sNetworkOffTimestamp = Time.now();
            Log.trace("sNetworkOffTimestamp: %ld", sNetworkOffTimestamp);
        }
    });
}

test(33_System_Sleep_With_Configuration_Object_Stop_Mode_Execution_Time) {

    constexpr uint32_t SLEEP_DURATION_S = 3;
    Network.on();
    Particle.connect();
    assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP)
          .duration(SLEEP_DURATION_S * 1000);

    if (system_thread_get_state(nullptr) == spark::feature::ENABLED) {
        sNetworkOffTimestamp = Time.now();
    } else {
        sNetworkOffTimestamp = 0;
    }
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));
    SystemSleepResult result = System.sleep(config);
    time32_t exit = Time.now();

    delay(1s); // FIXME: P2 specific due to USB thread

    assertNotEqual(sNetworkOffTimestamp, 0);
    assertMoreOrEqual(exit - sNetworkOffTimestamp, SLEEP_DURATION_S);
    if (system_thread_get_state(nullptr) == spark::feature::ENABLED) {
        assertLessOrEqual(exit - sNetworkOffTimestamp, SLEEP_DURATION_S + 10 );
    } else {
        assertLessOrEqual(exit - sNetworkOffTimestamp, SLEEP_DURATION_S + 2 );
    }

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result.wakeupReason(), (int)SystemSleepWakeupReason::BY_RTC);

    // Make sure we reconnect back to the cloud
    assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));
}

test(34_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_Execution_Time) {

    constexpr uint32_t SLEEP_DURATION_S = 3;
    Network.on();
    Particle.connect();
    assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::ULTRA_LOW_POWER)
          .duration(SLEEP_DURATION_S * 1000);

    if (system_thread_get_state(nullptr) == spark::feature::ENABLED) {
        sNetworkOffTimestamp = Time.now();
    } else {
        sNetworkOffTimestamp = 0;
    }
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));
    SystemSleepResult result = System.sleep(config);
    time32_t exit = Time.now();

    delay(1s); // FIXME: P2 specific due to USB thread

    assertNotEqual(sNetworkOffTimestamp, 0);
    assertMoreOrEqual(exit - sNetworkOffTimestamp, SLEEP_DURATION_S);
    if (system_thread_get_state(nullptr) == spark::feature::ENABLED) {
        assertLessOrEqual(exit - sNetworkOffTimestamp, SLEEP_DURATION_S + 10 );
    } else {
        assertLessOrEqual(exit - sNetworkOffTimestamp, SLEEP_DURATION_S + 2 );
    }

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result.wakeupReason(), (int)SystemSleepWakeupReason::BY_RTC);
}

test(35_System_Sleep_With_Configuration_Object_Network_Power_State_Consistent_On) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));

    Particle.disconnect();
    assertTrue(waitFor(Particle.disconnected, CLOUD_CONNECT_TIMEOUT));

    {
        // Make sure the modem is off first
        Log.trace("    >> Powering off the modem...");
#if HAL_PLATFORM_CELLULAR
        Cellular.off();
        assertTrue(waitFor(Cellular.isOff, MAX_CELLULAR_OFF_TIME));
        Log.trace("    >> Powering on the modem...");
        Cellular.on();
        assertTrue(waitFor(Cellular.isOn, 60000));
#elif HAL_PLATFORM_WIFI
        WiFi.off();
        assertTrue(waitFor(WiFi.isOff, 60000));
        Log.trace("    >> Powering on the modem...");
        WiFi.on();
        assertTrue(waitFor(WiFi.isOn, 60000));
#endif

        Log.trace("    >> Entering sleep... Please reconnect serial after 3 seconds");
        SystemSleepResult result = System.sleep(SystemSleepConfiguration().mode(SystemSleepMode::STOP).duration(3s));

        delay(1s); // FIXME: P2 specific due to USB thread

        assertEqual(result.error(), SYSTEM_ERROR_NONE);
        assertEqual((int)result.wakeupReason(), (int)SystemSleepWakeupReason::BY_RTC);

        Log.trace("    >> Waiting for the modem to be turned on...");
#if HAL_PLATFORM_CELLULAR
        assertTrue(waitFor(Cellular.isOn, 60000));
#elif HAL_PLATFORM_WIFI
        assertTrue(waitFor(WiFi.isOn, 60000));
#endif
    }
}

test(36_System_Sleep_With_Configuration_Object_Network_Power_State_Consistent_Off) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));

    {
        // Make sure the modem is on first
        Log.trace("    >> Powering on the modem...");
#if HAL_PLATFORM_CELLULAR
        Cellular.on();
        assertTrue(waitFor(Cellular.isOn, 60000));
        Log.trace("    >> Powering off the modem...");
        Cellular.off();
        assertTrue(waitFor(Cellular.isOff, MAX_CELLULAR_OFF_TIME));
#elif HAL_PLATFORM_WIFI
        WiFi.on();
        assertTrue(waitFor(WiFi.isOn, 60000));
        Log.trace("    >> Powering off the modem...");
        WiFi.off();
        assertTrue(waitFor(WiFi.isOff, 60000));
#endif

        Log.trace("    >> Entering sleep... Please reconnect serial after 3 seconds");
        SystemSleepResult result = System.sleep(SystemSleepConfiguration().mode(SystemSleepMode::STOP).duration(3s));

        delay(1s); // FIXME: P2 specific due to USB thread

        assertEqual(result.error(), SYSTEM_ERROR_NONE);
        assertEqual((int)result.wakeupReason(), (int)SystemSleepWakeupReason::BY_RTC);

        // Give system thread a chance to run if threading is enabled.
        delay(3s);
#if HAL_PLATFORM_CELLULAR
        assertTrue(Cellular.isOff());
#elif HAL_PLATFORM_WIFI
        assertTrue(WiFi.isOff());
#endif
    }
}
