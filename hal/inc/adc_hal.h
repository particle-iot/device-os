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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ADC_HAL_H
#define ADC_HAL_H

#include "pinmap_hal.h"

typedef enum hal_adc_state_t {
    HAL_ADC_STATE_DISABLED,
    HAL_ADC_STATE_ENABLED,
    HAL_ADC_STATE_SUSPENDED
} hal_adc_state_t;

typedef enum hal_adc_reference_t {
    HAL_ADC_REFERENCE_DEFAULT = 0,
    HAL_ADC_REFERENCE_INTERNAL = 1,
    HAL_ADC_REFERENCE_VCC = 2
} hal_adc_reference_t;

#ifdef __cplusplus
extern "C" {
#endif

void hal_adc_set_sample_time(uint8_t sample_time);
int32_t hal_adc_read(hal_pin_t pin);
void hal_adc_dma_init();
int hal_adc_calibrate(uint32_t reserved, void* reserved1);
int hal_adc_sleep(bool sleep, void* reserved);
int hal_adc_set_reference(uint32_t reference, void* reserved);
int hal_adc_get_reference(void* reserved);

#include "adc_hal_compat.h"

#ifdef __cplusplus
}
#endif

#endif  /* ADC_HAL_H */
