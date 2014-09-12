/**
 ******************************************************************************
 * @file    usb_hal.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    10-Sept-2014
 * @brief   USB Virtual COM Port and HID device HAL
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
#ifndef __USB_HAL_H
#define __USB_HAL_H

/* Includes ------------------------------------------------------------------*/
#include "core_hal.h"
#include "usb_conf.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macros ------------------------------------------------------------*/

/* We are temporary defining this macro over here */
/* This could also be passed via -D option in build script */
#define SPARK_USB_SERIAL        //Default is Virtual COM Port
//#define SPARK_USB_MOUSE
//#define SPARK_USB_KEYBOARD

/* USB Config : IMR_MSK */
/* mask defining which events has to be handled */
/* by the device application software */
#define IMR_MSK (CNTR_CTRM  | \
                 CNTR_WKUPM | \
                 CNTR_SUSPM | \
                 CNTR_ERRM  | \
                 CNTR_SOFM  | \
                 CNTR_ESOFM | \
                 CNTR_RESETM  \
)

#define USART_RX_DATA_SIZE      256

/* Exported functions ------------------------------------------------------- */
#if defined (USB_CDC_ENABLE) || defined (USB_HID_ENABLE)
void SPARK_USB_Setup(void);
void Get_SerialNum(void);
#endif

#ifdef USB_CDC_ENABLE
void USB_USART_Init(uint32_t baudRate);
uint8_t USB_USART_Available_Data(void);
int32_t USB_USART_Receive_Data(void);
void USB_USART_Send_Data(uint8_t Data);
#endif

#ifdef USB_HID_ENABLE
void USB_HID_Send_Report(void *pHIDReport, size_t reportSize);
#endif

#endif  /* __USB_HAL_H */
