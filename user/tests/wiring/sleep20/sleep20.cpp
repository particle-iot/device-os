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

//Serial1LogHandler logHandler(115200, LOG_LEVEL_INFO);

//TODO: Run user/tests/app/tracker_wakeup to verify the additional wakeup sources for platform tracker.

namespace {

STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY));
static retained uint32_t magick = 0;
static retained uint32_t phase = 0;

static retained time32_t enterTime = 0;
static          time32_t exitTime = 0;

constexpr system_tick_t CLOUD_CONNECT_TIMEOUT = 10 * 60 * 1000;

time32_t sNetworkOffTimestamp = 0;

} // anonymous

void updateTime() {
    exitTime = Time.now();
}

STARTUP(updateTime());

test(01_System_Sleep_With_Configuration_Object_Hibernate_Mode_Without_Wakeup) {
    if (magick != 0xdeadbeef) {
        magick = 0xdeadbeef;
        phase = 0xbeef0001;
    }
    if (phase == 0xbeef0001) {
        Serial.println("    >> Device enters hibernate mode.");
        Serial.println("    >> Please reconnect serial and type 't' after you press the reset button.");
        Serial.println("    >> Press any key now");
        while (Serial.available() <= 0);
        while (Serial.available() > 0) {
            (void)Serial.read();
        }

        phase = 0xbeef0002;

        SystemSleepConfiguration config;
        config.mode(SystemSleepMode::HIBERNATE);
        SystemSleepResult result = System.sleep(config);
        assertEqual(result.error(), SYSTEM_ERROR_NONE);
    } else if (phase == 0xbeef0002) {
        Serial.println("    >> Device is reset from hibernate mode.");
        assertTrue(System.resetReason() == RESET_REASON_PIN_RESET || System.resetReason() == RESET_REASON_POWER_DOWN);
    }
}

test(02_System_Sleep_Mode_Deep_Without_Wakeup) {
    if (phase == 0xbeef0002) {
        Serial.println("    >> Device enters hibernate mode.");
        Serial.println("    >> Please reconnect serial and type 't' after you press the reset button.");
        Serial.println("    >> Press any key now");
        while (Serial.available() <= 0);
        while (Serial.available() > 0) {
            (void)Serial.read();
        }

        phase = 0xbeef0003;

        SleepResult result = System.sleep(SLEEP_MODE_DEEP, SLEEP_DISABLE_WKP_PIN);
        assertEqual(result.error(), SYSTEM_ERROR_NONE);
    } else if (phase == 0xbeef0003) {
        Serial.println("    >> Device is reset from hibernate mode.");
        assertTrue(System.resetReason() == RESET_REASON_PIN_RESET || System.resetReason() == RESET_REASON_POWER_DOWN);
    }
}

#if !HAL_PLATFORM_RTL872X
test(03_System_Sleep_With_Configuration_Object_Hibernate_Mode_Wakeup_By_D0) {
    if (phase == 0xbeef0003) {
        Serial.println("    >> Device enters hibernate mode.");
        Serial.println("    >> Please reconnect serial and type 't' after you have a rising edge on D0.");
        Serial.println("    >> Press any key now");
        while (Serial.available() <= 0);
        while (Serial.available() > 0) {
            (void)Serial.read();
        }

        phase = 0xbeef0004;

        SystemSleepConfiguration config;
        config.mode(SystemSleepMode::HIBERNATE)
              .gpio(D0, RISING);
        SystemSleepResult result = System.sleep(config);
        assertEqual(result.error(), SYSTEM_ERROR_NONE);
    } else if (phase == 0xbeef0004) {
        Serial.println("    >> Device is woken up from hibernate mode.");
        assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);
    }
}

test(04_System_Sleep_Mode_Deep_Wakeup_By_WKP_Pin) {
    if (phase == 0xbeef0004) {
        Serial.println("    >> Device enters hibernate mode.");
        Serial.println("    >> Please reconnect serial and type 't' after you have a rising edge on WKP pin.");
        Serial.println("    >> Press any key now");
        while (Serial.available() <= 0);
        while (Serial.available() > 0) {
            (void)Serial.read();
        }

        phase = 0xbeef0005;

        SleepResult result = {};

// Tracker supports waking up device from hibernate mode by external RTC
#if !HAL_PLATFORM_EXTERNAL_RTC
        result = System.sleep(SLEEP_MODE_DEEP, 3s);
        assertNotEqual(result.error(), SYSTEM_ERROR_NONE); // Gen3 doesn't support RTC wakeup source.
#endif

        result = System.sleep(SLEEP_MODE_DEEP);
        assertEqual(result.error(), SYSTEM_ERROR_NONE);
    } else if (phase == 0xbeef0005) {
        Serial.println("    >> Device is woken up from hibernate mode.");
        assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);
    }
}

