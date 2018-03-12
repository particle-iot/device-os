/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/* These are the default Bluetooth address, name and class-of-device */
#define WICED_BLUETOOTH_DEVICE_NAME    "WICED AV Sink"
#define WICED_BLUETOOTH_DEVICE_ADDRESS "\x11\x22\x33\x44\x55\x66"
/* Service class: Audio(speakers, microphones,...), Major device class: Audio/Video, Minor device class: Portable-Audio */
/* See here for more details-  https://www.bluetooth.org/en-us/specification/assigned-numbers/baseband */
#define WICED_BLUETOOTH_DEVICE_CLASS   "\x20\x04\x1C"

/* Set this FLAG to WICED_TRUE to enable Simple Secure Pairing Debug mode */
#define WICED_BLUETOOTH_SSP_DEBUG_MODE WICED_FALSE

/*Maximum number of paired remote device info stored in NV*/
#define WICED_BLUETOOTH_NV_MAX_LAST_PAIRED_DEVICES 8


/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /*extern "C" */
#endif
