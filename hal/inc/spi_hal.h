/**
 ******************************************************************************
 * @file    spi_hal.h
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
#ifndef __SPI_HAL_H
#define __SPI_HAL_H

/* Includes ------------------------------------------------------------------*/
#include "pinmap_hal.h"

/* Exported types ------------------------------------------------------------*/
typedef enum HAL_SPI_Interface {
    HAL_SPI_INTERFACE1 = 0,    //maps to SPI1 (pins: A3, A4, A5)
    HAL_SPI_INTERFACE2 = 1     //maps to SPI3 (pins: D4, D3, D2)
#if PLATFORM_ID == 10 // Electron
   ,HAL_SPI_INTERFACE3 = 2     //maps to SPI3 (pins: C3, C2, C1)
#endif
    ,TOTAL_SPI
} HAL_SPI_Interface;

typedef enum
{
    SPI_MODE_MASTER = 0, SPI_MODE_SLAVE = 1
} SPI_Mode;

typedef void (*HAL_SPI_DMA_UserCallback)(void);
typedef void (*HAL_SPI_Select_UserCallback)(uint8_t);

/* Exported constants --------------------------------------------------------*/

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

/* Exported functions --------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  HAL_SPI_INFO_VERSION_1 = 11,
  HAL_SPI_INFO_VERSION_2 = 12
} hal_spi_info_version_t;

#define HAL_SPI_INFO_VERSION HAL_SPI_INFO_VERSION_2

typedef struct hal_spi_info_t {
    uint16_t version;

    uint32_t system_clock;      // the clock speed that is divided when setting a divider
    //
    uint8_t default_settings;
    uint8_t enabled;
    SPI_Mode mode;
    uint32_t clock;
    uint8_t bit_order;
    uint8_t data_mode;
    pin_t ss_pin;
} hal_spi_info_t;

typedef struct HAL_SPI_TransferStatus {
    uint8_t version;
    uint32_t configured_transfer_length;
    uint32_t transfer_length;
    uint8_t transfer_ongoing    : 1;
    uint8_t ss_state            : 1;
} HAL_SPI_TransferStatus;

void HAL_SPI_Init(HAL_SPI_Interface spi);
void HAL_SPI_Begin(HAL_SPI_Interface spi, uint16_t pin);
void HAL_SPI_Begin_Ext(HAL_SPI_Interface spi, SPI_Mode mode, uint16_t pin, void* reserved);
void HAL_SPI_End(HAL_SPI_Interface spi);
void HAL_SPI_Set_Bit_Order(HAL_SPI_Interface spi, uint8_t order);
void HAL_SPI_Set_Data_Mode(HAL_SPI_Interface spi, uint8_t mode);
void HAL_SPI_Set_Clock_Divider(HAL_SPI_Interface spi, uint8_t rate);
uint16_t HAL_SPI_Send_Receive_Data(HAL_SPI_Interface spi, uint16_t data);
void HAL_SPI_DMA_Transfer(HAL_SPI_Interface spi, void* tx_buffer, void* rx_buffer, uint32_t length, HAL_SPI_DMA_UserCallback userCallback);
bool HAL_SPI_Is_Enabled_Old();
bool HAL_SPI_Is_Enabled(HAL_SPI_Interface spi);
void HAL_SPI_Info(HAL_SPI_Interface spi, hal_spi_info_t* info, void* reserved);
void HAL_SPI_Set_Callback_On_Select(HAL_SPI_Interface spi, HAL_SPI_Select_UserCallback cb, void* reserved);
void HAL_SPI_DMA_Transfer_Cancel(HAL_SPI_Interface spi);
int32_t HAL_SPI_DMA_Transfer_Status(HAL_SPI_Interface spi, HAL_SPI_TransferStatus* st);
// HAL_SPI_Set_Bit_Order, HAL_SPI_Set_Data_Mode and HAL_SPI_Set_Clock_Divider in one go
// to avoid having to reconfigure SPI peripheral 3 times
int32_t HAL_SPI_Set_Settings(HAL_SPI_Interface spi, uint8_t set_default, uint8_t clockdiv, uint8_t order, uint8_t mode, void* reserved);

int32_t HAL_SPI_Acquire(HAL_SPI_Interface spi, void* reserved);
int32_t HAL_SPI_Release(HAL_SPI_Interface spi, void* reserved);

#ifdef __cplusplus
}
#endif

#endif  /* __SPI_HAL_H */
