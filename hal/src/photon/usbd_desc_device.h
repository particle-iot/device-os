/**
 ******************************************************************************
 * @file    usbd_desc.c
 * @author  Satish Nair
 * @version V1.0.0
 * @date    05-Nov-2014
 * @brief   This file provides the USBD descriptors and string formating method.
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

#include "platforms.h"

/* Includes ------------------------------------------------------------------*/

/* USB Device String Definitions ---------------------------------------------*/

#define USBD_LANGID_STRING              0x0409  //U.S. English
#define USBD_MANUFACTURER_STRING        "Particle"

#if PLATFORM_ID==PLATFORM_P1
#define USBD_PRODUCT_HS_STRING          "P1 with WiFi"
#define USBD_SERIALNUMBER_HS_STRING     "00000000050B"

#define USBD_PRODUCT_FS_STRING          "P1 with WiFi"
#define USBD_SERIALNUMBER_FS_STRING     "00000000050C"
#else
#define USBD_PRODUCT_HS_STRING          "Photon with WiFi"
#define USBD_SERIALNUMBER_HS_STRING     "00000000050B"

#define USBD_PRODUCT_FS_STRING          "Photon with WiFi"
#define USBD_SERIALNUMBER_FS_STRING     "00000000050C"
#endif

#define USBD_CONFIGURATION_HS_STRING    "VCP Config"
#define USBD_INTERFACE_HS_STRING        "VCP Interface"

#define USBD_CONFIGURATION_FS_STRING    "VCP Config"
#define USBD_INTERFACE_FS_STRING        "VCP Interface"

