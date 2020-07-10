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

#ifndef I2C_HAL_COMPAT_H
#define I2C_HAL_COMPAT_H

typedef hal_i2c_mode_t I2C_Mode __attribute__((deprecated("Use hal_i2c_mode_t instead")));
typedef hal_i2c_interface_t HAL_I2C_Interface __attribute__((deprecated("Use hal_i2c_interface_t instead")));
typedef hal_i2c_config_t HAL_I2C_Config __attribute__((deprecated("Use hal_i2c_config_t instead")));
typedef hal_i2c_transmission_config_t HAL_I2C_Transmission_Config __attribute__((deprecated("Use hal_i2c_transmission_config_t instead")));

// Deprecated *dynalib* APIs for backwards compatibility
inline int __attribute__((deprecated("Use hal_i2c_init() instead"))) HAL_I2C_Init(HAL_I2C_Interface i2c, const HAL_I2C_Config* config) {
    return hal_i2c_init(i2c, config);
}

inline void __attribute__((deprecated("Use hal_i2c_set_speed() instead"))) HAL_I2C_Set_Speed(HAL_I2C_Interface i2c, uint32_t speed, void* reserved) {
    hal_i2c_set_speed(i2c, speed, reserved);
}

inline void __attribute__((deprecated("Use hal_i2c_enable_dma_mode() instead"))) HAL_I2C_Enable_DMA_Mode(HAL_I2C_Interface i2c, bool enable, void* reserved) {
    hal_i2c_enable_dma_mode(i2c, enable, reserved);
}

inline void __attribute__((deprecated("Use hal_i2c_stretch_clock() instead"))) HAL_I2C_Stretch_Clock(HAL_I2C_Interface i2c, bool stretch, void* reserved) {
    hal_i2c_stretch_clock(i2c, stretch, reserved);
}

inline void __attribute__((deprecated("Use hal_i2c_begin() instead"))) HAL_I2C_Begin(HAL_I2C_Interface i2c, I2C_Mode mode, uint8_t address, void* reserved) {
    hal_i2c_begin(i2c, mode, address, reserved);
}

inline void __attribute__((deprecated("Use hal_i2c_end() instead"))) HAL_I2C_End(HAL_I2C_Interface i2c, void* reserved) {
    hal_i2c_end(i2c, reserved);
}

inline uint32_t __attribute__((deprecated("Use hal_i2c_request() instead"))) HAL_I2C_Request_Data(HAL_I2C_Interface i2c, uint8_t address, uint8_t quantity, uint8_t stop, void* reserved) {
    return hal_i2c_request(i2c, address, quantity, stop, reserved);
}

inline int32_t __attribute__((deprecated("Use hal_i2c_request_ex() instead"))) HAL_I2C_Request_Data_Ex(HAL_I2C_Interface i2c, const HAL_I2C_Transmission_Config* config, void* reserved) {
    return hal_i2c_request_ex(i2c, config, reserved);
}

inline void __attribute__((deprecated("Use hal_i2c_begin_transmission() instead"))) HAL_I2C_Begin_Transmission(HAL_I2C_Interface i2c, uint8_t address, const HAL_I2C_Transmission_Config* config) {
    hal_i2c_begin_transmission(i2c, address, config);
}

inline uint8_t __attribute__((deprecated("Use hal_i2c_end_transmission() instead"))) HAL_I2C_End_Transmission(HAL_I2C_Interface i2c, uint8_t stop, void* reserved) {
    return hal_i2c_end_transmission(i2c, stop, reserved);
}

inline uint32_t __attribute__((deprecated("Use hal_i2c_write() instead"))) HAL_I2C_Write_Data(HAL_I2C_Interface i2c, uint8_t data, void* reserved) {
    return hal_i2c_write(i2c, data, reserved);
}

inline int32_t __attribute__((deprecated("Use hal_i2c_available() instead"))) HAL_I2C_Available_Data(HAL_I2C_Interface i2c, void* reserved) {
    return hal_i2c_available(i2c, reserved);
}

inline int32_t __attribute__((deprecated("Use hal_i2c_read() instead"))) HAL_I2C_Read_Data(HAL_I2C_Interface i2c, void* reserved) {
    return hal_i2c_read(i2c, reserved);
}

inline int32_t __attribute__((deprecated("Use hal_i2c_peek() instead"))) HAL_I2C_Peek_Data(HAL_I2C_Interface i2c, void* reserved) {
    return hal_i2c_peek(i2c, reserved);
}

inline void __attribute__((deprecated("Use hal_i2c_flush() instead"))) HAL_I2C_Flush_Data(HAL_I2C_Interface i2c, void* reserved) {
    hal_i2c_flush(i2c, reserved);
}

inline bool __attribute__((deprecated("Use hal_i2c_is_enabled() instead"))) HAL_I2C_Is_Enabled(HAL_I2C_Interface i2c, void* reserved) {
    return hal_i2c_is_enabled(i2c, reserved);
}

inline void __attribute__((deprecated("Use hal_i2c_set_callback_on_received() instead"))) HAL_I2C_Set_Callback_On_Receive(HAL_I2C_Interface i2c, void (*function)(int), void* reserved) {
    hal_i2c_set_callback_on_received(i2c, function, reserved);
}

inline void __attribute__((deprecated("Use hal_i2c_set_callback_on_requested() instead"))) HAL_I2C_Set_Callback_On_Request(HAL_I2C_Interface i2c, void (*function)(void), void* reserved) {
    hal_i2c_set_callback_on_requested(i2c, function, reserved);
}

inline uint8_t __attribute__((deprecated("Use hal_i2c_reset() instead"))) HAL_I2C_Reset(HAL_I2C_Interface i2c, uint32_t reserved, void* reserved1) {
    return hal_i2c_reset(i2c, reserved, reserved1);
}

inline int32_t __attribute__((deprecated("Use hal_i2c_lock() instead"))) HAL_I2C_Acquire(HAL_I2C_Interface i2c, void* reserved) {
    return hal_i2c_lock(i2c, reserved);
}

inline int32_t __attribute__((deprecated("Use hal_i2c_unlock() instead"))) HAL_I2C_Release(HAL_I2C_Interface i2c, void* reserved) {
    return hal_i2c_unlock(i2c, reserved);
}

#endif  /* I2C_HAL_COMPAT_H */
