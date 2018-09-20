/**
 ******************************************************************************
 * @file    hal_dynalib_usb.h
 * @authors Andrey Tolstoy
 * @date    24 April 2016
 ******************************************************************************
  Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.
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

#ifndef HAL_DYNALIB_USB_H
#define HAL_DYNALIB_USB_H

#include "dynalib.h"
#include "usb_config_hal.h"
#include "platform_config.h"

#ifdef DYNALIB_EXPORT
#include "usb_hal.h"
#endif

DYNALIB_BEGIN(hal_usb)

#ifdef USB_CDC_ENABLE
DYNALIB_FN(0, hal_usb, HAL_USB_USART_Init, void(HAL_USB_USART_Serial, const HAL_USB_USART_Config*))
DYNALIB_FN(1, hal_usb, HAL_USB_USART_Begin, void(HAL_USB_USART_Serial, uint32_t, void *))
DYNALIB_FN(2, hal_usb, HAL_USB_USART_End, void(HAL_USB_USART_Serial))
DYNALIB_FN(3, hal_usb, HAL_USB_USART_Baud_Rate, unsigned int(HAL_USB_USART_Serial))
DYNALIB_FN(4, hal_usb, HAL_USB_USART_Available_Data, int32_t(HAL_USB_USART_Serial))
DYNALIB_FN(5, hal_usb, HAL_USB_USART_Available_Data_For_Write, int32_t(HAL_USB_USART_Serial))
DYNALIB_FN(6, hal_usb, HAL_USB_USART_Receive_Data, int32_t(HAL_USB_USART_Serial, uint8_t))
DYNALIB_FN(7, hal_usb, HAL_USB_USART_Send_Data, int32_t(HAL_USB_USART_Serial, uint8_t))
DYNALIB_FN(8, hal_usb, HAL_USB_USART_Flush_Data, void(HAL_USB_USART_Serial))
DYNALIB_FN(9, hal_usb, HAL_USB_USART_Is_Enabled, bool(HAL_USB_USART_Serial))
DYNALIB_FN(10, hal_usb, HAL_USB_USART_Is_Connected, bool(HAL_USB_USART_Serial))
DYNALIB_FN(11, hal_usb, HAL_USB_USART_LineCoding_BitRate_Handler, int32_t(void (*handler)(uint32_t bitRate), void* reserved))
#define BASE_IDX 12
#else
#define BASE_IDX 0
#endif

#ifdef USB_HID_ENABLE
DYNALIB_FN(BASE_IDX + 0, hal_usb, HAL_USB_HID_Init, void(uint8_t, void*))
DYNALIB_FN(BASE_IDX + 1, hal_usb, HAL_USB_HID_Begin, void(uint8_t, void*))
DYNALIB_FN(BASE_IDX + 2, hal_usb, HAL_USB_HID_Send_Report, void(uint8_t, void*, uint16_t, void*))
DYNALIB_FN(BASE_IDX + 3, hal_usb, HAL_USB_HID_End, void(uint8_t))
# define BASE_IDX1 (BASE_IDX + 4)
#else
# define BASE_IDX1 BASE_IDX
#endif

#if defined(USB_CDC_ENABLE) || defined(USB_HID_ENABLE)
DYNALIB_FN(BASE_IDX1 + 0, hal_usb, HAL_USB_Init, void(void))
DYNALIB_FN(BASE_IDX1 + 1, hal_usb, HAL_USB_Attach, void(void))
DYNALIB_FN(BASE_IDX1 + 2, hal_usb, HAL_USB_Detach, void(void))
# define BASE_IDX2 (BASE_IDX1 + 3)
#else
# define BASE_IDX2 BASE_IDX1
#endif

#ifdef USB_VENDOR_REQUEST_ENABLE
DYNALIB_FN(BASE_IDX2 + 0, hal_usb, HAL_USB_Set_Vendor_Request_Callback, void(HAL_USB_Vendor_Request_Callback, void*))
# define BASE_IDX3 (BASE_IDX2 + 1)
#else
# define BASE_IDX3 BASE_IDX2
#endif


#ifdef USE_USB_OTG_FS
	DYNALIB_FN(BASE_IDX3 + 0, hal_usb, OTG_FS_WKUP_irq, void(void))
	DYNALIB_FN(BASE_IDX3 + 1, hal_usb, OTG_FS_irq, void(void))
	#define BASE_IDX4 (BASE_IDX3 + 2)
#elif defined USE_USB_OTG_HS
	DYNALIB_FN(BASE_IDX3 + 0, hal_usb, OTG_HS_WKUP_irq, void(void))
	DYNALIB_FN(BASE_IDX3 + 1, hal_usb, OTG_HS_irq, void(void))
	DYNALIB_FN(BASE_IDX3 + 2, hal_usb, OTG_HS_EP1_OUT_irq, void(void))
	DYNALIB_FN(BASE_IDX3 + 3, hal_usb, OTG_HS_EP1_IN_irq, void(void))
	#define BASE_IDX4 (BASE_IDX3 + 4)
#else
# define BASE_IDX4 BASE_IDX3
#endif

#ifdef USB_HID_ENABLE
DYNALIB_FN(BASE_IDX4 + 0, hal_usb, HAL_USB_HID_Status, int32_t(uint8_t, void*))
DYNALIB_FN(BASE_IDX4 + 1, hal_usb, HAL_USB_HID_Set_State, uint8_t(uint8_t, uint8_t, void*))
# define BASE_IDX5 (BASE_IDX4 + 2)
#else
# define BASE_IDX5 BASE_IDX4
#endif

#ifdef USB_VENDOR_REQUEST_ENABLE
DYNALIB_FN(BASE_IDX5 + 0, hal_usb, HAL_USB_Set_Vendor_Request_State_Callback, void(HAL_USB_Vendor_Request_State_Callback, void*))
#endif

DYNALIB_END(hal_usb)

#undef BASE_IDX
#undef BASE_IDX1
#undef BASE_IDX2
#undef BASE_IDX3
#undef BASE_IDX4
#undef BASE_IDX5

#endif  /* HAL_DYNALIB_USB_H */
