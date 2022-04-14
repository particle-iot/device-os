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

#if PLATFORM_ID != PLATFORM_TRACKER
#error "This test is not applicable for the specified platform."
#endif

#if PLATFORM_ID == PLATFORM_TRACKER

#include "port.h"
#include "sdspi_host.h"
#include <cstdlib>
#include <string>

#define IO_EXPANDER_THREAD_PRESENT          1
#define GNSS_THREAD_PRESENT                 1
#define BMI160_THREAD_PRESENT               1
#define ESP32_THREAD_PRESENT                1
#define CAN_THREAD_PRESENT                  1

#define GNSS_LOG_VERBOSE                    0
#define GNSS_ENABLE_TX_READY                0

// In ms
#define IO_EXPANDER_ACCESS_PERIOD           10
#define GNSS_ACCESS_PERIOD                  1000
#define BMI160_ACESS_PERIOD                 10
#define ESP32_ACCESS_PERIOD                 10
#define CAN_ACCESS_PERIOD                   10

SYSTEM_MODE(MANUAL);
SYSTEM_THREAD(ENABLED);

SerialLogHandler log(LOG_LEVEL_ALL);

// This thread simply toggles the RTC_WDI pin for stress test purpose.
#if IO_EXPANDER_THREAD_PRESENT
uint32_t ioExpanderFailure = 0;
uint32_t ioExpanderAccessCnt = 0, preIoExpanderAccessCnt = 0;
os_thread_return_t ioExpanderThread(void *param) {
    pinMode(RTC_WDI, OUTPUT);
    while (1) {
        digitalWrite(RTC_WDI, HIGH);
        if (digitalRead(RTC_WDI) != 1) {
            LOG(ERROR, "IO Expander output high failed.");
            ioExpanderFailure++;
        } else {
            ioExpanderAccessCnt++;
        }

        delay(IO_EXPANDER_ACCESS_PERIOD);

        digitalWrite(RTC_WDI, LOW);
        if (digitalRead(RTC_WDI) != 0) {
            LOG(ERROR, "IO Expander output low failed.");
            ioExpanderFailure++;
        } else {
            ioExpanderAccessCnt++;
        }

        delay(IO_EXPANDER_ACCESS_PERIOD);
    }
}
#endif  // IO_EXPANDER_THREAD_PRESENT

#if GNSS_THREAD_PRESENT
uint32_t gnssFailure = 0;
uint32_t gnssAccessCnt = 0, preGnssAccessCnt = 0;

#if GNSS_ENABLE_TX_READY
os_semaphore_t gnssReadySem = nullptr;
// Dirty hack!
uint8_t ubxCfgMsg[] = {
    0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x04, 0x00, 0x3B, 0x01, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x96, 0x07,
    0xB5, 0x62, 0x05, 0x01, 0x02, 0x00, 0x06, 0x00, 0x0E, 0x37,
    0xb5, 0x62, 0x06, 0x00, 0x01, 0x00, 0x04, 0x0b, 0x25,
    0xb5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x04, 0x00, 0x3b, 0x01, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x96, 0x07,
    0xB5, 0x62, 0x05, 0x01, 0x02, 0x00, 0x06, 0x00, 0x0E, 0x37
};
void gnssIntHandler(void) {
    if (gnssReadySem) {
        os_semaphore_give(gnssReadySem, false);
    }
}
#endif // GNSS_ENABLE_TX_READY

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

#if GNSS_ENABLE_TX_READY
    attachInterrupt(GPS_INT, gnssIntHandler, FALLING);
    // Enable TX-ready
    SPI1.beginTransaction(SPISettings(5*MHZ, MSBFIRST, SPI_MODE0));
    digitalWrite(GPS_CS, LOW);
    delay(50);
    uint8_t in_byte;
    for (int i=0; i < sizeof(ubxCfgMsg); i++) {
        in_byte = SPI1.transfer(ubxCfgMsg[i]);
        if (in_byte != 0xFF) {
            Serial.write(in_byte);
        }
        delay(10);
    }
    digitalWrite(GPS_CS, HIGH);
    delay(5);
    SPI1.endTransaction();
