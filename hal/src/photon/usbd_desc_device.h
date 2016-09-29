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
#define USBD_PRODUCT_STRING             "P1"
#else
#define USBD_PRODUCT_STRING             "Photon"
#endif

#define USBD_CONFIGURATION_STRING    "Composite"
// Unused
#define USBD_INTERFACE_STRING        ""
