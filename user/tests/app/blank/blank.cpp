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
#include "at_dtm_hal.h"

SYSTEM_MODE(MANUAL);

SerialLogHandler l(115200, LOG_LEVEL_ALL);

/* executes once at startup */
void setup() {
    while (!Serial.isConnected());
    delay(1s);
    Log.info("Application started.");

    hal_at_dtm_interface_config_t config = {};
    config.interface = HAL_AT_DTM_INTERFACE_UART;
    config.index = HAL_USART_SERIAL1;
    hal_at_dtm_init(HAL_AT_DTM_TYPE_BLE, &config, nullptr);

    delay(3s);
    config.params.baudrate = 115200;
    hal_at_dtm_init(HAL_AT_DTM_TYPE_WIFI, &config, nullptr);
}

/* executes continuously after setup() runs */
void loop() {

}
