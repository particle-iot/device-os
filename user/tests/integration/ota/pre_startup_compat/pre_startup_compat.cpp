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

volatile int updateResult = firmware_update_failed;

void prepareOta(bool connect = true, bool compressedOta = true) {
    System.disableReset();

    System.on(firmware_update, [](system_event_t ev, int data, void* context) {
        updateResult = data;
    });

    updateResult = SYSTEM_ERROR_OTA;

#if HAL_PLATFORM_COMPRESSED_OTA
    spark_protocol_set_connection_property(spark_protocol_instance(), protocol::Connection::COMPRESSED_OTA, compressedOta, nullptr, nullptr);
#endif

    if (connect) {
        Particle.connect();
        waitFor(Particle.connected, 5 * 60 * 1000);
    }
}

void waitOta() {
    for (auto start = millis(); millis() - start <= 20 * 60 * 1000;) {
        if (updateResult == firmware_update_complete || updateResult == firmware_update_failed || updateResult == firmware_update_pending) {
            break;
        } else if (updateResult == SYSTEM_ERROR_OTA && millis() - start >= 1 * 60 * 1000) {
            break;
        }
        Particle.process();
        delay(100);
    }

    if (Particle.connected() && updateResult == firmware_update_complete) {
        // Just in case
        Particle.publish("test/ota", "success", WITH_ACK).wait();
    }

    assertNotEqual((int)updateResult, (int)firmware_update_failed);
    assertNotEqual((int)updateResult, (int)SYSTEM_ERROR_OTA);
}

bool preStartupRan = false;
bool startupRan = false;

} // anonymous

void PRE_STARTUP() {
    preStartupRan = true;
}

STARTUP({
    startupRan = true; 
    testAppInit();
});

void setup() {
    testAppSetup();
}

void loop() {
    testAppLoop();
}

test(01_check_current_application) {
    assertTrue(preStartupRan);
    assertTrue(startupRan);
}

test(02_ota_before_pre_startup_application_start) {
    prepareOta(true, false);
}

test(03_ota_before_pre_startup_application_wait_1) {
    waitOta();
}

test(03_ota_before_pre_startup_application_wait_2) {
    System.off(all_events);
    expectSystemReset();
    System.enableReset();
    // Should not reach normally
    delay(5000);
    System.reset();
}

test(04_check_before_pre_startup_application) {
    assertFalse(preStartupRan);
    assertTrue(startupRan);
}

test(05_ota_original_application_start) {
    prepareOta(true, false);
}

test(06_ota_original_application_wait_1) {
    waitOta();
}

test(06_ota_original_application_wait_2) {
    System.off(all_events);
    expectSystemReset();
    System.enableReset();
    // Should not reach normally
    delay(5000);
    System.reset();
}

test(07_check_original_application) {
    assertTrue(preStartupRan);
    assertTrue(startupRan);
}

test(99_cleanup) {
    // Just in case
    System.enableReset();
}
