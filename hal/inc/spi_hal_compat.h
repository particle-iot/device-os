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

#ifndef SPI_HAL_COMPAT_H
#define SPI_HAL_COMPAT_H

typedef hal_spi_interface_t HAL_SPI_Interface __attribute__((deprecated("Will be removed in 5.x! Use hal_spi_interface_t instead")));
typedef hal_spi_mode_t SPI_Mode __attribute__((deprecated("Will be removed in 5.x! Use hal_spi_mode_t instead")));
typedef hal_spi_transfer_status_t HAL_SPI_TransferStatus __attribute__((deprecated("Will be removed in 5.x! Use hal_spi_transfer_status_t instead")));
typedef hal_spi_acquire_config_t HAL_SPI_AcquireConfig __attribute__((deprecated("Will be removed in 5.x! Use hal_spi_acquire_config_t instead")));

typedef hal_spi_dma_user_callback HAL_SPI_DMA_UserCallback;
typedef hal_spi_select_user_callback HAL_SPI_Select_UserCallback;

// Deprecated *dynalib* APIs for backwards compatibility
inline void __attribute__((deprecated("Will be removed in 5.x! Use hal_spi_init() instead"), always_inline))
HAL_SPI_Init(hal_spi_interface_t spi) {
    hal_spi_init(spi);
}

inline void __attribute__((deprecated("Will be removed in 5.x! Use hal_spi_begin() instead"), always_inline))
HAL_SPI_Begin(hal_spi_interface_t spi, uint16_t pin) {
    hal_spi_begin(spi, pin);
}

inline void __attribute__((deprecated("Will be removed in 5.x! Use hal_spi_begin_ext() instead"), always_inline))
HAL_SPI_Begin_Ext(hal_spi_interface_t spi, hal_spi_mode_t mode, uint16_t pin, hal_spi_config_t* spi_config) {
    hal_spi_begin_ext(spi, mode, pin, spi_config);
}

inline void __attribute__((deprecated("Will be removed in 5.x! Use hal_spi_end() instead"), always_inline))
HAL_SPI_End(hal_spi_interface_t spi) {
    hal_spi_end(spi);
}

inline void __attribute__((deprecated("Will be removed in 5.x! Use hal_spi_set_bit_order() instead"), always_inline))
HAL_SPI_Set_Bit_Order(hal_spi_interface_t spi, uint8_t order) {
    hal_spi_set_bit_order(spi, order);
}

inline void __attribute__((deprecated("Will be removed in 5.x! Use hal_spi_set_data_mode() instead"), always_inline))
HAL_SPI_Set_Data_Mode(hal_spi_interface_t spi, uint8_t mode) {
    hal_spi_set_data_mode(spi, mode);
}

inline void __attribute__((deprecated("Will be removed in 5.x! Use hal_spi_set_clock_divider() instead"), always_inline))
HAL_SPI_Set_Clock_Divider(hal_spi_interface_t spi, uint8_t rate) {
    hal_spi_set_clock_divider(spi, rate);
}

inline uint16_t __attribute__((deprecated("Will be removed in 5.x! Use hal_spi_transfer() instead"), always_inline))
HAL_SPI_Send_Receive_Data(hal_spi_interface_t spi, uint16_t data) {
    return hal_spi_transfer(spi, data);
}

inline void __attribute__((deprecated("Will be removed in 5.x! Use hal_spi_transfer_dma() instead"), always_inline))
HAL_SPI_DMA_Transfer(hal_spi_interface_t spi, void* tx_buffer, void* rx_buffer, uint32_t length, HAL_SPI_DMA_UserCallback userCallback) {
    hal_spi_transfer_dma(spi, tx_buffer, rx_buffer, length, userCallback);
}

inline bool __attribute__((deprecated("Will be removed in 5.x! Use hal_spi_is_enabled_deprecated() instead"), always_inline))
HAL_SPI_Is_Enabled_Old() {
    return hal_spi_is_enabled_deprecated();
}

inline bool __attribute__((deprecated("Will be removed in 5.x! Use hal_spi_is_enabled() instead"), always_inline))
HAL_SPI_Is_Enabled(hal_spi_interface_t spi) {
    return hal_spi_is_enabled(spi);
}

inline void __attribute__((deprecated("Will be removed in 5.x! Use hal_spi_info() instead"), always_inline))
HAL_SPI_Info(hal_spi_interface_t spi, hal_spi_info_t* info, void* reserved) {
    hal_spi_info(spi, info, reserved);
}

inline void __attribute__((deprecated("Will be removed in 5.x! Use hal_spi_set_callback_on_selected() instead"), always_inline))
HAL_SPI_Set_Callback_On_Select(hal_spi_interface_t spi, HAL_SPI_Select_UserCallback cb, void* reserved) {
    hal_spi_set_callback_on_selected(spi, cb, reserved);
}

inline void __attribute__((deprecated("Will be removed in 5.x! Use hal_spi_transfer_dma_cancel() instead"), always_inline))
HAL_SPI_DMA_Transfer_Cancel(hal_spi_interface_t spi) {
    hal_spi_transfer_dma_cancel(spi);
}

inline int32_t __attribute__((deprecated("Will be removed in 5.x! Use hal_spi_transfer_dma_status() instead"), always_inline))
HAL_SPI_DMA_Transfer_Status(hal_spi_interface_t spi, hal_spi_transfer_status_t* st) {
    return hal_spi_transfer_dma_status(spi, st);
}

inline int32_t __attribute__((deprecated("Will be removed in 5.x! Use hal_spi_set_settings() instead"), always_inline))
HAL_SPI_Set_Settings(hal_spi_interface_t spi, uint8_t set_default, uint8_t clockdiv, uint8_t order, uint8_t mode, void* reserved) {
    return hal_spi_set_settings(spi, set_default, clockdiv, order, mode, reserved);
}

#if HAL_PLATFORM_SPI_HAL_THREAD_SAFETY

inline int32_t __attribute__((deprecated("Will be removed in 5.x! Use hal_spi_acquire() instead"), always_inline))
HAL_SPI_Acquire(hal_spi_interface_t spi, const hal_spi_acquire_config_t* conf) { 
    return hal_spi_acquire(spi, conf);
}

inline int32_t __attribute__((deprecated("Will be removed in 5.x! Use hal_spi_release() instead"), always_inline))
HAL_SPI_Release(hal_spi_interface_t spi, void* reserved) {
    return hal_spi_release(spi, reserved);
}

#endif // HAL_PLATFORM_SPI_HAL_THREAD_SAFETY

#endif  /* SPI_HAL_COMPAT_H */
