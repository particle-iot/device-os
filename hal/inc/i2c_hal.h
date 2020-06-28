/**
 ******************************************************************************
 * @file    i2c_hal.h
 * @author  Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    12-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __I2C_HAL_H
#define __I2C_HAL_H

/* Includes ------------------------------------------------------------------*/
#include <stddef.h>
#include "pinmap_hal.h"
#include "platforms.h"
#include "system_tick_hal.h"

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  I2C_MODE_MASTER = 0, I2C_MODE_SLAVE = 1
} I2C_Mode;

/* Exported types ------------------------------------------------------------*/
typedef enum HAL_I2C_Interface {
    HAL_I2C_INTERFACE1 = 0,
    HAL_I2C_INTERFACE2 = 1,
    HAL_I2C_INTERFACE3 = 2
} HAL_I2C_Interface;

/*! I2c Config Structure Version */
typedef enum HAL_I2C_Config_Version {
    HAL_I2C_CONFIG_VERSION_1 = 0,
    HAL_I2C_CONFIG_VERSION_LATEST = HAL_I2C_CONFIG_VERSION_1,
} HAL_I2C_Config_Version;

typedef struct HAL_I2C_Config {
    uint16_t size;
    uint16_t version;
    uint8_t* rx_buffer;
    uint32_t rx_buffer_size;
    uint8_t* tx_buffer;
    uint32_t tx_buffer_size;
} HAL_I2C_Config;

typedef enum HAL_I2C_Transmission_Flag {
    HAL_I2C_TRANSMISSION_FLAG_NONE = 0x00,
    HAL_I2C_TRANSMISSION_FLAG_STOP = 0x01
} HAL_I2C_Transmission_Flag;

typedef struct HAL_I2C_Transmission_Config {
    uint16_t size;
    uint16_t version;

    uint8_t address;
    uint8_t reserved[3];
    uint32_t quantity;
    system_tick_t timeout_ms;
    uint32_t flags;
} HAL_I2C_Transmission_Config;

typedef enum hal_i2c_state_t {
    HAL_I2C_STATE_DISABLED,
    HAL_I2C_STATE_ENABLED,
    HAL_I2C_STATE_SUSPENDED
} hal_i2c_state_t;

/* Exported constants --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/
#define CLOCK_SPEED_100KHZ         (uint32_t)100000
#define CLOCK_SPEED_400KHZ         (uint32_t)400000
#define HAL_I2C_DEFAULT_TIMEOUT_MS (100)
#define I2C_BUFFER_LENGTH          (uint8_t)32

/* Exported functions --------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

int HAL_I2C_Init(HAL_I2C_Interface i2c, const HAL_I2C_Config* config);
void HAL_I2C_Set_Speed(HAL_I2C_Interface i2c, uint32_t speed, void* reserved);
void HAL_I2C_Enable_DMA_Mode(HAL_I2C_Interface i2c, bool enable, void* reserved);
void HAL_I2C_Stretch_Clock(HAL_I2C_Interface i2c, bool stretch, void* reserved);
void HAL_I2C_Begin(HAL_I2C_Interface i2c, I2C_Mode mode, uint8_t address, void* reserved);
void HAL_I2C_End(HAL_I2C_Interface i2c, void* reserved);
uint32_t HAL_I2C_Request_Data(HAL_I2C_Interface i2c, uint8_t address, uint8_t quantity, uint8_t stop, void* reserved);
int32_t HAL_I2C_Request_Data_Ex(HAL_I2C_Interface i2c, const HAL_I2C_Transmission_Config* config, void* reserved);
void HAL_I2C_Begin_Transmission(HAL_I2C_Interface i2c, uint8_t address, const HAL_I2C_Transmission_Config* config);
uint8_t HAL_I2C_End_Transmission(HAL_I2C_Interface i2c, uint8_t stop, void* reserved);
uint32_t HAL_I2C_Write_Data(HAL_I2C_Interface i2c, uint8_t data, void* reserved);
int32_t HAL_I2C_Available_Data(HAL_I2C_Interface i2c, void* reserved);
int32_t HAL_I2C_Read_Data(HAL_I2C_Interface i2c, void* reserved);
int32_t HAL_I2C_Peek_Data(HAL_I2C_Interface i2c, void* reserved);
void HAL_I2C_Flush_Data(HAL_I2C_Interface i2c, void* reserved);
bool HAL_I2C_Is_Enabled(HAL_I2C_Interface i2c, void* reserved);
void HAL_I2C_Set_Callback_On_Receive(HAL_I2C_Interface i2c, void (*function)(int), void* reserved);
void HAL_I2C_Set_Callback_On_Request(HAL_I2C_Interface i2c, void (*function)(void), void* reserved);
uint8_t HAL_I2C_Reset(HAL_I2C_Interface i2c, uint32_t reserved, void* reserve1);

int32_t HAL_I2C_Acquire(HAL_I2C_Interface i2c, void* reserved);
int32_t HAL_I2C_Release(HAL_I2C_Interface i2c, void* reserved);
int HAL_I2C_Sleep(HAL_I2C_Interface i2c, bool sleep, void* reserved);

void HAL_I2C_Set_Speed_v1(uint32_t speed);
void HAL_I2C_Enable_DMA_Mode_v1(bool enable);
void HAL_I2C_Stretch_Clock_v1(bool stretch);
void HAL_I2C_Begin_v1(I2C_Mode mode, uint8_t address);
void HAL_I2C_End_v1();
uint32_t HAL_I2C_Request_Data_v1(uint8_t address, uint8_t quantity, uint8_t stop);
void HAL_I2C_Begin_Transmission_v1(uint8_t address);
uint8_t HAL_I2C_End_Transmission_v1(uint8_t stop);
uint32_t HAL_I2C_Write_Data_v1(uint8_t data);
int32_t HAL_I2C_Available_Data_v1(void);
int32_t HAL_I2C_Read_Data_v1(void);
int32_t HAL_I2C_Peek_Data_v1(void);
void HAL_I2C_Flush_Data_v1(void);
bool HAL_I2C_Is_Enabled_v1(void);
void HAL_I2C_Set_Callback_On_Receive_v1(void (*function)(int));
void HAL_I2C_Set_Callback_On_Request_v1(void (*function)(void));

#ifdef __cplusplus
}
#endif

#endif  /* __I2C_HAL_H */
