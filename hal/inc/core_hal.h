/**
 ******************************************************************************
 * @file    core_hal.h
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
#ifndef __CORE_HAL_H
#define __CORE_HAL_H

#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/
typedef enum
{
    BKP_DR_01 = 0x01,
    BKP_DR_02 = 0x02,
    BKP_DR_03 = 0x03,
    BKP_DR_04 = 0x04,
    BKP_DR_05 = 0x05,
    BKP_DR_06 = 0x06,
    BKP_DR_07 = 0x07,
    BKP_DR_08 = 0x08,
    BKP_DR_09 = 0x09,
    BKP_DR_10 = 0x10
} BKP_DR_TypeDef;

typedef enum
{
    PIN_RESET = 0x01,
    SOFTWARE_RESET = 0x02,
    WATCHDOG_RESET = 0x03,
    LOW_POWER_RESET = 0x04
} RESET_TypeDef;

/* Exported constants --------------------------------------------------------*/

//Following is normally defined via "CFLAGS += -DDFU_BUILD_ENABLE" in makefile
#ifndef DFU_BUILD_ENABLE
#define DFU_BUILD_ENABLE
#endif

/*
 * Use the JTAG IOs as standard GPIOs (D3 to D7)
 * Note that once the JTAG IOs are disabled, the connection with the host debugger
 * is lost and cannot be re-established as long as the JTAG IOs remain disabled.
 */

#ifdef USE_SWD_JTAG
#define SWD_JTAG_ENABLE    
#else 
#ifdef USE_SWD
#define SWD_ENABLE_JTAG_DISABLE    
#else
#define SWD_JTAG_DISABLE
#endif
#endif
/*
 * Use Independent Watchdog to force a system reset when a software error occurs
 * During JTAG program/debug, the Watchdog counting is disabled by debug configuration
 */
#define IWDG_RESET_ENABLE
#define TIMING_IWDG_RELOAD      1000 //1sec

/* Exported functions --------------------------------------------------------*/
#include "watchdog_hal.h"
#include "core_subsys_hal.h"

#ifdef __cplusplus
extern "C" {
#endif
    
void HAL_Core_Init(void);
void HAL_Core_Config(void);
bool HAL_Core_Validate_User_Module(void);
bool HAL_Core_Mode_Button_Pressed(uint16_t pressedMillisDuration);
void HAL_Core_Mode_Button_Reset(void);
void HAL_Core_System_Reset(void);
void HAL_Core_Factory_Reset(void);
void HAL_Core_Enter_Bootloader(bool persist);
void HAL_Core_Enter_Stop_Mode(uint16_t wakeUpPin, uint16_t edgeTriggerMode);
void HAL_Core_Execute_Stop_Mode(void);
void HAL_Core_Enter_Standby_Mode(void);
void HAL_Core_Execute_Standby_Mode(void);
uint32_t HAL_Core_Compute_CRC32(const uint8_t *pBuffer, uint32_t bufferSize);

typedef enum _BootloaderFlag_t {
    BOOTLOADER_FLAG_VERSION,
    BOOTLOADER_FLAG_STARTUP_MODE
} BootloaderFlag;

uint16_t HAL_Bootloader_Get_Flag(BootloaderFlag flag);

//Following is currently defined in bootloader/src/core-vx/dfu_hal.c
//Move the definitions to core_hal.c and add hal as a dependency for bootloader
int32_t HAL_Core_Backup_Register(uint32_t BKP_DR);
void HAL_Core_Write_Backup_Register(uint32_t BKP_DR, uint32_t Data);
uint32_t HAL_Core_Read_Backup_Register(uint32_t BKP_DR);

void HAL_SysTick_Handler(void);

void HAL_Bootloader_Lock(bool lock);

bool HAL_Core_System_Reset_FlagSet(RESET_TypeDef resetType);

extern void app_setup_and_loop();

#ifdef __cplusplus
}
#endif

#endif  /* __CORE_HAL_H */
