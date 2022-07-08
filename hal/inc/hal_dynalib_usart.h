/**
 ******************************************************************************
 * @file    hal_dynalib_usart.h
 * @authors mat
 * @date    04 March 2015
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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

#ifndef HAL_DYNALIB_USART_H
#define HAL_DYNALIB_USART_H

#include "dynalib.h"
#include "hal_platform.h"
#include "usb_config_hal.h"

#ifdef DYNALIB_EXPORT
#include "usart_hal.h"
#include "usb_hal.h"
#endif

// WARNING
// The order of functions must not be changed or older applications will break
// when used with newer system firmware.
// Function signatures shouldn't be changed other than changing pointer types.
// New HAL functions must be added to the end of this list.
// GNINRAW

DYNALIB_BEGIN(hal_usart)

#if defined (USB_CDC_ENABLE) && !HAL_PLATFORM_NRF52840 && !HAL_PLATFORM_RTL872X
DYNALIB_FN(0, hal_usart, USB_USART_Init, void(uint32_t))
DYNALIB_FN(1, hal_usart, USB_USART_Available_Data, uint8_t(void))
DYNALIB_FN(2, hal_usart, USB_USART_Receive_Data, int32_t(uint8_t))
DYNALIB_FN(3, hal_usart, USB_USART_Send_Data, void(uint8_t))
DYNALIB_FN(4, hal_usart, USB_USART_Baud_Rate, unsigned(void))
DYNALIB_FN(5, hal_usart, USB_USART_LineCoding_BitRate_Handler, void(void(*)(uint32_t)))
#define BASE_IDX 6 // Base index for all subsequent functions
#else
#define BASE_IDX 0
#endif

DYNALIB_FN(BASE_IDX + 0, hal_usart, hal_usart_init, void(hal_usart_interface_t, hal_usart_ring_buffer_t*, hal_usart_ring_buffer_t*))
DYNALIB_FN(BASE_IDX + 1, hal_usart, hal_usart_begin, void(hal_usart_interface_t, uint32_t))
DYNALIB_FN(BASE_IDX + 2, hal_usart, hal_usart_end, void(hal_usart_interface_t))
DYNALIB_FN(BASE_IDX + 3, hal_usart, hal_usart_write, uint32_t(hal_usart_interface_t, uint8_t))
DYNALIB_FN(BASE_IDX + 4, hal_usart, hal_usart_available, int32_t(hal_usart_interface_t))
DYNALIB_FN(BASE_IDX + 5, hal_usart, hal_usart_read, int32_t(hal_usart_interface_t))
DYNALIB_FN(BASE_IDX + 6, hal_usart, hal_usart_peek, int32_t(hal_usart_interface_t))
DYNALIB_FN(BASE_IDX + 7, hal_usart, hal_usart_flush, void(hal_usart_interface_t))
DYNALIB_FN(BASE_IDX + 8, hal_usart, hal_usart_is_enabled, bool(hal_usart_interface_t))
DYNALIB_FN(BASE_IDX + 9, hal_usart, hal_usart_half_duplex, void(hal_usart_interface_t, bool))
DYNALIB_FN(BASE_IDX + 10, hal_usart, hal_usart_available_data_for_write, int32_t(hal_usart_interface_t))

#if defined (USB_CDC_ENABLE) && !HAL_PLATFORM_NRF52840 && !HAL_PLATFORM_RTL872X
DYNALIB_FN(BASE_IDX + 11, hal_usart, USB_USART_Available_Data_For_Write, int32_t(void))
DYNALIB_FN(BASE_IDX + 12, hal_usart, USB_USART_Flush_Data, void(void))
#define BASE_IDX2 (BASE_IDX+13)
#else
#define BASE_IDX2 (BASE_IDX+11)
#endif

DYNALIB_FN(BASE_IDX2 + 0, hal_usart, hal_usart_begin_config, void(hal_usart_interface_t serial, uint32_t baud, uint32_t config, void *ptr))
DYNALIB_FN(BASE_IDX2 + 1, hal_usart, hal_usart_write_nine_bits, uint32_t(hal_usart_interface_t serial, uint16_t data))
DYNALIB_FN(BASE_IDX2 + 2, hal_usart, hal_usart_send_break, void(hal_usart_interface_t, void*))
DYNALIB_FN(BASE_IDX2 + 3, hal_usart, hal_usart_break_detected, uint8_t(hal_usart_interface_t))
DYNALIB_FN(BASE_IDX2 + 4, hal_usart, hal_usart_sleep, int(hal_usart_interface_t serial, bool, void*))
DYNALIB_FN(BASE_IDX2 + 5, hal_usart, hal_usart_init_ex, int(hal_usart_interface_t, const hal_usart_buffer_config_t*, void*))

DYNALIB_END(hal_usart)

#undef BASE_IDX
#undef BASE_IDX2

#endif /* HAL_DYNALIB_USART_H */