test(05_System_Sleep_With_Configuration_Object_Hibernate_Mode_Wakeup_By_Analog_Pin) {
    if (phase == 0xbeef0005) {
        Serial.println("    >> Device enters hibernate mode.");
        Serial.println("    >> Please reconnect serial and type 't' after applying voltage crossing 1500mV on A0.");
        Serial.println("    >> Press any key now");
        while (Serial.available() <= 0);
        while (Serial.available() > 0) {
            (void)Serial.read();
        }

        phase = 0xbeef0006;

        SystemSleepConfiguration config;
        config.mode(SystemSleepMode::HIBERNATE)
              .analog(A0, 1500, AnalogInterruptMode::CROSS);
        SystemSleepResult result = System.sleep(config);
        assertEqual(result.error(), SYSTEM_ERROR_NONE);
    } else if (phase == 0xbeef0006) {
        Serial.println("    >> Device is woken up from hibernate mode.");
        assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);
    }
}

// Tracker support waking up device from hibernate mode by external RTC
#if HAL_PLATFORM_EXTERNAL_RTC
test(06_System_Sleep_With_Configuration_Object_Hibernate_Mode_Wakeup_By_External_Rtc) {
    if (phase == 0xbeef0006) {
        Serial.println("    >> Device enters hibernate mode.");
        Serial.println("    >> Please reconnect serial and type 't' after 3 seconds.");
        Serial.println("    >> Press any key now");
        while (Serial.available() <= 0);
        while (Serial.available() > 0) {
            (void)Serial.read();
        }

        phase = 0xbeef0007;

        SystemSleepConfiguration config;
        config.mode(SystemSleepMode::HIBERNATE)
              .duration(3s);
        SystemSleepResult result = System.sleep(config);
        assertEqual(result.error(), SYSTEM_ERROR_NONE);
    } else if (phase == 0xbeef0007) {
        Serial.println("    >> Device is woken up from hibernate mode.");
        assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);
    }
}

test(07_System_Sleep_Mode_Deep_Wakeup_By_External_Rtc) {
    if (phase == 0xbeef0007) {
        Serial.println("    >> Device enters hibernate mode.");
        Serial.println("    >> Please reconnect serial and type 't' after 3 seconds.");
        Serial.println("    >> Press any key now");
        while (Serial.available() <= 0);
        while (Serial.available() > 0) {
            (void)Serial.read();
        }

        phase = 0xbeef0008;

        SleepResult result = System.sleep(SLEEP_MODE_DEEP, 3s);
        assertEqual(result.error(), SYSTEM_ERROR_NONE);
    } else if (phase == 0xbeef0008) {
        Serial.println("    >> Device is woken up from hibernate mode.");
        assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);
    }
}
#endif // HAL_PLATFORM_EXTERNAL_RTC
#endif // !HAL_PLATFORM_RTL872X

#if HAL_PLATFORM_RTL872X
test(08_System_Sleep_With_Configuration_Object_Hibernate_Mode_Wakeup_By_Wkp_Pin) {
    if (phase == 0xbeef0003) {
        Serial.println("    >> Device enters hibernate mode.");
        Serial.println("    >> Please reconnect serial and type 't' after you have a rising edge on WKP pin.");
        Serial.println("    >> Press any key now");
        while (Serial.available() <= 0);
        while (Serial.available() > 0) {
            (void)Serial.read();
        }

        phase = 0xbeef0004;

        SystemSleepConfiguration config;
        config.mode(SystemSleepMode::HIBERNATE)
              .gpio(WKP, RISING);
        SystemSleepResult result = System.sleep(config);
        assertEqual(result.error(), SYSTEM_ERROR_NONE);
    } else if (phase == 0xbeef0004) {
        Serial.println("    >> Device is woken up from hibernate mode.");
        assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);
    }
}

