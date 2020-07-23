/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHAN'TABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "Particle.h"

#define SYSTEM_MODE_MANUAL          1
#define ENABLE_CLOUD_CONNECTION     1

#ifdef SYSTEM_MODE_MANUAL
SYSTEM_MODE(SEMI_AUTOMATIC);
#else
SYSTEM_MODE(AUTOMATIC);
#endif

SYSTEM_THREAD(ENABLED);

// STARTUP(Cellular.setActiveSim(EXTERNAL_SIM));

Serial1LogHandler log(115200, LOG_LEVEL_ALL);

static void enterStopMode() {
    LOG(TRACE, "Device is entering stop mode.");
    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP)
          .gpio(D0, FALLING)
          .duration(10s);
    System.sleep(config);
    LOG(TRACE, "Device is woken up.");
}

static void enterUltraLowPowerMode() {
    LOG(TRACE, "Device is entering ultra-low power mode.");
    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::ULTRA_LOW_POWER)
          .gpio(D0, FALLING)
          .duration(10s);
    System.sleep(config);
    LOG(TRACE, "Device is woken up.");
}

static void enterHibernateMode() {
    LOG(TRACE, "Device is entering hibernate mode.");
    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::HIBERNATE)
          .gpio(D0, FALLING);
    System.sleep(config);
}

void setup() {
    LOG(TRACE, "Application started.");
    delay(1000);

    Wire.begin();
    SPI.begin();

#ifdef SYSTEM_MODE_MANUAL
#if HAL_PLATFORM_WIFI
    WiFi.on();
#endif
#if HAL_PLATFORM_CELLULAR
    Cellular.on();
    // Enabled NB-IoT in China
    // delay(20000);
    // Cellular.command("AT+UMNOPROF=0\r\n");
    // Cellular.command(1000, "AT+URAT=8\r\n");
#endif
#endif // SYSTEM_MODE_MANUAL

#if ENABLE_CLOUD_CONNECTION
    Particle.connect();
    waitUntil(Particle.connected);
#endif // ENABLE_CLOUD_CONNECTION

    delay(5000);
}

void loop() {
    enterStopMode();

#if ENABLE_CLOUD_CONNECTION
    waitUntil(Particle.connected);
#endif
    delay(5000);

    enterUltraLowPowerMode();

#if ENABLE_CLOUD_CONNECTION
    waitUntil(Particle.connected);
#endif
    delay(5000);

    enterHibernateMode();
}