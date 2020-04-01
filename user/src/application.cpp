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
 * MERCHAN'TABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "Particle.h"

#define IO_EXPANDER_THREAD_PRESENT          1
#define GNSS_THREAD_PRESENT                 1
#define BMI160_THREAD_PRESENT               1

SYSTEM_MODE(MANUAL);
SYSTEM_THREAD(ENABLED);

STARTUP(System.enable(SYSTEM_FLAG_PM_DETECTION));

SerialLogHandler log(LOG_LEVEL_ALL);

#if IO_EXPANDER_THREAD_PRESENT
uint32_t ioExpanderFailure = 0;
os_thread_return_t ioExpanderThread(void *param) {
    pinMode(RTC_WDI, OUTPUT);
    while (1) {
        digitalWrite(RTC_WDI, HIGH);
        if (digitalRead(RTC_WDI) != 1) {
            LOG(TRACE, "IO Expander output high failed.");
            ioExpanderFailure++;
        }

        delay(100);

        digitalWrite(RTC_WDI, LOW);
        if (digitalRead(RTC_WDI) != 0) {
            LOG(TRACE, "IO Expander output low failed.");
            ioExpanderFailure++;
        }
    }
}
#endif

#if GNSS_THREAD_PRESENT
uint32_t gnssFailure = 0;
os_thread_return_t gnssThread(void *param) {
    pinMode(GPS_BOOT, OUTPUT);
    digitalWrite(GPS_BOOT, HIGH);
    pinMode(GPS_PWR, OUTPUT);
    digitalWrite(GPS_PWR, HIGH);
    pinMode(GPS_CS, OUTPUT);
    digitalWrite(GPS_CS, HIGH);
    pinMode(GPS_RST, OUTPUT);
    digitalWrite(GPS_RST, HIGH);
    digitalWrite(GPS_RST, LOW);
    delay(50);
    digitalWrite(GPS_RST, HIGH);

    SPI1.begin();

    while (1) {
        uint8_t gpsData;
        uint8_t dummyBytes = 0;
        bool echoed = false;

        SPI1.beginTransaction(__SPISettings(1*MHZ, MSBFIRST, SPI_MODE0));
        digitalWrite(GPS_CS, LOW);
        delay(1000);
        while (dummyBytes < 50) {
            gpsData = SPI1.transfer(0xFF);
            if (gpsData == 0xFF)
                dummyBytes++;
            else {
                // Serial.printf("%c", gpsData);
                echoed = true;
                dummyBytes = 0;
            }
        }

        digitalWrite(GPS_CS, HIGH);
        SPI1.endTransaction();

        if (!echoed) {
            LOG(TRACE, "Reading GPS data failed.");
            gnssFailure++;
        }
        delay(5000);
    }
}
#endif

#if BMI160_THREAD_PRESENT
uint32_t bmi160Failure = 0;
os_thread_return_t bmi160Thread(void *param) {
    pinMode(SEN_CS, OUTPUT);
    digitalWrite(SEN_CS, LOW);
    delay(50);
    digitalWrite(SEN_CS, HIGH); //Rising edge on the /CSB pin to select SPI interface.

    SPI1.begin();

    while (1) {
        SPI1.beginTransaction(__SPISettings(1*MHZ, MSBFIRST, SPI_MODE3));
        digitalWrite(SEN_CS, LOW);
        SPI1.transfer(0x80); // R/W bit along with register address
        uint8_t id = SPI1.transfer(0xFF);
        digitalWrite(SEN_CS, HIGH);
        SPI1.endTransaction();

        if (id != 0xD1) {
            LOG(TRACE, "Reading BMI160 failed.");
            bmi160Failure++;
        }
        delay(100);
    }
}
#endif

void setup() {
    Serial.begin(115200);
    while(!Serial.isConnected());
    delay(500);
    LOG(TRACE, "Test application started.");

#if IO_EXPANDER_THREAD_PRESENT
    Thread* thread1 = new Thread("ioExpanderThread", ioExpanderThread);
#endif

#if GNSS_THREAD_PRESENT
    Thread* thread2 = new Thread("gnssThread", gnssThread);
#endif

#if BMI160_THREAD_PRESENT
    Thread* thread3 = new Thread("bmi160Thread", bmi160Thread);
#endif
}

void loop() {
#if IO_EXPANDER_THREAD_PRESENT
    LOG(INFO, "IO Expander failure: %d", ioExpanderFailure);
#endif

#if GNSS_THREAD_PRESENT
    LOG(INFO, "GNSS failure: %d", gnssFailure);
#endif

#if BMI160_THREAD_PRESENT
    LOG(INFO, "BMI160 failure: %d", bmi160Failure);
#endif

    delay(10000);
}
