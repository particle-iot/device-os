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

#ifndef __USART_HAL_COMPAT_H
#define __USART_HAL_COMPAT_H

// Deprecated *dynalib* APIs for backwards compatibility
inline void __attribute__((deprecated("Use hal_usart_init() instead"))) HAL_USART_Init(HAL_USART_Serial serial, Ring_Buffer *rx_buffer, Ring_Buffer *tx_buffer) {
    hal_usart_init(serial, rx_buffer, tx_buffer);
}

inline void __attribute__((deprecated("Use hal_usart_begin() instead"))) HAL_USART_Begin(HAL_USART_Serial serial, uint32_t baud) {
    hal_usart_begin(serial, baud);
}

inline void __attribute__((deprecated("Use hal_usart_end() instead"))) HAL_USART_End(HAL_USART_Serial serial) {
    hal_usart_end(serial);
}

inline uint32_t __attribute__((deprecated("Use hal_usart_write() instead"))) HAL_USART_Write_Data(HAL_USART_Serial serial, uint8_t data) {
    return hal_usart_write(serial, data);
}

inline int32_t __attribute__((deprecated("Use hal_usart_available_data_for_write() instead"))) HAL_USART_Available_Data_For_Write(HAL_USART_Serial serial) {
    return hal_usart_available_data_for_write(serial);
}

inline int32_t __attribute__((deprecated("Use hal_usart_available() instead"))) HAL_USART_Available_Data(HAL_USART_Serial serial) {
    return hal_usart_available(serial);
}

inline int32_t __attribute__((deprecated("Use hal_usart_read() instead"))) HAL_USART_Read_Data(HAL_USART_Serial serial) {
    return hal_usart_read(serial);
}

inline int32_t __attribute__((deprecated("Use hal_usart_peek() instead"))) HAL_USART_Peek_Data(HAL_USART_Serial serial) {
    return hal_usart_peek(serial);
}

inline void __attribute__((deprecated("Use hal_usart_flush() instead"))) HAL_USART_Flush_Data(HAL_USART_Serial serial) {
    hal_usart_flush(serial);
}

inline bool __attribute__((deprecated("Use hal_usart_is_enabled() instead"))) HAL_USART_Is_Enabled(HAL_USART_Serial serial) {
    return hal_usart_is_enabled(serial);
}

inline void __attribute__((deprecated("Use hal_usart_half_duplex() instead"))) HAL_USART_Half_Duplex(HAL_USART_Serial serial, bool Enable) {
    hal_usart_half_duplex(serial, Enable);
}

inline void __attribute__((deprecated("Use hal_usart_begin_config() instead"))) HAL_USART_BeginConfig(HAL_USART_Serial serial, uint32_t baud, uint32_t config, void* reserved) {
    hal_usart_begin_config(serial, baud, config, reserved);
}

inline uint32_t __attribute__((deprecated("Use hal_usart_write_nine_bits() instead"))) HAL_USART_Write_NineBitData(HAL_USART_Serial serial, uint16_t data) {
    return hal_usart_write_nine_bits(serial, data);
}

inline void __attribute__((deprecated("Use hal_usart_send_break() instead"))) HAL_USART_Send_Break(HAL_USART_Serial serial, void* reserved) {
    hal_usart_send_break(serial, reserved);
}

inline uint8_t __attribute__((deprecated("Use hal_usart_break_detected() instead"))) HAL_USART_Break_Detected(HAL_USART_Serial serial) {
    return hal_usart_break_detected(serial);
}

#endif  /* __USART_HAL_COMPAT_H */