test(09_System_Sleep_With_Configuration_Object_Hibernate_Mode_Wakeup_By_Rtc) {
    if (phase == 0xbeef0004) {
        Serial.println("    >> Device enters hibernate mode.");
        Serial.println("    >> Please reconnect serial and type 't' after 3 seconds.");
        Serial.println("    >> Press any key now");
        while (Serial.available() <= 0);
        while (Serial.available() > 0) {
            (void)Serial.read();
        }

        phase = 0xbeef0005;

        SystemSleepConfiguration config;
        config.mode(SystemSleepMode::HIBERNATE)
              .duration(3s);
        SystemSleepResult result = System.sleep(config);
        assertEqual(result.error(), SYSTEM_ERROR_NONE);
    } else if (phase == 0xbeef0005) {
        Serial.println("    >> Device is woken up from hibernate mode.");
        assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);
    }
}

test(10_System_Sleep_Mode_Deep_Wakeup_By_Wkp_Pin) {
    if (phase == 0xbeef0005) {
        Serial.println("    >> Device enters hibernate mode.");
        Serial.println("    >> Please reconnect serial and type 't' after you have a rising edge on WKP pin.");
        Serial.println("    >> Press any key now");
        while (Serial.available() <= 0);
        while (Serial.available() > 0) {
            (void)Serial.read();
        }

        phase = 0xbeef0006;

        SleepResult result = System.sleep(SLEEP_MODE_DEEP);
        assertEqual(result.error(), SYSTEM_ERROR_NONE);
    } else if (phase == 0xbeef0006) {
        Serial.println("    >> Device is woken up from hibernate mode.");
        assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);
    }
}

test(11_System_Sleep_Mode_Deep_Wakeup_By_Rtc) {
    if (phase == 0xbeef0006) {
        Serial.println("    >> Device enters hibernate mode.");
        Serial.println("    >> Please reconnect serial and type 't' after 3 seconds.");
        Serial.println("    >> Press any key now");
        while (Serial.available() <= 0);
        while (Serial.available() > 0) {
            (void)Serial.read();
        }

        phase = 0xbeef0007;

        SleepResult result = System.sleep(SLEEP_MODE_DEEP, 3s, SLEEP_DISABLE_WKP_PIN); // Disable WKP pin.
        assertEqual(result.error(), SYSTEM_ERROR_NONE);
    } else if (phase == 0xbeef0007) {
        Serial.println("    >> Device is woken up from hibernate mode.");
        assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);
    }
}

test(12_System_Sleep_With_Configuration_Object_Hibernate_Mode_Bypass_Network_Off_Execution_Time) {
    constexpr uint32_t SLEEP_DURATION_S = 3;
    if (phase == 0xbeef0007) {
        Serial.printf("    >> Device enters hibernate mode. Please reconnect serial and type 't' after %ld s\r\n", SLEEP_DURATION_S);
        Serial.println("    >> Press any key now");
        while (Serial.available() <= 0);
        while (Serial.available() > 0) {
            (void)Serial.read();
        }

        phase = 0xbeef0008;

        Serial.print("Turning on the modem...");
        Network.on();
        Network.connect(); // to finally power on the modem. The Network.on() won't do that for us on Gen3 as for now.
        Serial.println("Done.");
        assertTrue(waitFor(Network.ready, CLOUD_CONNECT_TIMEOUT));

        SystemSleepConfiguration config;
        config.mode(SystemSleepMode::HIBERNATE)
              .duration(SLEEP_DURATION_S * 1000);
        // P2 doesn't support using network as wakeup source
        // config.network(WiFi, SystemSleepNetworkFlag::INACTIVE_STANDBY);

        enterTime = Time.now();
        Serial.printlnf("Before: %ld", enterTime);
        SystemSleepResult result = System.sleep(config);
        assertEqual(result.error(), SYSTEM_ERROR_NONE);
    } else if (phase == 0xbeef0008) {
        Serial.println("    >> Device is reset from hibernate mode.");
        Serial.printlnf("After: %ld", exitTime);
        assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);
        assertLessOrEqual(exitTime - enterTime, SLEEP_DURATION_S + 3);
    }
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
    assertNotEqual(result.error(), SYSTEM_ERROR_NONE)   ;
}

