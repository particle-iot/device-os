/**
 ******************************************************************************
 * @file    core_hal.c
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    27-Sept-2014
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

#include "core_hal.h"
#include "system_error.h"

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

void HAL_Core_Init(void)
{
}


/*******************************************************************************
 * Function Name  : HAL_Core_Config.
 * Description    : Called in startup routine, before calling C++ constructors.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_Core_Config(void)
{
}

bool HAL_Core_Mode_Button_Pressed(uint16_t pressedMillisDuration)
{
    return false;
}

void HAL_Core_Mode_Button_Reset(void)
{
}

void HAL_Core_System_Reset(void)
{
}

void HAL_Core_Factory_Reset(void)
{
}

void HAL_Core_Enter_Safe_Mode(void* reserved)
{
}

void HAL_Core_Enter_Bootloader(bool persist)
{
}

void HAL_Core_Enter_Stop_Mode(uint16_t wakeUpPin, uint16_t edgeTriggerMode)
{
}

int HAL_Core_Enter_Stop_Mode_Ext(const uint16_t* pins, size_t pins_count, const InterruptMode* mode, size_t mode_count, long seconds, void* reserved)
{
    return -1;
}

void HAL_Core_Execute_Stop_Mode(void)
{
}

int HAL_Core_Enter_Standby_Mode(uint32_t seconds, uint32_t flags)
{
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

void HAL_Core_Execute_Standby_Mode(void)
{
}

int HAL_Core_Get_Last_Reset_Info(int *reason, uint32_t *data, void *reserved)
{
    return -1;
}

/**
 * @brief  Computes the 32-bit CRC of a given buffer of byte data.
 * @param  pBuffer: pointer to the buffer containing the data to be computed
 * @param  BufferSize: Size of the buffer to be computed
 * @retval 32-bit CRC
 */
uint32_t HAL_Core_Compute_CRC32(const uint8_t *pBuffer, uint32_t bufferSize)
{
    return 0;
}

// todo find a technique that allows accessor functions to be inlined while still keeping
// hardware independence.
bool HAL_watchdog_reset_flagged()
{
    return false;
}

void HAL_Notify_WDT()
{
}

uint16_t HAL_Core_Mode_Button_Pressed_Time()
{
    return 0;
}

void HAL_Bootloader_Lock(bool lock)
{
}


unsigned HAL_Core_System_Clock(HAL_SystemClock clock, void* reserved)
{
    return 1;
}


int main()
{
    while(1);
}


int HAL_Feature_Set(HAL_Feature feature, bool enabled)
{
    return -1;
}

bool HAL_Feature_Get(HAL_Feature feature)
{
    return false;
}

int HAL_Set_System_Config(hal_system_config_t config_item, const void* data, unsigned length)
{
    return -1;
}

int32_t HAL_Core_Backup_Register(uint32_t BKP_DR)
{
    return -1;
}

void HAL_Core_Write_Backup_Register(uint32_t BKP_DR, uint32_t Data)
{
}

uint32_t HAL_Core_Read_Backup_Register(uint32_t BKP_DR)
{
    return 0xFFFFFFFF;
}

void HAL_Core_Button_Mirror_Pin_Disable(uint8_t bootloader, uint8_t button, void* reserved)
{
}

void HAL_Core_Button_Mirror_Pin(uint16_t pin, InterruptMode mode, uint8_t bootloader, uint8_t button, void *reserved)
{
}

void HAL_Core_Led_Mirror_Pin_Disable(uint8_t led, uint8_t bootloader, void* reserved)
{
}

void HAL_Core_Led_Mirror_Pin(uint8_t led, pin_t pin, uint32_t flags, uint8_t bootloader, void* reserved)
{
}

void HAL_Set_Event_Callback(HAL_Event_Callback callback, void* reserved) {
}

void hal_notify_event(int event, int flags, void* data) {
}
