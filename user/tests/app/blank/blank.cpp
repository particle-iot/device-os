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
#define PARTICLE_USE_UNSTABLE_API
#include "storage_hal.h"

SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler(LOG_LEVEL_ALL);

#define QUECTEL_EG800Q_NA       0x6B
#define QUECTEL_EG800Q_EU       0x6C

void burnNcpId(uint8_t ncpId) {
    const uintptr_t NCP_ID_OTP_ADDRESS = 0x00000020;
    uint8_t readNcpId = 0xFF;
    hal_storage_read(HAL_STORAGE_ID_OTP, NCP_ID_OTP_ADDRESS, &readNcpId, sizeof(readNcpId));
    Serial.printf("Read current NCP ID: 0x%x\r\n", readNcpId);
    if (readNcpId == ncpId) {
        return;
    }
    hal_storage_write(HAL_STORAGE_ID_OTP, NCP_ID_OTP_ADDRESS, &ncpId, sizeof(ncpId));
    hal_storage_read(HAL_STORAGE_ID_OTP, NCP_ID_OTP_ADDRESS, &readNcpId, sizeof(readNcpId));
    Serial.printf("Read new NCP ID: 0x%x\r\n", readNcpId);
    Serial.println("Rebooting to apply new NCP ID...");
    delay(2s);
    System.reset();
}

/* executes once at startup */
void setup() {
    // while (!Serial.isConnected()) {
    //     delay(10);
    // }

    delay(5s);

    burnNcpId(QUECTEL_EG800Q_NA);

    Cellular.on();
    delay(5s);

    Particle.connect();
}

/* executes continuously after setup() runs */
void loop() {
    // static system_tick_t now = millis();
    // if (millis() - now > 30000) {
    //     now = millis();
    //     Cellular.command("AT+QWIFISCAN");
    // }
}
