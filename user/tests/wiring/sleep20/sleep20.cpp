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

STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY));
static retained uint32_t magick = 0;
static retained uint32_t phase = 0;

test(01_System_Sleep_With_Configuration_Object_Hibernate_Mode_Without_Wakeup) {
    if (magick != 0xdeadbeef) {
        magick = 0xdeadbeef;
        phase = 0xbeef0001;
    }
    if (phase == 0xbeef0001) {
        Serial.println("    >> Device enters hibernate mode.");
        Serial.println("    >> Please reconnect serial and type 't' after you pressing the reset button.");
        delay(1000);

        phase = 0xbeef0002;

        SystemSleepConfiguration config;
        config.mode(SystemSleepMode::HIBERNATE);
        SystemSleepResult result = System.sleep(config);
        assertEqual(result.error(), SYSTEM_ERROR_NONE);
    } else if (phase == 0xbeef0002) {
        Serial.println("    >> Device is reset from hibernate mode.");
        assertEqual(System.resetReason(), (int)RESET_REASON_PIN_RESET);
    }
}

test(02_System_Sleep_Mode_Deep_Without_Wakeup) {
    if (phase == 0xbeef0002) {
        Serial.println("    >> Device enters hibernate mode.");
        Serial.println("    >> Please reconnect serial and type 't' after you pressing the reset button.");
        delay(1000);

        phase = 0xbeef0003;

        SleepResult result = System.sleep(SLEEP_MODE_DEEP, SLEEP_DISABLE_WKP_PIN);
        assertEqual(result.error(), SYSTEM_ERROR_NONE);
    } else if (phase == 0xbeef0003) {
        Serial.println("    >> Device is reset from hibernate mode.");
        assertEqual(System.resetReason(), (int)RESET_REASON_PIN_RESET);
    }
}

#if HAL_PLATFORM_NRF52840
test(03_System_Sleep_With_Configuration_Object_Hibernate_Mode_Wakeup_By_D0) {
    if (phase == 0xbeef0003) {
        Serial.println("    >> Device enters hibernate mode.");
        Serial.println("    >> Please reconnect serial and type 't' after you have a rising edge on D0.");
        delay(1000);

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
        delay(1000);

        phase = 0xbeef0005;

        SleepResult result = {};

// Tracker support waking up device from hibernate mode by external RTC
#if PLATFORM_ID != PLATFORM_TRACKER
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

// Tracker support waking up device from hibernate mode by external RTC
#if PLATFORM_ID == PLATFORM_TRACKER
test(03_System_Sleep_With_Configuration_Object_Hibernate_Mode_Wakeup_By_External_Rtc) {
    if (phase == 0xbeef0003) {
        Serial.println("    >> Device enters hibernate mode.");
        Serial.println("    >> Please reconnect serial and type 't' after 3 seconds.");
        delay(1000);

        phase = 0xbeef0004;

        SystemSleepConfiguration config;
        config.mode(SystemSleepMode::HIBERNATE)
              .duration(3s);
        SystemSleepResult result = System.sleep(config);
        assertEqual(result.error(), SYSTEM_ERROR_NONE);
    } else if (phase == 0xbeef0004) {
        Serial.println("    >> Device is woken up from hibernate mode.");
        assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);
    }
}

test(04_System_Sleep_Mode_Deep_Wakeup_By_External_Rtc) {
    if (phase == 0xbeef0004) {
        Serial.println("    >> Device enters hibernate mode.");
        Serial.println("    >> Please reconnect serial and type 't' after 3 seconds.");
        delay(1000);

        phase = 0xbeef0005;

        SleepResult result = System.sleep(SLEEP_MODE_DEEP, 3s);
        assertEqual(result.error(), SYSTEM_ERROR_NONE);
    } else if (phase == 0xbeef0005) {
        Serial.println("    >> Device is woken up from hibernate mode.");
        assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);
    }
}
#endif // PLATFORM_ID != PLATFORM_TRACKER

#endif // HAL_PLATFORM_NRF52840

