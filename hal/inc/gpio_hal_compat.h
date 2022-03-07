/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
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

#pragma once

// Deprecated *dynalib* APIs for backwards compatibility
inline void __attribute__((deprecated("Use hal_gpio_mode() instead"), always_inline))
HAL_Pin_Mode(pin_t pin, PinMode mode) {
    hal_gpio_mode(pin, mode);
}

inline int __attribute__((deprecated("Use hal_gpio_configure() instead"), always_inline))
HAL_Pin_Configure(pin_t pin, const hal_gpio_config_t* conf, void* reserved) {
    return hal_gpio_configure(pin, conf, reserved);
}

inline PinMode __attribute__((deprecated("Use hal_gpio_get_mode() instead"), always_inline))
HAL_Get_Pin_Mode(pin_t pin) {
    return hal_gpio_get_mode(pin);
}

inline void __attribute__((deprecated("Use hal_gpio_write() instead"), always_inline))
HAL_GPIO_Write(pin_t pin, uint8_t value) {
    hal_gpio_write(pin, value);
}

inline int32_t __attribute__((deprecated("Use hal_gpio_read() instead"), always_inline))
HAL_GPIO_Read(pin_t pin) {
    return hal_gpio_read(pin);
}

inline uint32_t __attribute__((deprecated("Use hal_gpio_pulse_in() instead"), always_inline))
HAL_Pulse_In(pin_t pin, uint16_t value) {
    return hal_gpio_pulse_in(pin, value);
}

