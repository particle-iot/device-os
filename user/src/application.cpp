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

#include "port.h"
#include "sdspi_host.h"
#include <cstdlib>
#include <string>

#define IO_EXPANDER_THREAD_PRESENT          1
#define GNSS_THREAD_PRESENT                 1
#define BMI160_THREAD_PRESENT               1
#define ESP32_THREAD_PRESENT                1
#define CAN_THREAD_PRESENT                  1

// In ms
#define IO_EXPANDER_ACCESS_PERIOD           100
#define GNSS_ACCESS_PERIOD                  1000
#define BMI160_ACESS_PERIOD                 100
#define ESP32_ACCESS_PERIOD                 1000
#define CAN_ACCESS_PERIOD                   100

SYSTEM_MODE(MANUAL);
SYSTEM_THREAD(ENABLED);

SerialLogHandler log(LOG_LEVEL_ALL);

#if IO_EXPANDER_THREAD_PRESENT
uint32_t ioExpanderFailure = 0;
os_thread_return_t ioExpanderThread(void *param) {
    pinMode(RTC_WDI, OUTPUT);
    while (1) {
        digitalWrite(RTC_WDI, HIGH);
        if (digitalRead(RTC_WDI) != 1) {
            LOG(ERROR, "IO Expander output high failed.");
            ioExpanderFailure++;
        }

        delay(IO_EXPANDER_ACCESS_PERIOD);

        digitalWrite(RTC_WDI, LOW);
        if (digitalRead(RTC_WDI) != 0) {
            LOG(ERROR, "IO Expander output low failed.");
            ioExpanderFailure++;
        }

        delay(IO_EXPANDER_ACCESS_PERIOD);
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

        SPI1.beginTransaction(__SPISettings(16*MHZ, MSBFIRST, SPI_MODE0));
        digitalWrite(GPS_CS, LOW);
        delay(1000);
        while (dummyBytes < 50) {
            gpsData = SPI1.transfer(0xFF);
            if (gpsData == 0xFF)
                dummyBytes++;
            else {
                echoed = true;
                dummyBytes = 0;
            }
        }
        digitalWrite(GPS_CS, HIGH);
        SPI1.endTransaction();

        if (!echoed) {
            LOG(ERROR, "Reading GPS data failed.");
            gnssFailure++;
        }
        delay(GNSS_ACCESS_PERIOD);
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
        SPI1.beginTransaction(__SPISettings(16*MHZ, MSBFIRST, SPI_MODE3));
        digitalWrite(SEN_CS, LOW);
        SPI1.transfer(0x80); // R/W bit along with register address
        uint8_t id = SPI1.transfer(0xFF);
        digitalWrite(SEN_CS, HIGH);
        SPI1.endTransaction();

        if (id != 0xD1) {
            LOG(ERROR, "Reading BMI160 failed.");
            bmi160Failure++;
        }
        delay(BMI160_ACESS_PERIOD);
    }
}
#endif

#if ESP32_THREAD_PRESENT
uint32_t esp32Failure = 0;
uint8_t txStr[] = "AT\r\n";
uint8_t echoStr[] = "OK\r\n";
spi_context_t context;
os_semaphore_t echoSem;

os_thread_return_t esp32TxThread(void *param) {
    while (1) {
        delay(ESP32_ACCESS_PERIOD);
        esp_err_t ret = at_sdspi_send_packet(&context, txStr, 4, UINT32_MAX);
        if (ret != ESP_OK) {
            LOG(ERROR, "Sending to ESP32 failed: %d", ret);
            esp32Failure++;
            continue;
        }
        if (os_semaphore_take(echoSem, 5000/*ms*/, false)) {
            LOG(ERROR, "Receiving OK failed.");
            esp32Failure++;
        }
    }
}

