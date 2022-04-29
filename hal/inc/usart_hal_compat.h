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

#ifndef USART_HAL_COMPAT_H
#define USART_HAL_COMPAT_H

typedef hal_usart_ring_buffer_t Ring_Buffer __attribute__((deprecated("Will be removed in 5.x! Use hal_usart_ring_buffer_t instead")));
typedef hal_usart_interface_t HAL_USART_Serial __attribute__((deprecated("Will be removed in 5.x! Use hal_usart_interface_t instead")));
typedef hal_usart_buffer_config_t HAL_USART_Buffer_Config __attribute__((deprecated("Will be removed in 5.x! Use hal_usart_buffer_config_t instead")));

// Deprecated *dynalib* APIs for backwards compatibility
inline void __attribute__((deprecated("Will be removed in 5.x! Use hal_usart_init() instead"), always_inline))
HAL_USART_Init(hal_usart_interface_t serial, hal_usart_ring_buffer_t *rx_buffer, hal_usart_ring_buffer_t *tx_buffer) {
    hal_usart_init(serial, rx_buffer, tx_buffer);
}

inline void __attribute__((deprecated("Will be removed in 5.x! Use hal_usart_begin() instead"), always_inline))
HAL_USART_Begin(hal_usart_interface_t serial, uint32_t baud) {
    hal_usart_begin(serial, baud);
}

inline void __attribute__((deprecated("Will be removed in 5.x! Use hal_usart_end() instead"), always_inline))
HAL_USART_End(hal_usart_interface_t serial) {
    hal_usart_end(serial);
}

inline uint32_t __attribute__((deprecated("Will be removed in 5.x! Use hal_usart_write() instead"), always_inline))
HAL_USART_Write_Data(hal_usart_interface_t serial, uint8_t data) {
    return hal_usart_write(serial, data);
}

inline int32_t __attribute__((deprecated("Will be removed in 5.x! Use hal_usart_available_data_for_write() instead"), always_inline))
HAL_USART_Available_Data_For_Write(hal_usart_interface_t serial) {
    return hal_usart_available_data_for_write(serial);
}

inline int32_t __attribute__((deprecated("Will be removed in 5.x! Use hal_usart_available() instead"), always_inline))
HAL_USART_Available_Data(hal_usart_interface_t serial) {
    return hal_usart_available(serial);
}

inline int32_t __attribute__((deprecated("Will be removed in 5.x! Use hal_usart_read() instead"), always_inline))
HAL_USART_Read_Data(hal_usart_interface_t serial) {
    return hal_usart_read(serial);
}

inline int32_t __attribute__((deprecated("Will be removed in 5.x! Use hal_usart_peek() instead"), always_inline))
HAL_USART_Peek_Data(hal_usart_interface_t serial) {
    return hal_usart_peek(serial);
}

inline void __attribute__((deprecated("Will be removed in 5.x! Use hal_usart_flush() instead"), always_inline))
HAL_USART_Flush_Data(hal_usart_interface_t serial) {
    hal_usart_flush(serial);
}

inline bool __attribute__((deprecated("Will be removed in 5.x! Use hal_usart_is_enabled() instead"), always_inline))
HAL_USART_Is_Enabled(hal_usart_interface_t serial) {
    return hal_usart_is_enabled(serial);
}

inline void __attribute__((deprecated("Will be removed in 5.x! Use hal_usart_half_duplex() instead"), always_inline))
HAL_USART_Half_Duplex(hal_usart_interface_t serial, bool Enable) {
    hal_usart_half_duplex(serial, Enable);
}

inline void __attribute__((deprecated("Will be removed in 5.x! Use hal_usart_begin_config() instead"), always_inline))
HAL_USART_BeginConfig(hal_usart_interface_t serial, uint32_t baud, uint32_t config, void* reserved) {
    hal_usart_begin_config(serial, baud, config, reserved);
}

inline uint32_t __attribute__((deprecated("Will be removed in 5.x! Use hal_usart_write_nine_bits() instead"), always_inline))
HAL_USART_Write_NineBitData(hal_usart_interface_t serial, uint16_t data) {
    return hal_usart_write_nine_bits(serial, data);
}

inline void __attribute__((deprecated("Will be removed in 5.x! Use hal_usart_send_break() instead"), always_inline))
HAL_USART_Send_Break(hal_usart_interface_t serial, void* reserved) {
    hal_usart_send_break(serial, reserved);
}

inline uint8_t __attribute__((deprecated("Will be removed in 5.x! Use hal_usart_break_detected() instead"), always_inline))
HAL_USART_Break_Detected(hal_usart_interface_t serial) {
    return hal_usart_break_detected(serial);
}

#endif  /* USART_HAL_COMPAT_H */