test(16_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_D0) {
    Serial.println("    >> Device enters stop mode. Please reconnect serial after you have a rising edge on D0.");
    Serial.println("    >> Press any key now");
    while(Serial.available() <= 0);
    while (Serial.available() > 0) {
        (void)Serial.read();
    }

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP)
          .gpio(D0, RISING);
    SystemSleepResult result = System.sleep(config);

    delay(1s); // FIXME: P2 specific due to USB thread
    waitUntil(Serial.isConnected);

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result.wakeupReason(), (int)SystemSleepWakeupReason::BY_GPIO);
    assertEqual(result.wakeupPin(), D0);
}

test(17_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_Rtc) {
    Serial.println("    >> Device enters stop mode. Please reconnect serial after 3 seconds.");
    Serial.println("    >> Press any key now");
    while(Serial.available() <= 0);
    while (Serial.available() > 0) {
        (void)Serial.read();
    }

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP)
          .duration(3s);
    SystemSleepResult result = System.sleep(config);

    delay(1s); // FIXME: P2 specific due to USB thread
    waitUntil(Serial.isConnected);

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result.wakeupReason(), (int)SystemSleepWakeupReason::BY_RTC);
}

#if HAL_PLATFORM_BLE && !HAL_PLATFORM_RTL872X
test(18_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_Ble) {
    Serial.println("    >> Device enters stop mode. Please reconnect serial after device being connected by BLE Central.");
    Serial.println("    >> Press any key now");
    BLE.on();
    BLE.advertise();
    while(Serial.available() <= 0);
    while (Serial.available() > 0) {
        (void)Serial.read();
    }

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP)
          .ble();
    SystemSleepResult result = System.sleep(config);

    waitUntil(Serial.isConnected);

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result.wakeupReason(), (int)SystemSleepWakeupReason::BY_BLE);

    BLE.stopAdvertising();
}
#endif // HAL_PLATFORM_BLE && !HAL_PLATFORM_RTL872X

test(19_System_Sleep_Mode_Stop_Wakeup_By_D0) {
    Serial.println("    >> Device enters stop mode. Please reconnect serial after you have a rising edge on D0.");
    Serial.println("    >> Press any key now");
    while(Serial.available() <= 0);
    while (Serial.available() > 0) {
        (void)Serial.read();
    }

    SleepResult result = System.sleep(D0, RISING);

    delay(1s); // FIXME: P2 specific due to USB thread
    waitUntil(Serial.isConnected);

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result.reason(), (int)WAKEUP_REASON_PIN);
    assertEqual(result.pin(), D0);
}

test(20_System_Sleep_Mode_Stop_Wakeup_By_Rtc) {
    Serial.println("    >> Device enters stop mode. Please reconnect serial after 3 seconds");
    Serial.println("    >> Press any key now");
    while(Serial.available() <= 0);
    while (Serial.available() > 0) {
        (void)Serial.read();
    }

    SleepResult result = System.sleep(nullptr, 0, nullptr, 0, 3s);

    delay(1s); // FIXME: P2 specific due to USB thread
    waitUntil(Serial.isConnected);

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result.reason(), (int)WAKEUP_REASON_RTC);
}

test(21_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_D0) {
    Serial.println("    >> Device enters ultra-low power mode. Please reconnect serial after you have a rising edge on D0.");
    Serial.println("    >> Press any key now");
    while(Serial.available() <= 0);
    while (Serial.available() > 0) {
        (void)Serial.read();
    }

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::ULTRA_LOW_POWER)
          .gpio(D0, RISING);
    SystemSleepResult result = System.sleep(config);

    delay(1s); // FIXME: P2 specific due to USB thread
    waitUntil(Serial.isConnected);

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result.wakeupReason(), (int)SystemSleepWakeupReason::BY_GPIO);
    assertEqual(result.wakeupPin(), D0);
}

test(22_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_Rtc) {
    Serial.println("    >> Device enters ultra-low power mode. Please reconnect serial after 3 seconds.");
    Serial.println("    >> Press any key now");
    while(Serial.available() <= 0);
    while (Serial.available() > 0) {
        (void)Serial.read();
    }

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::ULTRA_LOW_POWER)
          .duration(3s);
    SystemSleepResult result = System.sleep(config);

    delay(1s); // FIXME: P2 specific due to USB thread
    waitUntil(Serial.isConnected);

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result.wakeupReason(), (int)SystemSleepWakeupReason::BY_RTC);
}

#if !HAL_PLATFORM_RTL872X

