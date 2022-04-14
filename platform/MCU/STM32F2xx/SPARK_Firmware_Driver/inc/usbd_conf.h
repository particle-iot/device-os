/**
 ******************************************************************************
 * @file    usbd_conf.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    21-Oct-2014
 * @brief   USB Device configuration file
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  Copyright 2012 STMicroelectronics
  http://www.st.com/software_license_agreement_liberty_v2

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
#ifndef __USBD_CONF__H__
#define __USBD_CONF__H__

/* Includes ------------------------------------------------------------------*/
#include "usb_conf.h"
#include "hw_config.h"

/* Maximum number of supported media (Flash, sFlash & DCT) */
#ifdef HAS_SERIAL_FLASH
#define MAX_USED_MEDIA                  3
#else
#define MAX_USED_MEDIA                  2
#endif

/*
 * Disabling multiple configurations (100mA and 500mA) due to this Windows note:
 * 
 * USBCCGP will not load on a multi-config device by default
 * because the hub driver doesn't create a "USB\COMPOSITE" PNP ID for a composite device
 * if it has multiple configurations. However, you can write your own INF
 * that matches a device-specific PNP ID to get USBCCGP to load as your device's function driver.
 */
#define USBD_CFG_MAX_NUM                1    // ^^^
#define USBD_ITF_MAX_NUM                10
#define USBD_DFU_INT_NUM                MAX_USED_MEDIA
#define USB_MAX_STR_DESC_SIZ            255
#define USB_SUPPORT_USER_STRING_DESC

#define USBD_EP0_MAX_PACKET_SIZE        64

#define USBD_SELF_POWERED

/* USB DFU Class Layer Parameter */
#define XFERSIZE                        4096   /* Max DFU Packet Size   = 16384 bytes */

#define DFU_IN_EP                       0x80
#define DFU_OUT_EP                      0x00

/* Flash memory address from where user application will be loaded
   This address represents the DFU code protected against write and erase operations.*/
#define APP_DEFAULT_ADD                 CORE_FW_ADDRESS

#ifdef HAS_SERIAL_FLASH
#define DFU_MAL_SUPPORT_sFLASH
#endif

/* Uncomment this define to impelement OTP memory interface */
//#define DFU_MAL_SUPPORT_OTP

/* Uncomment this define to implement template memory interface  */
/* #define DFU_MAL_SUPPORT_MEM */

/* Update this define to modify the DFU protected area.
   This area corresponds to the memory where the DFU code should be loaded
   and cannot be erased or everwritten by DFU application. */
#define DFU_MAL_IS_PROTECTED_AREA(add)    (uint8_t)(((add >= 0x08000000) && (add < (APP_DEFAULT_ADD)))? 1:0)

#define TRANSFER_SIZE_BYTES(sze)          ((uint8_t)(sze)), /* XFERSIZEB0 */\
                                          ((uint8_t)(sze >> 8)) /* XFERSIZEB1 */

/* USB CDC/VCP Class Layer Parameter */
#define CDC0_IN_EP                       0x81  /* EP1 for data IN */
#define CDC0_OUT_EP                      0x01  /* EP1 for data OUT */
#define CDC1_IN_EP                       0x82  /* EP2 for data IN */
#define CDC1_OUT_EP                      0x02  /* EP2 for data OUT */

/* XXX: Using dummy CMD endpoints on Electron, since Electron uses USB OTG FS peripheral,
 * which has only 3 IN and 3 OUT endpoints, which is not enough to run 2 CDC + HID
 * or even 2 CDC.
 */
#ifndef USE_USB_OTG_FS
# define CDC0_CMD_EP                      0x83  /* EP3 for CDC commands */
# define CDC1_CMD_EP                      0x84  /* EP4 for CDC commands */
#else
# define CDC0_CMD_EP                      0x8A  /* dummy EP10 for CDC commands */
# define CDC0_CMD_EP_COMPAT               0x83  /* EP3 used for compatibility */
# define CDC1_CMD_EP                      0x8B  /* dummy EP11 for CDC commands */
//# define CDC_CMD_EP_SHARED
#endif

/* USB HID Class Layer Parameters */
#ifndef USE_USB_OTG_FS
# define HID_IN_EP                        0x85  /* EP5 */
#else
# define HID_IN_EP                        0x83  /* EP3 */
#endif
#define HID_IN_PACKET                    16
#define HID_OUT_EP                       0x03
#define HID_OUT_PACKET                   16

/* CDC Endpoints parameters: you can fine tune these values depending on the needed baudrates and performance. */
#define CDC_DATA_MAX_PACKET_SIZE        64   /* Endpoint IN & OUT Packet size */
#define CDC_CMD_PACKET_SZE              8    /* Control Endpoint Packet size */

#define CDC_IN_FRAME_INTERVAL           1    /* Number of micro-frames between IN transfers */

#define APP_FOPS                        APP_fops

/* The following structure groups all needed parameters to be configured for the
   ComPort. These parameters can modified on the fly by the host through CDC class
   command class requests. */
typedef struct
{
  uint32_t bitrate;
  uint8_t  format;
  uint8_t  paritytype;
  uint8_t  datatype;
} LINE_CODING;

typedef void (*linecoding_bitrate_handler)(uint32_t bitrate);
void SetLineCodingBitRateHandler(linecoding_bitrate_handler handler);

#endif //__USBD_CONF__H__