os_thread_return_t esp32RxThread(void *param) {
    esp_err_t ret;
    uint32_t intr_raw;
    while (1) {
        at_spi_wait_int(CONCURRENT_WAIT_FOREVER);

        if ((ret = at_sdspi_get_intr(&intr_raw)) != ESP_OK) {
            LOG(ERROR, "at_sdspi_get_intr() failed: %d", ret);
            continue;
        }
        if ((ret = at_sdspi_clear_intr(intr_raw)) !=ESP_OK) {
            LOG(ERROR, "at_sdspi_clear_intr() failed: %d", ret);
            continue;
        }
        if (intr_raw & HOST_SLC0_RX_NEW_PACKET_INT_ST) {
            uint8_t readBuf[16] = {};
            size_t readBytes = 0;
            ret = at_sdspi_get_packet(&context, readBuf, sizeof(readBuf), &readBytes);
            if (ret == ESP_ERR_NOT_FOUND) {
                LOG(ERROR, "ESP32 interrupt but no data can be read");
                continue;
            } else if (ret != ESP_OK && ret != ESP_ERR_NOT_FINISHED) {
                LOG(ERROR, "ESP32 rx packet error: %08X", ret);
                continue;
            }
            if (strstr((const char*)readBuf, "OK")) {
                os_semaphore_give(echoSem, false);
            }
        } else {
            LOG(ERROR, "!(intr_raw & HOST_SLC0_RX_NEW_PACKET_INT_ST)");
        }
    }
}
#endif

#if CAN_THREAD_PRESENT
uint32_t canFailure = 0;
os_thread_return_t canThread(void *param) {
    pinMode(CAN_CS, OUTPUT);
    digitalWrite(CAN_CS, HIGH);
    pinMode(CAN_PWR, OUTPUT);
    digitalWrite(CAN_PWR, HIGH);
    pinMode(CAN_RST, OUTPUT);
    digitalWrite(CAN_RST, LOW);
    delay(100);
    digitalWrite(CAN_RST, HIGH);

    SPI1.begin();

    SPI1.beginTransaction(__SPISettings(16*MHZ, MSBFIRST, SPI_MODE0));
    digitalWrite(CAN_CS, LOW);
    SPI1.transfer(0xC0); // CMD: Reset registers
    digitalWrite(CAN_CS, HIGH);
    SPI1.endTransaction();

    while (1) {
        SPI1.beginTransaction(__SPISettings(1*MHZ, MSBFIRST, SPI_MODE0));
        digitalWrite(CAN_CS, LOW);
        SPI1.transfer(0x03); // CMD: Read
        SPI1.transfer(0x7F); // Register Address
        uint8_t canCtrlReg = SPI1.transfer(0xFF); // Value
        digitalWrite(CAN_CS, HIGH);
        SPI1.endTransaction();

        if (canCtrlReg != 0x87) {
            LOG(ERROR, "Reading MCP25625 failed.");
            canFailure++;
        }
        delay(CAN_ACCESS_PERIOD);
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

#if ESP32_THREAD_PRESENT
    PORT_SPI.setDataMode(SPI_MODE0);
    PORT_SPI.begin();

    pinMode(PORT_CS_PIN, OUTPUT);
    digitalWrite(PORT_CS_PIN, HIGH);
    pinMode(PORT_BOOT_PIN, OUTPUT);
    digitalWrite(PORT_BOOT_PIN, HIGH);
    pinMode(PORT_EN_PIN, OUTPUT);
    digitalWrite(PORT_EN_PIN, LOW);
    delay(500);
    digitalWrite(PORT_EN_PIN, HIGH);

    if (at_sdspi_init() == ESP_OK) {
        memset(&context, 0x0, sizeof(spi_context_t));
        if (!os_semaphore_create(&echoSem, 1, 0)) {
            Thread* thread4 = new Thread("esp32TxThread", esp32TxThread);
            Thread* thread5 = new Thread("esp32RxThread", esp32RxThread);
        }
    }
#endif

#if CAN_THREAD_PRESENT
    Thread* thread6 = new Thread("canThread", canThread);
#endif
}

void loop() {
    delay(10000);

#if IO_EXPANDER_THREAD_PRESENT
    LOG(INFO, "IO Expander failure: %d", ioExpanderFailure);
#endif

#if GNSS_THREAD_PRESENT
    LOG(INFO, "GNSS failure: %d", gnssFailure);
#endif

#if BMI160_THREAD_PRESENT
    LOG(INFO, "BMI160 failure: %d", bmi160Failure);
#endif

#if ESP32_THREAD_PRESENT
    LOG(INFO, "ESP32 failure: %d", esp32Failure);
#endif

#if CAN_THREAD_PRESENT
    LOG(INFO, "CAN bus failure: %d", canFailure);
#endif

    LOG(INFO, "");
}