#endif // GNSS_ENABLE_TX_READY

    while (1) {
#if GNSS_ENABLE_TX_READY
        if (os_semaphore_take(gnssReadySem, 10000/*ms*/, false)) {
            LOG(ERROR, "Wait GNSS data timeout.");
            gnssFailure++;
            continue;
        }
        // Read data
        SPI1.beginTransaction(SPISettings(5*MHZ, MSBFIRST, SPI_MODE0));
        digitalWrite(GPS_CS, LOW);
        delayMicroseconds(50);
        uint8_t temp;
        while(SPI1.transfer(0xFF) == 0xFF); // Just in case
#if GNSS_LOG_VERBOSE
        do {
            temp = SPI1.transfer(0xFF);
            Serial.printf("%c", temp);
        } while (temp != 0xFF);
#else
        while(SPI1.transfer(0xFF) != 0xFF); // Read rest data
#endif // GNSS_LOG_VERBOSE
        digitalWrite(GPS_CS, HIGH);
        delay(10);
        SPI1.endTransaction();
        gnssAccessCnt++;
#else
        bool echoed = false;
        system_tick_t start = millis();
        SPI1.beginTransaction(SPISettings(5*MHZ, MSBFIRST, SPI_MODE0));
        digitalWrite(GPS_CS, LOW);
        delayMicroseconds(50);
        while ((millis() - start) < 2000) {
            if (SPI1.transfer(0xFF) != 0xFF) {
                echoed = true;
#if GNSS_LOG_VERBOSE
                uint8_t temp;
                do {
                    temp = SPI1.transfer(0xFF);
                    Serial.printf("%c", temp);
                } while (temp != 0xFF);
#else
                while(SPI1.transfer(0xFF) != 0xFF); // Read rest data
#endif // GNSS_LOG_VERBOSE
                break;
            }
        }
        digitalWrite(GPS_CS, HIGH);
        SPI1.endTransaction();
        if (!echoed) {
            LOG(ERROR, "Reading GPS data failed.");
            gnssFailure++;
        } else {
            gnssAccessCnt++;
        }
        delay(GNSS_ACCESS_PERIOD);
#endif // GNSS_ENABLE_TX_READY
    }
}
#endif // GNSS_THREAD_PRESENT

#if BMI160_THREAD_PRESENT
uint32_t bmi160Failure = 0;
uint32_t bmi160AccessCnt = 0, preBmi160AccessCnt = 0;
os_thread_return_t bmi160Thread(void *param) {
    pinMode(SEN_CS, OUTPUT);
    digitalWrite(SEN_CS, LOW);
    delay(50);
    digitalWrite(SEN_CS, HIGH); //Rising edge on the /CSB pin to select SPI interface.

    // Perform a single read access to the 0x7F register before actual communication.
    SPI1.beginTransaction(SPISettings(16*MHZ, MSBFIRST, SPI_MODE3));
    digitalWrite(SEN_CS, LOW);
    SPI1.transfer(0xFF); // R/W bit along with register address
    SPI1.transfer(0xFF);
    digitalWrite(SEN_CS, HIGH);
    SPI1.endTransaction();

    while (1) {
        SPI1.beginTransaction(SPISettings(16*MHZ, MSBFIRST, SPI_MODE3));
        digitalWrite(SEN_CS, LOW);
        SPI1.transfer(0x80); // R/W bit along with register address
        uint8_t id = SPI1.transfer(0xFF);
        digitalWrite(SEN_CS, HIGH);
        SPI1.endTransaction();

        if (id != 0xD1) {
            LOG(ERROR, "Reading BMI160 failed.");
            bmi160Failure++;
        } else {
            bmi160AccessCnt++;
        }
        delay(BMI160_ACESS_PERIOD);
    }
}
#endif // BMI160_THREAD_PRESENT

#if ESP32_THREAD_PRESENT
uint32_t esp32Failure = 0;
uint8_t txStr[] = "AT\r\n";
uint8_t echoStr[] = "OK\r\n";
spi_context_t context;
os_mutex_t echoMutex;
uint32_t esp32AccessCnt = 0, preEsp32AccessCnt = 0;

os_thread_return_t esp32TxThread(void *param) {
    os_mutex_lock(echoMutex);
    while (1) {
        delay(ESP32_ACCESS_PERIOD);
        esp_err_t ret = at_sdspi_send_packet(&context, txStr, 4, UINT32_MAX);
        if (ret != ESP_OK) {
            LOG(ERROR, "Sending to ESP32 failed: %d", ret);
            esp32Failure++;
            continue;
        }
        if (os_mutex_lock(echoMutex)) {
            LOG(ERROR, "Receiving OK failed.");
            esp32Failure++;
        } else {
            esp32AccessCnt++;
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
                os_mutex_unlock(echoMutex);
            }
        } else {
            LOG(ERROR, "!(intr_raw & HOST_SLC0_RX_NEW_PACKET_INT_ST)");
        }
    }
}
#endif // ESP32_THREAD_PRESENT

