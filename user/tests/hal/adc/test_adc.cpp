/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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
 #include "adc_hal.h"

SYSTEM_MODE(SEMI_AUTOMATIC);

Serial1LogHandler logHandler(115200);

/* executes once at startup */
void setup() {
    hal_adc_dma_init();
}

/* executes continuously after setup() runs */
void loop() {
    Log.info("A0: %d, A1: %d, A2: %d, A3: %d, A4: %d, A5: %d", 
             hal_adc_read(A0),
             hal_adc_read(A1),
             hal_adc_read(A2),
             hal_adc_read(A3),
             hal_adc_read(A4),
             hal_adc_read(A5));
    delay(500);
}
