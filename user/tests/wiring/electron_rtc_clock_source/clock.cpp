/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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
#include "test_config.h"

#ifndef PARTICLE_TEST_RUNNER
#error "This test requires to be run under device-os-test"
#endif // PARTICLE_TEST_RUNNER

#ifndef TEST_ELECTRON_USE_HSE_LSI
#define TEST_ELECTRON_USE_HSE_LSI (1)
#endif // TEST_ELECTRON_USE_HSE_LSI

#ifndef TEST_ELECTRON_VALIDATE_USES_HSE_LSI
#define TEST_ELECTRON_VALIDATE_USES_HSE_LSI (0)
#endif // TEST_ELECTRON_VALIDATE_USES_HSE_LSI

namespace {

retained time_t sRtcTime = 0;

} // anonymous

test(01_connect_set_feature_reset) {
    if (TEST_ELECTRON_USE_HSE_LSI) {
        assertEqual((RCC->BDCR & 0x300), RCC_RTCCLKSource_LSE);
        System.enableFeature(FEATURE_DISABLE_EXTERNAL_LOW_SPEED_CLOCK);
        assertTrue(System.featureEnabled(FEATURE_DISABLE_EXTERNAL_LOW_SPEED_CLOCK));
    } else {
        if (TEST_ELECTRON_VALIDATE_USES_HSE_LSI) {
            assertEqual((RCC->BDCR & 0x300), (RCC_RTCCLKSource_HSE_Div2 & 0x300));
        }
        System.disableFeature(FEATURE_DISABLE_EXTERNAL_LOW_SPEED_CLOCK);
        assertFalse(System.featureEnabled(FEATURE_DISABLE_EXTERNAL_LOW_SPEED_CLOCK));
    }

    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

    if (!Particle.syncTimeDone()) {
        Particle.syncTime();
        assertTrue(waitFor(Particle.syncTimeDone, 30000));
    }

    assertTrue(Time.isValid());
    sRtcTime = Time.now();

    // Test runner will reset the device after this
}

test(02_validate) {
    if (TEST_ELECTRON_USE_HSE_LSI) {
        assertEqual((RCC->BDCR & 0x300), (RCC_RTCCLKSource_HSE_Div2 & 0x300));
    } else {
        assertEqual((RCC->BDCR & 0x300), RCC_RTCCLKSource_LSE);
    }
    delay(1000);
    // FIXME: time seems to default to 2000-01-01 in some cases
    // when switching from HSE to LSE
    // assertMore(Time.now(), sRtcTime);

    const unsigned testSeconds = 5;
    system_tick_t t0 = 0, t1 = 0;
    time_t prev = Time.now();
    system_tick_t started = millis();
    while(millis() - started < testSeconds * 1000 * 2) {
        auto now = Time.now();
        if (now > prev && t0 == 0) {
            t0 = millis();
            prev = now;
        } else if (now - prev >= testSeconds) {
            t1 = millis();
            break;
        }
    }

    assertNotEqual(t0, 0);
    assertNotEqual(t1, 0);
    assertMoreOrEqual((t1 - t0), testSeconds * 1000 * 9 / 10); // 5 -10%
    assertLessOrEqual((t1 - t0), testSeconds * 1000 * 11 / 10); // 5 +10%
}
