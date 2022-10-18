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
#ifndef I2C_HAL_H
#define I2C_HAL_H

/* Includes ------------------------------------------------------------------*/
#include <stddef.h>
#include "pinmap_hal.h"
#include "platforms.h"
#include "system_tick_hal.h"
#include "system_error.h"

/* Exported types ------------------------------------------------------------*/
typedef enum hal_i2c_mode_t {
    I2C_MODE_MASTER = 0,
    I2C_MODE_SLAVE = 1
} hal_i2c_mode_t;

/* Exported types ------------------------------------------------------------*/
typedef enum hal_i2c_interface_t {
    HAL_I2C_INTERFACE1 = 0,
    HAL_I2C_INTERFACE2 = 1,
    HAL_I2C_INTERFACE3 = 2
} hal_i2c_interface_t;

/*! I2c Config Structure Version */
typedef enum hal_i2c_config_version_t {
    HAL_I2C_CONFIG_VERSION_1 = 0,
    HAL_I2C_CONFIG_VERSION_2 = 1,
    HAL_I2C_CONFIG_VERSION_LATEST = HAL_I2C_CONFIG_VERSION_2,
} hal_i2c_config_version_t;

typedef struct hal_i2c_config_t {
    uint16_t size;
    uint16_t version;
    uint8_t* rx_buffer;
    uint32_t rx_buffer_size;
    uint8_t* tx_buffer;
    uint32_t tx_buffer_size;
    uint32_t flags;
} hal_i2c_config_t;

typedef enum hal_i2c_config_flag_t {
    HAL_I2C_CONFIG_FLAG_NONE     = 0x00,
    HAL_I2C_CONFIG_FLAG_FREEABLE = 0x01
} hal_i2c_config_flag_t;

typedef enum hal_i2c_transmission_flag_t {
    HAL_I2C_TRANSMISSION_FLAG_NONE = 0x00,
    HAL_I2C_TRANSMISSION_FLAG_STOP = 0x01
} hal_i2c_transmission_flag_t;

typedef struct hal_i2c_transmission_config_t {
    uint16_t size;
    uint16_t version;
    uint8_t address;
    uint8_t reserved[3];
    uint32_t quantity;
    system_tick_t timeout_ms;
    uint32_t flags;
} hal_i2c_transmission_config_t;

typedef enum hal_i2c_state_t {
    HAL_I2C_STATE_DISABLED,
    HAL_I2C_STATE_ENABLED,
    HAL_I2C_STATE_SUSPENDED
} hal_i2c_state_t;

/* Exported macros -----------------------------------------------------------*/
#define CLOCK_SPEED_100KHZ         (uint32_t)100000
#define CLOCK_SPEED_400KHZ         (uint32_t)400000
#define HAL_I2C_DEFAULT_TIMEOUT_MS (100)
#define I2C_BUFFER_LENGTH          (uint32_t)HAL_PLATFORM_I2C_BUFFER_SIZE_DEFAULT

/* Exported functions --------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

int hal_i2c_init(hal_i2c_interface_t i2c, const hal_i2c_config_t* config);
void hal_i2c_set_speed(hal_i2c_interface_t i2c, uint32_t speed, void* reserved);
void hal_i2c_enable_dma_mode(hal_i2c_interface_t i2c, bool enable, void* reserved);
void hal_i2c_stretch_clock(hal_i2c_interface_t i2c, bool stretch, void* reserved);
void hal_i2c_begin(hal_i2c_interface_t i2c, hal_i2c_mode_t mode, uint8_t address, void* reserved);
void hal_i2c_end(hal_i2c_interface_t i2c, void* reserved);
uint32_t hal_i2c_request(hal_i2c_interface_t i2c, uint8_t address, uint8_t quantity, uint8_t stop, void* reserved);
int32_t hal_i2c_request_ex(hal_i2c_interface_t i2c, const hal_i2c_transmission_config_t* config, void* reserved);
void hal_i2c_begin_transmission(hal_i2c_interface_t i2c, uint8_t address, const hal_i2c_transmission_config_t* config);
uint8_t hal_i2c_end_transmission(hal_i2c_interface_t i2c, uint8_t stop, void* reserved);
int hal_i2c_end_transmission_ext(hal_i2c_interface_t i2c, uint8_t stop, void* reserved);
uint32_t hal_i2c_write(hal_i2c_interface_t i2c, uint8_t data, void* reserved);
int32_t hal_i2c_available(hal_i2c_interface_t i2c, void* reserved);
int32_t hal_i2c_read(hal_i2c_interface_t i2c, void* reserved);
int32_t hal_i2c_peek(hal_i2c_interface_t i2c, void* reserved);
void hal_i2c_flush(hal_i2c_interface_t i2c, void* reserved);
bool hal_i2c_is_enabled(hal_i2c_interface_t i2c, void* reserved);
void hal_i2c_set_callback_on_received(hal_i2c_interface_t i2c, void (*function)(int), void* reserved);
void hal_i2c_set_callback_on_requested(hal_i2c_interface_t i2c, void (*function)(void), void* reserved);
int hal_i2c_reset(hal_i2c_interface_t i2c, uint32_t reserved, void* reserve1);
int hal_i2c_sleep(hal_i2c_interface_t i2c, bool sleep, void* reserved);
int32_t hal_i2c_lock(hal_i2c_interface_t i2c, void* reserved);
int32_t hal_i2c_unlock(hal_i2c_interface_t i2c, void* reserved);

void hal_i2c_set_speed_deprecated(uint32_t speed);
void hal_i2c_enable_dma_mode_deprecated(bool enable);
void hal_i2c_stretch_clock_deprecated(bool stretch);
void hal_i2c_begin_deprecated(hal_i2c_mode_t mode, uint8_t address);
void hal_i2c_end_deprecated();
uint32_t hal_i2c_request_deprecated(uint8_t address, uint8_t quantity, uint8_t stop);
void hal_i2c_begin_transmission_deprecated(uint8_t address);
uint8_t hal_i2c_end_transmission_deprecated(uint8_t stop);
uint32_t hal_i2c_write_deprecated(uint8_t data);
int32_t hal_i2c_available_deprecated(void);
int32_t hal_i2c_read_deprecated(void);
int32_t hal_i2c_peek_deprecated(void);
void hal_i2c_flush_deprecated(void);
bool hal_i2c_is_enabled_deprecated(void);
void hal_i2c_set_callback_on_received_deprecated(void (*function)(int));
void hal_i2c_set_callback_on_requested_deprecated(void (*function)(void));

inline uint8_t __attribute__((always_inline))
hal_i2c_compat_error_from(int system_error) {
    // TODO: deprecate it
    // As per: https://docs.particle.io/reference/device-os/firmware/#endtransmission-
    switch (system_error) {
        case SYSTEM_ERROR_NONE: return 0;
        case SYSTEM_ERROR_I2C_BUS_BUSY: return 1;
        case SYSTEM_ERROR_I2C_ARBITRATION_FAILED: return 2;
        case SYSTEM_ERROR_I2C_TX_ADDR_TIMEOUT: return 3;
        case SYSTEM_ERROR_I2C_FILL_DATA_TIMEOUT: return 4;
        case SYSTEM_ERROR_I2C_TX_DATA_TIMEOUT: return 5;
        case SYSTEM_ERROR_I2C_STOP_TIMEOUT: return 6;
        default: return 7;
    }
}

#include "i2c_hal_compat.h"

#ifdef __cplusplus
}
#endif

#endif  /* I2C_HAL_H */