#if HAL_PLATFORM_BLE
test(23_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_Ble) {
    Serial.println("    >> Device enters ultra-low power mode. Please reconnect serial after device being connected by BLE Central.");
    Serial.println("    >> Press any key now");
    while(Serial.available() <= 0);
    while (Serial.available() > 0) {
        (void)Serial.read();
    }

    BLE.advertise();

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::ULTRA_LOW_POWER)
          .ble();
    SystemSleepResult result = System.sleep(config);

    waitUntil(Serial.isConnected);

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result.wakeupReason(), (int)SystemSleepWakeupReason::BY_BLE);

    BLE.stopAdvertising();
}
#endif // HAL_PLATFORM_BLE

test(24_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_Analog_Pin) {
    Serial.println("    >> Device enters stop mode. Please reconnect serial after applying voltage crossing 1500mV on A0.");
    Serial.println("    >> Press any key now");
    while(Serial.available() <= 0);
    while (Serial.available() > 0) {
        (void)Serial.read();
    }

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP)
          .analog(A0, 1500, AnalogInterruptMode::CROSS);
    SystemSleepResult result = System.sleep(config);

    waitUntil(Serial.isConnected);

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result.wakeupReason(), (int)SystemSleepWakeupReason::BY_LPCOMP);
}

test(25_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_Analog_Pin) {
    Serial.println("    >> Device enters ultra-low power mode. Please reconnect serial after applying voltage crossing 1500mV on A0.");
    Serial.println("    >> Press any key now");
    while(Serial.available() <= 0);
    while (Serial.available() > 0) {
        (void)Serial.read();
    }

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::ULTRA_LOW_POWER)
          .analog(A0, 1500, AnalogInterruptMode::CROSS);
    SystemSleepResult result = System.sleep(config);

    waitUntil(Serial.isConnected);

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result.wakeupReason(), (int)SystemSleepWakeupReason::BY_LPCOMP);
}

test(26_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_Usart) {
    Serial.println("    >> Device enters stop mode. Please reconnect serial after sending characters over Serial1 @115200bps");
    Serial.println("    >> Press any key now");
    while(Serial.available() <= 0);
    while (Serial.available() > 0) {
        (void)Serial.read();
    }

    Serial1.begin(115200);

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP)
          .usart(Serial1);
    SystemSleepResult result = System.sleep(config);

    Serial1.end();

    waitUntil(Serial.isConnected);

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result.wakeupReason(), (int)SystemSleepWakeupReason::BY_USART);
}

test(27_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_Usart) {
    Serial.println("    >> Device enters ultra-low power mode. Please reconnect serial after sending characters over Serial1 @115200bps");
    Serial.println("    >> Press any key now");
    while(Serial.available() <= 0);
    while (Serial.available() > 0) {
        (void)Serial.read();
    }

    Serial1.begin(115200);

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::ULTRA_LOW_POWER)
          .usart(Serial1);
    SystemSleepResult result = System.sleep(config);

    Serial1.end();

    waitUntil(Serial.isConnected);

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result.wakeupReason(), (int)SystemSleepWakeupReason::BY_USART);
}

#if HAL_PLATFORM_CELLULAR
test(28_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_Cellular) {
    Serial.println("    >> Device enters stop mode. Please reconnect serial after waking up by network data");
    Serial.println("    >> Press any key now");
    while(Serial.available() <= 0);
    while (Serial.available() > 0) {
        (void)Serial.read();
    }

    Serial.println("    >> Turning on the modem");
    Cellular.on();
    waitFor(Cellular.isOn, 60000);
    Serial.println("    >> Connecting to the cloud");
    Particle.connect();
    assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));
    Serial.println("    >> Connected to the cloud. You'll see the RGB is turned on after waking up.");

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP)
          .network(Cellular);
    SystemSleepResult result = System.sleep(config);

    waitUntil(Serial.isConnected);

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result.wakeupReason(), (int)SystemSleepWakeupReason::BY_NETWORK);
}

test(29_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_Cellular) {
    Serial.println("    >> Device enters ultra-low power mode. Please reconnect serial after waking up by network data");
    Serial.println("    >> Press any key now");
    while(Serial.available() <= 0);
    while (Serial.available() > 0) {
        (void)Serial.read();
    }

    Serial.println("    >> Connecting to the cloud");
    Cellular.on();
    Particle.connect();
    assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));
    Serial.println("    >> Connected to the cloud. You'll see the RGB is turned on after waking up.");

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::ULTRA_LOW_POWER)
          .network(Cellular);
    SystemSleepResult result = System.sleep(config);

    waitUntil(Serial.isConnected);

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result.wakeupReason(), (int)SystemSleepWakeupReason::BY_NETWORK);
}
#endif // HAL_PLATFORM_CELLULAR

