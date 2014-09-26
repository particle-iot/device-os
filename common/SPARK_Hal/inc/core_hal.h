/**
 ******************************************************************************
 * @file    core_hal.h
 * @author  Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    12-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

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
#ifndef __CORE_HAL_H
#define __CORE_HAL_H

#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/
//Following is normally defined via "CFLAGS += -DDFU_BUILD_ENABLE" in makefile
#ifndef DFU_BUILD_ENABLE
#define DFU_BUILD_ENABLE
#endif

/*
 * Use the JTAG IOs as standard GPIOs (D3 to D7)
 * Note that once the JTAG IOs are disabled, the connection with the host debugger
 * is lost and cannot be re-established as long as the JTAG IOs remain disabled.
 */
#ifndef USE_SWD_JTAG
#define SWD_JTAG_DISABLE
#endif

/*
 * Use Independent Watchdog to force a system reset when a software error occurs
 * During JTAG program/debug, the Watchdog counting is disabled by debug configuration
 */
#define IWDG_RESET_ENABLE
#define TIMING_IWDG_RELOAD      1000 //1sec

/* Exported functions --------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

void HAL_Core_Config(void);
bool HAL_Core_Mode_Button_Pressed(uint16_t pressedMillisDuration);
void HAL_Core_Mode_Button_Reset(void);
void HAL_Core_System_Reset(void);
void HAL_Core_Factory_Reset(void);
void HAL_Core_Enter_Bootloader(void);
void HAL_Core_Enter_Stop_Mode(uint16_t wakeUpPin, uint16_t edgeTriggerMode);
void HAL_Core_Execute_Stop_Mode(void);
void HAL_Core_Enter_Standby_Mode(void);
void HAL_Core_Execute_Standby_Mode(void);
uint32_t HAL_Core_Compute_CRC32(uint8_t *pBuffer, uint32_t bufferSize);

void HAL_SysTick_Handler(void) __attribute__ ((weak));

#ifdef __cplusplus
}
#endif

#endif  /* __CORE_HAL_H */
