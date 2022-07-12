/**
 ******************************************************************************
 * @file    spark_wiring_version.h
 * @authors Matthew McGowan
 * @date    02 March 2015
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

#ifndef SPARK_WIRING_VERSION_H
#define	SPARK_WIRING_VERSION_H

#include "spark_protocol_functions.h"

struct __ApplicationProductID {
    __ApplicationProductID(product_id_t id) {
        spark_protocol_set_product_id(spark_protocol_instance(), id);
    }
};

struct __ApplicationProductVersion {
    __ApplicationProductVersion(product_firmware_version_t version) {
        spark_protocol_set_product_firmware_version(spark_protocol_instance(), version);
    }
};

#ifdef PRODUCT_ID
#undef PRODUCT_ID
#endif

#define PRODUCT_ID(x) _Pragma ("GCC error \"The PRODUCT_ID macro must be removed from your firmware source code. \
The same compiled firmware binary may be used in multiple products that share the same platform and functionality.\"")

/*
 * PRODUCT_ID and PRODUCT_VERSION will be added to the beginning of the suffix by the linker.
 *
 * The following is an example of where the two 16-bit values live at the end of the binary,
 * in the 40 byte suffix.
 *
 * This is for a Boron PLATFORM_ID 13, and code with version 3.
 * product_id (0D,00) and product_version (03,00)
 *
 * 0D,00,03,00,00,00,66,B6,B7,8F,3E,A4,8E,10,1B,30,FE,2A,C1,29,22,08,
 * 5A,8A,43,ED,93,1C,DA,3A,8D,50,13,48,80,D3,7F,74,28,00,4B,C2,BA,F6
 *
 * Here's the beautified view:
 *
 * suffixInfo: {
 *    productId: 13,
 *    productVersion: 3,
 *    fwUniqueId: '66b6b78f3ea48e101b30fe2ac12922085a8a43ed931cda3a8d50134880d37f74',
 *    reserved: 0,
 *    suffixSize: 40,
 *    crcBlock: '4bc2baf6'
 * }
 *
 */
#if PLATFORM_ID != PLATFORM_GCC
#define PRODUCT_VERSION(x) \
        __ApplicationProductVersion __appProductVersion(x); \
        __attribute__((externally_visible, section(".modinfo.product_version"))) uint16_t __system_product_version = (x); \
        /* PRODUCT_ID used to do the following with a dynamic ID, but now we hardcode the PLATFORM_ID as the PRODUCT_ID. */ \
        __ApplicationProductID __appProductID(PLATFORM_ID); \
        __attribute__((externally_visible, section(".modinfo.product_id"))) uint16_t __system_product_id = (PLATFORM_ID);
#else
#define PRODUCT_VERSION(x) _Pragma ("GCC warning \"The PRODUCT_VERSION macro is ignored on the GCC platform. \
Use the --product_version argument instead.\"")
#endif // PLATFORM_ID != PLATFORM_GCC

#endif	/* SPARK_WIRING_VERSION_H */
