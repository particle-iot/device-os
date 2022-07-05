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

#ifndef ADC_HAL_COMPAT_H
#define ADC_HAL_COMPAT_H

// Deprecated *dynalib* APIs for backwards compatibility
inline void __attribute__((deprecated("Will be removed in 5.x! Use hal_adc_set_sample_time() instead"), always_inline))
HAL_ADC_Set_Sample_Time(uint8_t sample_time) {
    hal_adc_set_sample_time(sample_time);
}

inline int32_t __attribute__((deprecated("Will be removed in 5.x! Use hal_adc_read() instead"), always_inline))
HAL_ADC_Read(hal_pin_t pin) {
    return hal_adc_read(pin);
}

#endif  /* ADC_HAL_COMPAT_H */