#if CAN_THREAD_PRESENT
uint32_t canFailure = 0;
uint32_t canAccessCnt = 0, preCanAccessCnt = 0;
os_thread_return_t canThread(void *param) {
    pinMode(CAN_CS, OUTPUT);
    digitalWrite(CAN_CS, HIGH);
    pinMode(CAN_PWR, OUTPUT);
    digitalWrite(CAN_PWR, HIGH);
    pinMode(CAN_RST, OUTPUT);
    digitalWrite(CAN_RST, LOW);
    delay(100);
    digitalWrite(CAN_RST, HIGH);

    SPI1.beginTransaction(SPISettings(16*MHZ, MSBFIRST, SPI_MODE0));
    digitalWrite(CAN_CS, LOW);
    SPI1.transfer(0xC0); // CMD: Reset registers
    digitalWrite(CAN_CS, HIGH);
    SPI1.endTransaction();

    while (1) {
        SPI1.beginTransaction(SPISettings(1*MHZ, MSBFIRST, SPI_MODE0));
        digitalWrite(CAN_CS, LOW);
        SPI1.transfer(0x03); // CMD: Read
        SPI1.transfer(0x7F); // Register Address
        uint8_t canCtrlReg = SPI1.transfer(0xFF); // Value
        digitalWrite(CAN_CS, HIGH);
        SPI1.endTransaction();

        if (canCtrlReg != 0x87) {
            LOG(ERROR, "Reading MCP25625 failed.");
            canFailure++;
        } else {
            canAccessCnt++;
        }
        delay(CAN_ACCESS_PERIOD);
    }
}
#endif // CAN_THREAD_PRESENT

void setup() {
    Serial.begin(115200);
    while(!Serial.isConnected());
    delay(500);
    LOG(TRACE, "Test application started.");

    SPI1.begin();

#if IO_EXPANDER_THREAD_PRESENT
    Thread* thread1 = new Thread("ioExpanderThread", ioExpanderThread);
#endif // IO_EXPANDER_THREAD_PRESENT

#if GNSS_THREAD_PRESENT
#if GNSS_ENABLE_TX_READY
    if (!os_semaphore_create(&gnssReadySem, 1, 0)) {
        Thread* thread2 = new Thread("gnssThread", gnssThread);
    } else {
        LOG(ERROR, "Creating GNSS ready semophore failed.");
    }
#else
    Thread* thread2 = new Thread("gnssThread", gnssThread);
#endif // GNSS_ENABLE_TX_READY
#endif // GNSS_THREAD_PRESENT

#if BMI160_THREAD_PRESENT
    Thread* thread3 = new Thread("bmi160Thread", bmi160Thread);
#endif // BMI160_THREAD_PRESENT

#if ESP32_THREAD_PRESENT
    PORT_SPI.setDataMode(SPI_MODE0);

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
        if (!os_mutex_create(&echoMutex)) {
            Thread* thread4 = new Thread("esp32TxThread", esp32TxThread);
            Thread* thread5 = new Thread("esp32RxThread", esp32RxThread);
        }
    }
#endif // ESP32_THREAD_PRESENT

#if CAN_THREAD_PRESENT
    Thread* thread6 = new Thread("canThread", canThread);
#endif // CAN_THREAD_PRESENT
}

void loop() {
    delay(10000);
    Serial.println("");

#if IO_EXPANDER_THREAD_PRESENT
    if (preIoExpanderAccessCnt != ioExpanderAccessCnt) {
        preIoExpanderAccessCnt = ioExpanderAccessCnt;
    } else {
        LOG(ERROR, "IO Expander thread is probably blocked!");
        ioExpanderFailure++;
    }
    LOG(INFO, "IO Expander access: %d, failure: %d", ioExpanderAccessCnt, ioExpanderFailure);
#endif // IO_EXPANDER_THREAD_PRESENT

#if GNSS_THREAD_PRESENT
    if (preGnssAccessCnt != gnssAccessCnt) {
        preGnssAccessCnt = gnssAccessCnt;
    } else {
        LOG(ERROR, "GNSS thread is probably blocked!");
        gnssFailure++;
    }
    LOG(INFO, "GNSS access: %d, failure: %d", gnssAccessCnt, gnssFailure);
#endif // GNSS_THREAD_PRESENT

#if BMI160_THREAD_PRESENT
    if (preBmi160AccessCnt != bmi160AccessCnt) {
        preBmi160AccessCnt = bmi160AccessCnt;
    } else {
        LOG(ERROR, "BMI160 thread is probably blocked!");
        bmi160Failure++;
    }
    LOG(INFO, "BMI160 access: %d, failure: %d", bmi160AccessCnt, bmi160Failure);
#endif // BMI160_THREAD_PRESENT

#if ESP32_THREAD_PRESENT
    if (preEsp32AccessCnt != esp32AccessCnt) {
        preEsp32AccessCnt = esp32AccessCnt;
    } else {
        LOG(ERROR, "ESP32 thread is probably blocked!");
        esp32Failure++;
    }
    LOG(INFO, "ESP32 access: %d, failure: %d", esp32AccessCnt, esp32Failure);
#endif // ESP32_THREAD_PRESENT

#if CAN_THREAD_PRESENT
    if (preCanAccessCnt != canAccessCnt) {
        preCanAccessCnt = canAccessCnt;
    } else {
        LOG(ERROR, "CAN bus thread is probably blocked!");
        canFailure++;
    }
    LOG(INFO, "CAN bus access: %d, failure: %d", canAccessCnt, canFailure);
#endif // CAN_THREAD_PRESENT

    Serial.println("");
}

#endif // PLATFORM_TRACKER
