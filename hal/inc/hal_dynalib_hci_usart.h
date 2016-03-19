/**
 ******************************************************************************
 * @file    hal_dynalib_hci_usart.h
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

#ifndef HAL_DYNALIB_HCI_USART_H
#define HAL_DYNALIB_HCI_USART_H

#if PLATFORM_ID == 88 // Duo

#include "dynalib.h"
#include "usb_config_hal.h"

#ifdef DYNALIB_EXPORT
#include "hci_usart_hal.h"
#endif

// WARNING
// The order of functions must not be changed or older applications will break
// when used with newer system firmware.
// Function signatures shouldn't be changed other than changing pointer types.
// New HAL functions must be added to the end of this list.
// GNINRAW

DYNALIB_BEGIN(hal_hci_usart)

DYNALIB_FN(0, hal_hci_usart, HAL_HCI_USART_registerReceiveHandler, void(ReceiveHandler_t))
DYNALIB_FN(1, hal_hci_usart, HAL_HCI_USART_receiveEvent, void(void))
DYNALIB_FN(2, hal_hci_usart, HAL_HCI_USART_downloadFirmeare, int32_t(HAL_HCI_USART_Serial))
DYNALIB_FN(3, hal_hci_usart, HAL_HCI_USART_Init, void(HAL_HCI_USART_Serial, HCI_USART_Ring_Buffer*, HCI_USART_Ring_Buffer*))
DYNALIB_FN(4, hal_hci_usart, HAL_HCI_USART_Begin, void(HAL_HCI_USART_Serial, uint32_t))
DYNALIB_FN(5, hal_hci_usart, HAL_HCI_USART_End, void(HAL_HCI_USART_Serial))
DYNALIB_FN(6, hal_hci_usart, HAL_HCI_USART_Write_Data, int32_t(HAL_HCI_USART_Serial, uint8_t))
DYNALIB_FN(7, hal_hci_usart, HAL_HCI_USART_Write_Buffer, int32_t(HAL_HCI_USART_Serial, const uint8_t*, uint16_t))
DYNALIB_FN(8, hal_hci_usart, HAL_HCI_USART_Available_Data, int32_t(HAL_HCI_USART_Serial))
DYNALIB_FN(9, hal_hci_usart, HAL_HCI_USART_Read_Data, int32_t(HAL_HCI_USART_Serial))
DYNALIB_FN(10, hal_hci_usart, HAL_HCI_USART_RestartSend, void(HAL_HCI_USART_Serial))

DYNALIB_END(hal_hci_usart)

#endif

#endif	/* HAL_DYNALIB_USART_H */

