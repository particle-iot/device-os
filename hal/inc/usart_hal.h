/******************************************************************************
 * @file    usart_hal.h
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
 * MERCHAN'TABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef USART_HAL_H
#define USART_HAL_H

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include "platforms.h"
#include "hal_platform.h"

/* Includes ------------------------------------------------------------------*/
// #include "pinmap_hal.h"

/* Exported defines ----------------------------------------------------------*/

#define SERIAL_BUFFER_SIZE      HAL_PLATFORM_USART_DEFAULT_BUFFER_SIZE

// Pre-defined USART configurations
#define SERIAL_7E1 (uint32_t)(SERIAL_DATA_BITS_7 | SERIAL_PARITY_EVEN | SERIAL_STOP_BITS_1)
#define SERIAL_7E2 (uint32_t)(SERIAL_DATA_BITS_7 | SERIAL_PARITY_EVEN | SERIAL_STOP_BITS_2)
#define SERIAL_7O1 (uint32_t)(SERIAL_DATA_BITS_7 | SERIAL_PARITY_ODD  | SERIAL_STOP_BITS_1)
#define SERIAL_7O2 (uint32_t)(SERIAL_DATA_BITS_7 | SERIAL_PARITY_ODD  | SERIAL_STOP_BITS_2)
#define SERIAL_8N1 (uint32_t)(SERIAL_DATA_BITS_8 | SERIAL_PARITY_NO   | SERIAL_STOP_BITS_1)
#define SERIAL_8N2 (uint32_t)(SERIAL_DATA_BITS_8 | SERIAL_PARITY_NO   | SERIAL_STOP_BITS_2)
#define SERIAL_8E1 (uint32_t)(SERIAL_DATA_BITS_8 | SERIAL_PARITY_EVEN | SERIAL_STOP_BITS_1)
#define SERIAL_8E2 (uint32_t)(SERIAL_DATA_BITS_8 | SERIAL_PARITY_EVEN | SERIAL_STOP_BITS_2)
#define SERIAL_8O1 (uint32_t)(SERIAL_DATA_BITS_8 | SERIAL_PARITY_ODD  | SERIAL_STOP_BITS_1)
#define SERIAL_8O2 (uint32_t)(SERIAL_DATA_BITS_8 | SERIAL_PARITY_ODD  | SERIAL_STOP_BITS_2)
#define SERIAL_9N1 (uint32_t)(SERIAL_DATA_BITS_9 | SERIAL_PARITY_NO   | SERIAL_STOP_BITS_1)
#define SERIAL_9N2 (uint32_t)(SERIAL_DATA_BITS_9 | SERIAL_PARITY_NO   | SERIAL_STOP_BITS_2)

// Serial Configuration masks
#define SERIAL_STOP_BITS      ((uint32_t)0b00000011)
#define SERIAL_PARITY         ((uint32_t)0b00001100)
#define SERIAL_DATA_BITS      ((uint32_t)0b00110000)
#define SERIAL_FLOW_CONTROL   ((uint32_t)0b11000000)
#define SERIAL_MODE           (SERIAL_DATA_BITS | SERIAL_PARITY | SERIAL_STOP_BITS)

// Stop bits
#define SERIAL_STOP_BITS_1    ((uint32_t)0b00000000)
#define SERIAL_STOP_BITS_2    ((uint32_t)0b00000001)
#define SERIAL_STOP_BITS_0_5  ((uint32_t)0b00000010)
#define SERIAL_STOP_BITS_1_5  ((uint32_t)0b00000011)

// Parity
#define SERIAL_PARITY_NO      ((uint32_t)0b00000000)
#define SERIAL_PARITY_EVEN    ((uint32_t)0b00000100)
#define SERIAL_PARITY_ODD     ((uint32_t)0b00001000)

// Data bits
#define SERIAL_DATA_BITS_8    ((uint32_t)0b00000000)
#define SERIAL_DATA_BITS_9    ((uint32_t)0b00010000)
#define SERIAL_DATA_BITS_7    ((uint32_t)0b00100000)

// Flow control settings
#define SERIAL_FLOW_CONTROL_NONE    ((uint32_t)0b00000000)
#define SERIAL_FLOW_CONTROL_RTS     ((uint32_t)0b01000000)
#define SERIAL_FLOW_CONTROL_CTS     ((uint32_t)0b10000000)
#define SERIAL_FLOW_CONTROL_RTS_CTS ((uint32_t)0b11000000)

// LIN Configuration masks
#define LIN_MODE        ((uint32_t)0x0300)
#define LIN_BREAK_BITS  ((uint32_t)0x0C00)

// LIN modes
#define LIN_MODE_MASTER ((uint32_t)0x0100)
#define LIN_MODE_SLAVE  ((uint32_t)0x0200)