#if HAL_PLATFORM_STM32F2XX
test(03_System_Sleep_With_Configuration_Object_Hibernate_Mode_Wakeup_By_Wkp_Pin) {
    if (phase == 0xbeef0003) {
        Serial.println("    >> Device enters hibernate mode.");
        Serial.println("    >> Please reconnect serial and type 't' after you have a rising edge on WKP pin.");
        delay(1000);

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

test(04_System_Sleep_With_Configuration_Object_Hibernate_Mode_Wakeup_By_Rtc) {
    if (phase == 0xbeef0004) {
        Serial.println("    >> Device enters hibernate mode.");
        Serial.println("    >> Please reconnect serial and type 't' after 3 seconds.");
        delay(1000);

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

test(05_System_Sleep_Mode_Deep_Wakeup_By_Wkp_Pin) {
    if (phase == 0xbeef0005) {
        Serial.println("    >> Device enters hibernate mode.");
        Serial.println("    >> Please reconnect serial and type 't' after you have a rising edge on WKP pin.");
        delay(1000);

        phase = 0xbeef0006;

        SleepResult result = System.sleep(SLEEP_MODE_DEEP);
        assertEqual(result.error(), SYSTEM_ERROR_NONE);
    } else if (phase == 0xbeef0006) {
        Serial.println("    >> Device is woken up from hibernate mode.");
        assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);
    }
}

test(06_System_Sleep_Mode_Deep_Wakeup_By_Rtc) {
    if (phase == 0xbeef0006) {
        Serial.println("    >> Device enters hibernate mode.");
        Serial.println("    >> Please reconnect serial and type 't' after 3 seconds.");
        delay(1000);

        phase = 0xbeef0007;

        SleepResult result = System.sleep(SLEEP_MODE_DEEP, 3s, SLEEP_DISABLE_WKP_PIN); // Disable WKP pin.
        assertEqual(result.error(), SYSTEM_ERROR_NONE);
    } else if (phase == 0xbeef0007) {
        Serial.println("    >> Device is woken up from hibernate mode.");
        assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);
    }
}
#endif // HAL_PLATFORM_STM32F2XX

test(07_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_D0) {
    Serial.println("    >> Device enters stop mode. Please reconnect serial after you have a rising edge on D0.");
    delay(1000);

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP)
          .gpio(D0, RISING);
    SystemSleepResult result = System.sleep(config);

    while (!Serial.isConnected());

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result.wakeupReason(), (int)SystemSleepWakeupReason::BY_GPIO);
    assertEqual(result.wakeupPin(), D0);
}

test(08_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_Rtc) {
    Serial.println("    >> Device enters stop mode. Please reconnect serial after 3 seconds.");
    delay(1000);

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP)
          .duration(3s);
    SystemSleepResult result = System.sleep(config);

    while (!Serial.isConnected());

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result.wakeupReason(), (int)SystemSleepWakeupReason::BY_RTC);
}

#if HAL_PLATFORM_BLE
test(09_System_Sleep_With_Configuration_Object_Stop_Mode_Wakeup_By_Ble) {
    Serial.println("    >> Device enters stop mode. Please reconnect serial after device being connected by BLE Central.");
    delay(1000);

    BLE.advertise();

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP)
          .ble();
    SystemSleepResult result = System.sleep(config);

    while (!Serial.isConnected());

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result.wakeupReason(), (int)SystemSleepWakeupReason::BY_BLE);

    BLE.stopAdvertising();
}
#endif // HAL_PLATFORM_BLE

test(10_System_Sleep_Mode_Stop_Wakeup_By_D0) {
    Serial.println("    >> Device enters stop mode. Please reconnect serial after you have a rising edge on D0.");
    delay(1000);

    SleepResult result = System.sleep(D0, RISING);

    while (!Serial.isConnected());

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result.reason(), (int)WAKEUP_REASON_PIN);
    assertEqual(result.pin(), D0);
}

test(11_System_Sleep_Mode_Stop_Wakeup_By_Rtc) {
    Serial.println("    >> Device enters stop mode. Please reconnect serial after 3 seconds");
    delay(1000);

    SleepResult result = System.sleep(nullptr, 0, nullptr, 0, 3s);

    while (!Serial.isConnected());

    assertEqual(result.error(), SYSTEM_ERROR_NONE);
    assertEqual((int)result.reason(), (int)WAKEUP_REASON_RTC);
}

test(12_System_Sleep_With_Configuration_Object_Stop_Mode_Without_Wakeup) {
    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP);
    SystemSleepResult result = System.sleep(config);
    assertEqual(result.error(), SYSTEM_ERROR_INVALID_ARGUMENT);
}

test(13_System_Sleep_Mode_Stop_Without_Wakeup) {
    SleepResult result = System.sleep(nullptr, 0, nullptr, 0, 0);
    assertEqual(result.error(), SYSTEM_ERROR_INVALID_ARGUMENT);
}