#if HAL_PLATFORM_WIFI
test(30_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_WiFi) {
    Serial.println("    >> Device enters stop mode. Please reconnect serial after waking up by network data");
    Serial.println("    >> Press any key now");
    while(Serial.available() <= 0);
    while (Serial.available() > 0) {
        (void)Serial.read();
    }

    Serial.println("    >> Connecting to the cloud");
    WiFi.on();
    Particle.connect();
    waitUntil(Particle.connected);
    Serial.println("    >> Connected to the cloud. You'll see the RGB is turned on after waking up.");

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP)
          .network(WiFi);
    SystemSleepResult result = System.sleep(config);

    waitUntil(Serial.isConnected);

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result.wakeupReason(), (int)SystemSleepWakeupReason::BY_NETWORK);
}

test(31_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_By_WiFi) {
    Serial.println("    >> Device enters ultra-low power mode. Please reconnect serial after waking up by network data");
    Serial.println("    >> Press any key now");
    while(Serial.available() <= 0);
    while (Serial.available() > 0) {
        (void)Serial.read();
    }

    Serial.println("    >> Connecting to the cloud");
    WiFi.on();
    Particle.connect();
    waitUntil(Particle.connected);
    Serial.println("    >> Connected to the cloud. You'll see the RGB is turned on after waking up.");

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::ULTRA_LOW_POWER)
          .network(WiFi);
    SystemSleepResult result = System.sleep(config);

    waitUntil(Serial.isConnected);

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result.wakeupReason(), (int)SystemSleepWakeupReason::BY_NETWORK);
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
            Serial.printlnf("sNetworkOffTimestamp: %ld", sNetworkOffTimestamp);
        }
    });
}

test(33_System_Sleep_With_Configuration_Object_Stop_Mode_Execution_Time) {
    constexpr uint32_t SLEEP_DURATION_S = 3;
    Serial.println("    >> Press any key now");
    while(Serial.available() <= 0);
    while (Serial.available() > 0) {
        (void)Serial.read();
    }

    Serial.println("    >> Connecting to the cloud");
    Network.on();
    Particle.connect();
    assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));
    Serial.println("    >> Connected to the cloud");
    Serial.printf("    >> Enter stop mode. Please reconnect serial after %ld s\r\n", SLEEP_DURATION_S);

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP)
          .duration(SLEEP_DURATION_S * 1000);

    if (system_thread_get_state(nullptr) == spark::feature::ENABLED) {
        sNetworkOffTimestamp = Time.now();
    } else {
        sNetworkOffTimestamp = 0;
    }
    SystemSleepResult result = System.sleep(config);
    time32_t exit = Time.now();

    delay(1s); // FIXME: P2 specific due to USB thread
    waitUntil(Serial.isConnected);
    assertNotEqual(sNetworkOffTimestamp, 0);
    assertMoreOrEqual(exit - sNetworkOffTimestamp, SLEEP_DURATION_S);
    if (system_thread_get_state(nullptr) == spark::feature::ENABLED) {
        assertLessOrEqual(exit - sNetworkOffTimestamp, SLEEP_DURATION_S + 10 );
    } else {
        assertLessOrEqual(exit - sNetworkOffTimestamp, SLEEP_DURATION_S + 2 );
    }
    Serial.printf("Sleep execution time: %ld s\r\n", exit - sNetworkOffTimestamp);

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result.wakeupReason(), (int)SystemSleepWakeupReason::BY_RTC);

    // Make sure we reconnect back to the cloud
    assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));
}

