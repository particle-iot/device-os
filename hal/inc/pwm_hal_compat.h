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

#ifndef PWM_HAL_COMPAT_H
#define PWM_HAL_COMPAT_H

// Deprecated *dynalib* APIs for backwards compatibility
inline void __attribute__((deprecated("Will be removed in 5.x! Use hal_pwm_write() instead"), always_inline))
HAL_PWM_Write(uint16_t pin, uint8_t value) {
	hal_pwm_write(pin, value);
}

inline void __attribute__((deprecated("Will be removed in 5.x! Use hal_pwm_write_ext() instead"), always_inline))
HAL_PWM_Write_Ext(uint16_t pin, uint32_t value) {
	hal_pwm_write_ext(pin, value);
}

inline void __attribute__((deprecated("Will be removed in 5.x! Use hal_pwm_write_with_frequency() instead"), always_inline))
HAL_PWM_Write_With_Frequency(uint16_t pin, uint8_t value, uint16_t frequency) {
	hal_pwm_write_with_frequency(pin, value, frequency);
}

inline void __attribute__((deprecated("Will be removed in 5.x! Use hal_pwm_write_with_frequency_ext() instead"), always_inline))
HAL_PWM_Write_With_Frequency_Ext(uint16_t pin, uint32_t value, uint32_t frequency) {
	hal_pwm_write_with_frequency_ext(pin, value, frequency);
}

inline uint16_t __attribute__((deprecated("Will be removed in 5.x! Use hal_pwm_get_frequency() instead"), always_inline))
HAL_PWM_Get_Frequency(uint16_t pin) {
	return hal_pwm_get_frequency(pin);
}

inline uint32_t __attribute__((deprecated("Will be removed in 5.x! Use hal_pwm_get_frequency_ext() instead"), always_inline))
HAL_PWM_Get_Frequency_Ext(uint16_t pin) {
	return hal_pwm_get_frequency_ext(pin);
}

inline uint16_t __attribute__((deprecated("Will be removed in 5.x! Use hal_pwm_get_analog_value() instead"), always_inline))
HAL_PWM_Get_AnalogValue(uint16_t pin) {
	return hal_pwm_get_analog_value(pin);
}

inline uint32_t __attribute__((deprecated("Will be removed in 5.x! Use hal_pwm_get_analog_value_ext() instead"), always_inline))
HAL_PWM_Get_AnalogValue_Ext(uint16_t pin) {
	return hal_pwm_get_analog_value_ext(pin);
}

inline uint32_t __attribute__((deprecated("Will be removed in 5.x! Use hal_pwm_get_max_frequency() instead"), always_inline))
HAL_PWM_Get_Max_Frequency(uint16_t pin) {
	return hal_pwm_get_max_frequency(pin);
}

inline uint8_t __attribute__((deprecated("Will be removed in 5.x! Use hal_pwm_get_resolution() instead"), always_inline))
HAL_PWM_Get_Resolution(uint16_t pin) {
	return hal_pwm_get_resolution(pin);
}

inline void __attribute__((deprecated("Will be removed in 5.x! Use hal_pwm_set_resolution() instead"), always_inline))
HAL_PWM_Set_Resolution(uint16_t pin, uint8_t resolution) {
	hal_pwm_set_resolution(pin, resolution);
}

#endif  /* PWM_HAL_COMPAT_H */
