/**
 ******************************************************************************
 * @file    services-dynalib.h
 * @author  Matthew McGowan
 ******************************************************************************
  Copyright (c) 2015 Spark Labs, Inc.  All rights reserved.

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

#pragma once

#include "dynalib.h"
#include "usb_config_hal.h"
#include "core_hal.h"

/**
 * Define the dynamic exports for the current platform.
 * 
 * Note that once used in released binary, the only permissable changes to the dynamic
 * library definition is to append new functions to the end of the list.  This is
 * so the dynamic library maintains backwards compatibility and can continue to
 * be used by older clients.
 * 
 * Each platform may have a distinct dynamic library, via conditional compilation
 * but the rule above still holds.
 */

// as an option to free up space in the system-hal image, we can split the HAL
// into the parts needed by the system, 
#define DYNALIB_USER_FN(x,y) DYNALIB_FN(x,y)

DYNALIB_BEGIN(hal)        
DYNALIB_FN(hal,HAL_core_subsystem_version)        
DYNALIB_FN(hal,HAL_Core_Init)
DYNALIB_FN(hal,HAL_Core_Config)
DYNALIB_FN(hal,HAL_Core_Mode_Button_Pressed)
DYNALIB_FN(hal,HAL_Core_Mode_Button_Reset)
DYNALIB_FN(hal,HAL_Core_System_Reset)
DYNALIB_FN(hal,HAL_Core_Factory_Reset)
DYNALIB_FN(hal,HAL_Core_Enter_Bootloader)
DYNALIB_FN(hal,HAL_Core_Enter_Stop_Mode)
DYNALIB_FN(hal,HAL_Core_Execute_Stop_Mode)
DYNALIB_FN(hal,HAL_Core_Enter_Standby_Mode)
DYNALIB_FN(hal,HAL_Core_Execute_Standby_Mode)
DYNALIB_FN(hal,HAL_Core_Compute_CRC32)

DYNALIB_FN(hal,HAL_Delay_Milliseconds)
DYNALIB_FN(hal,HAL_Delay_Microseconds)
DYNALIB_FN(hal,HAL_device_ID)
        
DYNALIB_FN(hal,inet_gethostbyname)
        
DYNALIB_FN(hal,HAL_OTA_FlashAddress)
DYNALIB_FN(hal,HAL_OTA_FlashLength)
DYNALIB_FN(hal,HAL_OTA_ChunkSize)

DYNALIB_FN(hal,HAL_OTA_Flashed_GetStatus)
DYNALIB_FN(hal,HAL_OTA_Flashed_ResetStatus)

#if PLATFORM_ID > 3
DYNALIB_FN(hal,HAL_FLASH_AddToNextAvailableModulesSlot)
#endif
DYNALIB_FN(hal,HAL_FLASH_WriteProtectionEnable)
DYNALIB_FN(hal,HAL_FLASH_WriteProtectionDisable)
DYNALIB_FN(hal,HAL_FLASH_Begin)
DYNALIB_FN(hal,HAL_FLASH_Update)
DYNALIB_FN(hal,HAL_FLASH_End)

DYNALIB_FN(hal,HAL_RTC_Configuration)
DYNALIB_FN(hal,HAL_RTC_Get_UnixTime)
DYNALIB_FN(hal,HAL_RTC_Set_UnixTime)
DYNALIB_FN(hal,HAL_RTC_Set_UnixAlarm)

#if PLATFORM_ID > 3
DYNALIB_FN(hal,HAL_RNG_Configuration)
DYNALIB_FN(hal,HAL_RNG_GetRandomNumber)
#endif
        
DYNALIB_FN(hal,socket_active_status)
DYNALIB_FN(hal,socket_handle_valid)
DYNALIB_FN(hal,socket_create)
DYNALIB_FN(hal,socket_connect)
DYNALIB_FN(hal,socket_receive)
DYNALIB_FN(hal,socket_receivefrom)
DYNALIB_FN(hal,socket_send)
DYNALIB_FN(hal,socket_bind)
DYNALIB_FN(hal,socket_sendto)
DYNALIB_FN(hal,socket_close)
DYNALIB_FN(hal,socket_reset_blocking_call)
DYNALIB_FN(hal,socket_create_nonblocking_server)
DYNALIB_FN(hal,socket_accept)

DYNALIB_FN(hal,HAL_Get_Sys_Health)
DYNALIB_FN(hal,HAL_Set_Sys_Health)
        
DYNALIB_FN(hal,HAL_Timer_Get_Micro_Seconds)
DYNALIB_FN(hal,HAL_Timer_Get_Milli_Seconds)
                
        
#ifdef USB_CDC_ENABLE
DYNALIB_FN(hal,USB_USART_Init)
DYNALIB_FN(hal,USB_USART_Available_Data)
DYNALIB_FN(hal,USB_USART_Receive_Data)
DYNALIB_FN(hal,USB_USART_Send_Data)
DYNALIB_FN(hal,USB_USART_Baud_Rate)
#endif     
        
DYNALIB_FN(hal,HAL_watchdog_reset_flagged)
DYNALIB_FN(hal,HAL_Notify_WDT)
DYNALIB_FN(hal,wlan_connect_init)
DYNALIB_FN(hal,wlan_connect_finalize)

DYNALIB_FN(hal,wlan_reset_credentials_store_required)
DYNALIB_FN(hal,wlan_reset_credentials_store)
DYNALIB_FN(hal,wlan_disconnect_now)
DYNALIB_FN(hal,wlan_activate)
DYNALIB_FN(hal,wlan_deactivate)
DYNALIB_FN(hal,wlan_connected_rssi)

DYNALIB_FN(hal,wlan_clear_credentials)
DYNALIB_FN(hal,wlan_has_credentials)
DYNALIB_FN(hal,wlan_set_credentials)

DYNALIB_FN(hal,wlan_smart_config_init)
DYNALIB_FN(hal,wlan_smart_config_cleanup)
DYNALIB_FN(hal,wlan_smart_config_finalize)

DYNALIB_FN(hal,wlan_set_error_count)
DYNALIB_FN(hal,wlan_fetch_ipconfig)
DYNALIB_FN(hal,wlan_setup)

DYNALIB_FN(hal,HAL_WLAN_SetNetWatchDog)

DYNALIB_FN(hal,HAL_Bootloader_Get_Version)
DYNALIB_FN(hal,HAL_Bootloader_Lock)        
DYNALIB_FN(hal,HAL_Pin_Map)

DYNALIB_END(hal)