// LIN break settings
// Supported only in Master mode
#define LIN_BREAK_13B   ((uint32_t)0x0000)
// Supported only in Slave mode
#define LIN_BREAK_10B   ((uint32_t)0x0800)
#define LIN_BREAK_11B   ((uint32_t)0x0C00)

// Pre-defined LIN configurations
#define LIN_MASTER_13B (uint32_t)(((uint32_t)SERIAL_8N1) | LIN_MODE_MASTER | LIN_BREAK_13B)
#define LIN_SLAVE_10B  (uint32_t)(((uint32_t)SERIAL_8N1) | LIN_MODE_SLAVE  | LIN_BREAK_10B)
#define LIN_SLAVE_11B  (uint32_t)(((uint32_t)SERIAL_8N1) | LIN_MODE_SLAVE  | LIN_BREAK_11B)

// Half-duplex
#define SERIAL_HALF_DUPLEX            ((uint32_t)0x1000)
#define SERIAL_HALF_DUPLEX_NO_ECHO    ((uint32_t)0x3000)

#define SERIAL_OPEN_DRAIN             ((uint32_t)0x4000)
#define SERIAL_HALF_DUPLEX_OPEN_DRAIN (SERIAL_HALF_DUPLEX | SERIAL_OPEN_DRAIN)

#define SERIAL_TX_PULL_UP             ((uint32_t)0x8000)

/* Exported types ------------------------------------------------------------*/
typedef struct hal_usart_ring_buffer_t {
    uint16_t buffer[SERIAL_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
} hal_usart_ring_buffer_t;

typedef enum hal_usart_interface_t {
    HAL_USART_SERIAL1 = 0,    // maps to Serial1
    HAL_USART_SERIAL2 = 1,    // may map to NCP or Serial2, see HAL_PLATFORM_CELLULAR_SERIAL / HAL_PLATFORM_WIFI_SERIAL
    HAL_USART_SERIAL3 = 2,    // may map to NCP or Serial3, see HAL_PLATFORM_CELLULAR_SERIAL / HAL_PLATFORM_WIFI_SERIAL
    HAL_USART_SERIAL4 = 3,    // may map to NCP or Serial4, see HAL_PLATFORM_CELLULAR_SERIAL / HAL_PLATFORM_WIFI_SERIAL
    HAL_USART_SERIAL5 = 4,    // may map to NCP or Serial5, see HAL_PLATFORM_CELLULAR_SERIAL / HAL_PLATFORM_WIFI_SERIAL
} hal_usart_interface_t;

typedef enum hal_usart_state_t {
    HAL_USART_STATE_UNKNOWN,
    HAL_USART_STATE_ENABLED,
    HAL_USART_STATE_DISABLED,
    HAL_USART_STATE_SUSPENDED
} hal_usart_state_t;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_usart_buffer_config_t {
    uint16_t size;
    void* rx_buffer;
    uint16_t rx_buffer_size;
    void* tx_buffer;
    uint16_t tx_buffer_size;
} hal_usart_buffer_config_t;

int hal_usart_init_ex(hal_usart_interface_t serial, const hal_usart_buffer_config_t* config, void*);
void hal_usart_init(hal_usart_interface_t serial, hal_usart_ring_buffer_t *rx_buffer, hal_usart_ring_buffer_t *tx_buffer);
void hal_usart_begin(hal_usart_interface_t serial, uint32_t baud);
void hal_usart_begin_config(hal_usart_interface_t serial, uint32_t baud, uint32_t config, void*);
void hal_usart_end(hal_usart_interface_t serial);
uint32_t hal_usart_write(hal_usart_interface_t serial, uint8_t data);
int32_t hal_usart_available_data_for_write(hal_usart_interface_t serial);
int32_t hal_usart_available(hal_usart_interface_t serial);
int32_t hal_usart_read(hal_usart_interface_t serial);
int32_t hal_usart_peek(hal_usart_interface_t serial);
void hal_usart_flush(hal_usart_interface_t serial);
bool hal_usart_is_enabled(hal_usart_interface_t serial);
void hal_usart_half_duplex(hal_usart_interface_t serial, bool enable);
uint32_t hal_usart_write_nine_bits(hal_usart_interface_t serial, uint16_t data);
void hal_usart_send_break(hal_usart_interface_t serial, void* reserved);
uint8_t hal_usart_break_detected(hal_usart_interface_t serial);
int hal_usart_sleep(hal_usart_interface_t serial, bool sleep, void* reserved);

ssize_t hal_usart_write_buffer(hal_usart_interface_t serial, const void* buffer, size_t size, size_t elementSize);
ssize_t hal_usart_read_buffer(hal_usart_interface_t serial, void* buffer, size_t size, size_t elementSize);
ssize_t hal_usart_peek_buffer(hal_usart_interface_t serial, void* buffer, size_t size, size_t elementSize);


#include "usart_hal_compat.h"

#ifdef __cplusplus
}
#endif

#endif  /* USART_HAL_H */
