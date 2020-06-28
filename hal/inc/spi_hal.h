/******************************************************************************
 * @file    spi_hal.h
 * @author  Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    12-Sept-2014
 * @brief
 ******************************************************************************/
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SPI_HAL_H
#define __SPI_HAL_H

/* Includes ------------------------------------------------------------------*/
#include "pinmap_hal.h"
#include "system_tick_hal.h"

/* Exported types ------------------------------------------------------------*/
typedef enum hal_spi_interface_t {
    HAL_SPI_INTERFACE1 = 0,    //maps to SPI1 (pins: A3, A4, A5)
    HAL_SPI_INTERFACE2 = 1     //maps to SPI3 (pins: D4, D3, D2)
#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
   ,HAL_SPI_INTERFACE3 = 2     //maps to SPI3 (pins: C3, C2, C1)
#endif
} hal_spi_interface_t;
typedef hal_spi_interface_t HAL_SPI_Interface; // For backwards compatibility

typedef enum hal_spi_mode_t {
    SPI_MODE_MASTER = 0,
    SPI_MODE_SLAVE = 1
} hal_spi_mode_t;
typedef hal_spi_mode_t SPI_Mode; // For backwards compatibility

typedef enum hal_spi_state_t {
    HAL_SPI_STATE_UNKNOWN,
    HAL_SPI_STATE_ENABLED,
    HAL_SPI_STATE_DISABLED,
    HAL_SPI_STATE_SUSPENDED
} hal_spi_state_t;

typedef void (*hal_spi_dma_user_callback)(void);
typedef void (*hal_spi_select_user_callback)(uint8_t);

/* Exported macros -----------------------------------------------------------*/
#define SPI_MODE0               0x00
#define SPI_MODE1               0x01
#define SPI_MODE2               0x02
#define SPI_MODE3               0x03

#define SPI_CLOCK_DIV2          0x00
#define SPI_CLOCK_DIV4          0x08
#define SPI_CLOCK_DIV8          0x10
#define SPI_CLOCK_DIV16         0x18
#define SPI_CLOCK_DIV32         0x20
#define SPI_CLOCK_DIV64         0x28
#define SPI_CLOCK_DIV128        0x30
#define SPI_CLOCK_DIV256        0x38

#define SPI_DEFAULT_SS          ((uint16_t)(-1))

#define LSBFIRST                0
#define MSBFIRST                1

#ifdef __cplusplus
extern "C" {
#endif

typedef enum hal_spi_info_version_t {
    HAL_SPI_INFO_VERSION_1 = 11,
    HAL_SPI_INFO_VERSION_2 = 12
} hal_spi_info_version_t;

#define HAL_SPI_INFO_VERSION HAL_SPI_INFO_VERSION_2

typedef struct hal_spi_info_t {
    uint16_t version;
    uint32_t system_clock;      // the clock speed that is divided when setting a divider
    uint8_t default_settings;
    uint8_t enabled;
    hal_spi_mode_t mode;
    uint32_t clock;
    uint8_t bit_order;
    uint8_t data_mode;
    pin_t ss_pin;
} hal_spi_info_t;

typedef struct hal_spi_transfer_status_t {
    uint8_t version;
    uint32_t configured_transfer_length;
    uint32_t transfer_length;
    uint8_t transfer_ongoing    : 1;
    uint8_t ss_state            : 1;
} hal_spi_transfer_status_t;
typedef hal_spi_transfer_status_t HAL_SPI_TransferStatus; // For backwards compatibility

typedef struct hal_spi_acquire_config_t {
    uint16_t size;
    uint16_t version;
    system_tick_t timeout;
} hal_spi_acquire_config_t;
typedef hal_spi_acquire_config_t HAL_SPI_AcquireConfig;

void hal_spi_init(hal_spi_interface_t spi);
void hal_spi_begin(hal_spi_interface_t spi, uint16_t pin);
void hal_spi_begin_ext(hal_spi_interface_t spi, hal_spi_mode_t mode, uint16_t pin, void* reserved);
void hal_spi_end(hal_spi_interface_t spi);
void hal_spi_set_bit_order(hal_spi_interface_t spi, uint8_t order);
void hal_spi_set_data_mode(hal_spi_interface_t spi, uint8_t mode);
void hal_spi_set_clock_divider(hal_spi_interface_t spi, uint8_t rate);
// hal_spi_set_bit_order, hal_spi_set_data_mode and hal_spi_set_clock_divider in one go
// to avoid having to reconfigure SPI peripheral 3 times
int32_t hal_spi_set_settings(hal_spi_interface_t spi, uint8_t set_default, uint8_t clockdiv, uint8_t order, uint8_t mode, void* reserved);
uint16_t hal_spi_transfer(hal_spi_interface_t spi, uint16_t data);
void hal_spi_transfer_dma(hal_spi_interface_t spi, void* tx_buffer, void* rx_buffer, uint32_t length, hal_spi_dma_user_callback userCallback);
void hal_spi_transfer_dma_cancel(hal_spi_interface_t spi);
int32_t hal_spi_transfer_dma_status(hal_spi_interface_t spi, hal_spi_transfer_status_t* st);
bool hal_spi_is_enabled_deprecated();
bool hal_spi_is_enabled(hal_spi_interface_t spi);
void hal_spi_info(hal_spi_interface_t spi, hal_spi_info_t* info, void* reserved);
void hal_spi_set_callback_on_selected(hal_spi_interface_t spi, hal_spi_select_user_callback cb, void* reserved);
int hal_spi_sleep(hal_spi_interface_t spi, bool sleep, void* reserved);

#if HAL_PLATFORM_SPI_HAL_THREAD_SAFETY
int32_t hal_spi_acquire(hal_spi_interface_t spi, const hal_spi_acquire_config_t* conf);
int32_t hal_spi_release(hal_spi_interface_t spi, void* reserved);
#endif // HAL_PLATFORM_SPI_HAL_THREAD_SAFETY

#ifdef __cplusplus
}
#endif

#endif  /* __SPI_HAL_H */
