/**
 ******************************************************************************
 * @file    usbd_conf.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    21-Oct-2014
 * @brief   USB Device configuration file
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

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

#define USBD_CFG_MAX_NUM                1
#define USBD_ITF_MAX_NUM                MAX_USED_MEDIA
#define USB_MAX_STR_DESC_SIZ            200 
#define USB_SUPPORT_USER_STRING_DESC

#define USBD_SELF_POWERED               

#define XFERSIZE                        1024   /* Max DFU Packet Size   = 1024 bytes */

#define DFU_IN_EP                       0x80
#define DFU_OUT_EP                      0x00

/* Maximum number of supported media (Flash only) */
#define MAX_USED_MEDIA                  1 //2 for both Flash and OTP

/* Flash memory address from where user application will be loaded 
   This address represents the DFU code protected against write and erase operations.*/
#define APP_DEFAULT_ADD                 CORE_FW_ADDRESS

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

#endif //__USBD_CONF__H__

