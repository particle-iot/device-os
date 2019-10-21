/**
  ******************************************************************************
  * @file    usb_desc.h
  * @author  Saitsh Nair
  * @version V1.0.0
  * @date    30-April-2013
  * @brief   Descriptor Header for Device Firmware Upgrade (DFU)
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
#ifndef __USB_DESC_H
#define __USB_DESC_H
#include "platform_config.h"

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
#define DFU_SIZ_DEVICE_DESC             18

#ifdef SPARK_SFLASH_ENABLE
  #define DFU_SIZ_CONFIG_DESC			36
#else
  #define DFU_SIZ_CONFIG_DESC			27
#endif

#define DFU_SIZ_STRING_LANGID           4
#define DFU_SIZ_STRING_VENDOR           38
#define DFU_SIZ_STRING_PRODUCT          20
#define DFU_SIZ_STRING_SERIAL           52
#define DFU_SIZ_STRING_INTERFACE0       98		/* Flash Bank 0 */
#define DFU_SIZ_STRING_INTERFACE1       98		/* SPI Flash */

extern  uint8_t DFU_DeviceDescriptor[DFU_SIZ_DEVICE_DESC];
extern  uint8_t DFU_ConfigDescriptor[DFU_SIZ_CONFIG_DESC];
extern  uint8_t DFU_StringLangId     [DFU_SIZ_STRING_LANGID];
extern  uint8_t DFU_StringVendor     [DFU_SIZ_STRING_VENDOR];
extern  uint8_t DFU_StringProduct    [DFU_SIZ_STRING_PRODUCT];
extern  uint8_t DFU_StringSerial     [DFU_SIZ_STRING_SERIAL];
extern  uint8_t DFU_StringInterface0 [DFU_SIZ_STRING_INTERFACE0];
extern  uint8_t DFU_StringInterface1 [DFU_SIZ_STRING_INTERFACE1];

#define bMaxPacketSize0             0x40     /* bMaxPacketSize0 = 64 bytes   */

#define wTransferSize               0x0400   /* wTransferSize   = 1024 bytes */
/* bMaxPacketSize0 <= wTransferSize <= 32kbytes */
#define wTransferSizeB0             0x00
#define wTransferSizeB1             0x04

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/* External variables --------------------------------------------------------*/

#endif /* __USB_DESC_H */

