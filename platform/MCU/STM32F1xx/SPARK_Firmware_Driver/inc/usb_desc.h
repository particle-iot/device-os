/**
  ******************************************************************************
  * @file    usb_desc.h
  * @author  Satish Nair
  * @version V1.0.0
  * @date    24-April-2013
  * @brief   Descriptor Header for USB CDC-HID Device
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

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported define -----------------------------------------------------------*/
#define USB_DEVICE_DESCRIPTOR_TYPE              0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE       0x02
#define USB_STRING_DESCRIPTOR_TYPE              0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE           0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE            0x05

#define CDC_DATA_SIZE                           64
#define CDC_INT_SIZE                            8

#ifdef USB_CDC_ENABLE
#define CDC_SIZ_DEVICE_DESC                     18
#define CDC_SIZ_CONFIG_DESC                     67
#endif

#define HID_DESCRIPTOR_TYPE                     0x21
#define HID_SIZ_HID_DESC                        0x09
#define HID_OFF_HID_DESC                        0x12

#ifdef USB_HID_ENABLE
#define HID_SIZ_DEVICE_DESC                     18
#define HID_SIZ_CONFIG_DESC                     34

#if defined (SPARK_USB_MOUSE)
#define HID_SIZ_REPORT_DESC                     52
#elif defined (SPARK_USB_KEYBOARD)
#define HID_SIZ_REPORT_DESC                     45
#endif
#endif

#define USB_SIZ_STRING_LANGID                   4
#define USB_SIZ_STRING_VENDOR                   38
#define USB_SIZ_STRING_PRODUCT                  50
#define USB_SIZ_STRING_SERIAL                   52

#define STANDARD_ENDPOINT_DESC_SIZE             0x09

/* Exported functions ------------------------------------------------------- */
#ifdef USB_CDC_ENABLE
extern const uint8_t CDC_DeviceDescriptor[CDC_SIZ_DEVICE_DESC];
extern const uint8_t CDC_ConfigDescriptor[CDC_SIZ_CONFIG_DESC];
#endif

#ifdef USB_HID_ENABLE
extern const uint8_t HID_DeviceDescriptor[HID_SIZ_DEVICE_DESC];
extern const uint8_t HID_ConfigDescriptor[HID_SIZ_CONFIG_DESC];
extern const uint8_t HID_ReportDescriptor[HID_SIZ_REPORT_DESC];
#endif

extern const uint8_t USB_StringLangID[USB_SIZ_STRING_LANGID];
extern const uint8_t USB_StringVendor[USB_SIZ_STRING_VENDOR];
extern const uint8_t USB_StringProduct[USB_SIZ_STRING_PRODUCT];
extern uint8_t USB_StringSerial[USB_SIZ_STRING_SERIAL];

#endif /* __USB_DESC_H */
