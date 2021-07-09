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

#include "adc_hal.h"
#include "pinmap_impl.h"
#include "check.h"

static volatile hal_adc_state_t adcState = HAL_ADC_STATE_DISABLED;

void hal_adc_set_sample_time(uint8_t sample_time) {
    // deprecated
}

/*
 * @brief Read the analog value of a pin.
 * Should return a 16-bit value, 0-65536 (0 = LOW, 65536 = HIGH)
 * Note: ADC is 12-bit. Currently it returns 0-4096
 */
int32_t hal_adc_read(uint16_t pin) {
    return 0;
}

/*
 * @brief Initialize the ADC peripheral.
 */
void hal_adc_dma_init() {

}

/*
 * @brief Uninitialize the ADC peripheral.
 */
int hal_adc_dma_uninit(void* reserved) {
    return SYSTEM_ERROR_NONE;
}

/*
 * @brief ADC peripheral enters sleep mode
 */
int hal_adc_sleep(bool sleep, void* reserved) {
    return SYSTEM_ERROR_NONE;
}