test(34_System_Sleep_With_Configuration_Object_Ultra_Low_Power_Mode_Wakeup_Execution_Time) {
    constexpr uint32_t SLEEP_DURATION_S = 3;
    Serial.println("    >> Press any key now");
    while(Serial.available() <= 0);
    while (Serial.available() > 0) {
        (void)Serial.read();
    }

    Serial.println("    >> Connecting to the cloud");
    Network.on();
    Particle.connect();
    assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));
    Serial.println("    >> Connected to the cloud");
    Serial.printf("    >> Enter stop mode. Please reconnect serial after %ld s\r\n", SLEEP_DURATION_S);

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::ULTRA_LOW_POWER)
          .duration(SLEEP_DURATION_S * 1000);

    if (system_thread_get_state(nullptr) == spark::feature::ENABLED) {
        sNetworkOffTimestamp = Time.now();
    } else {
        sNetworkOffTimestamp = 0;
    }
    SystemSleepResult result = System.sleep(config);
    time32_t exit = Time.now();

    delay(1s); // FIXME: P2 specific due to USB thread
    waitUntil(Serial.isConnected);
    assertNotEqual(sNetworkOffTimestamp, 0);
    assertMoreOrEqual(exit - sNetworkOffTimestamp, SLEEP_DURATION_S);
    if (system_thread_get_state(nullptr) == spark::feature::ENABLED) {
        assertLessOrEqual(exit - sNetworkOffTimestamp, SLEEP_DURATION_S + 10 );
    } else {
        assertLessOrEqual(exit - sNetworkOffTimestamp, SLEEP_DURATION_S + 2 );
    }
    Serial.printf("Sleep execution time: %ld s\r\n", exit - sNetworkOffTimestamp);

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result.wakeupReason(), (int)SystemSleepWakeupReason::BY_RTC);
}

test(35_System_Sleep_With_Configuration_Object_Network_Power_State_Consistent_On) {
    Serial.println("    >> Press any key now");
    while(Serial.available() <= 0);
    while (Serial.available() > 0) {
        (void)Serial.read();
    }

    Particle.disconnect();
    assertTrue(waitFor(Particle.disconnected, CLOUD_CONNECT_TIMEOUT));

    {
        // Make sure the modem is off first
        Serial.println("    >> Powering off the modem...");
#if HAL_PLATFORM_CELLULAR
        Cellular.off();
        assertTrue(waitFor(Cellular.isOff, 60000));
        Serial.println("    >> Powering on the modem...");
        Cellular.on();
        assertTrue(waitFor(Cellular.isOn, 60000));
#elif HAL_PLATFORM_WIFI
        WiFi.off();
        assertTrue(waitFor(WiFi.isOff, 60000));
        Serial.println("    >> Powering on the modem...");
        WiFi.on();
        assertTrue(waitFor(WiFi.isOn, 60000));
#endif

        Serial.println("    >> Entering sleep... Please reconnect serial after 3 seconds");
        SystemSleepResult result = System.sleep(SystemSleepConfiguration().mode(SystemSleepMode::STOP).duration(3s));

        delay(1s); // FIXME: P2 specific due to USB thread
        waitUntil(Serial.isConnected);

        assertEqual(result.error(), SYSTEM_ERROR_NONE);
        assertEqual((int)result.wakeupReason(), (int)SystemSleepWakeupReason::BY_RTC);

        Serial.println("    >> Waiting for the modem to be turned on...");
#if HAL_PLATFORM_CELLULAR
        assertTrue(waitFor(Cellular.isOn, 60000));
#elif HAL_PLATFORM_WIFI
        assertTrue(waitFor(WiFi.isOn, 60000));
#endif
    }
}

test(36_System_Sleep_With_Configuration_Object_Network_Power_State_Consistent_Off) {
    Serial.println("    >> Press any key now");
    while(Serial.available() <= 0);
    while (Serial.available() > 0) {
        (void)Serial.read();
    }

    {
        // Make sure the modem is on first
        Serial.println("    >> Powering on the modem...");
#if HAL_PLATFORM_CELLULAR
        Cellular.on();
        assertTrue(waitFor(Cellular.isOn, 60000));
        Serial.println("    >> Powering off the modem...");
        Cellular.off();
        assertTrue(waitFor(Cellular.isOff, 60000));
#elif HAL_PLATFORM_WIFI
        WiFi.on();
        assertTrue(waitFor(WiFi.isOn, 60000));
        Serial.println("    >> Powering off the modem...");
        WiFi.off();
        assertTrue(waitFor(WiFi.isOff, 60000));
#endif

        Serial.println("    >> Entering sleep... Please reconnect serial after 3 seconds");
        SystemSleepResult result = System.sleep(SystemSleepConfiguration().mode(SystemSleepMode::STOP).duration(3s));

        delay(1s); // FIXME: P2 specific due to USB thread
        waitUntil(Serial.isConnected);

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
