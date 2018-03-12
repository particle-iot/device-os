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

#include "wiced_result.h"
#include "wwd_assert.h"

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
/* USB Host Event */
#define USB_HOST_EVENT_UNDEFINED                    0
#define USB_HOST_EVENT_HID_DEVICE_INSERTION         1
#define USB_HOST_EVENT_HID_DEVICE_REMOVAL           2
#define USB_HOST_EVENT_STORAGE_DEVICE_INSERTION     11
#define USB_HOST_EVENT_STORAGE_DEVICE_REMOVAL       12
#define USB_HOST_EVENT_AUDIO_DEVICE_INSERTION       21
#define USB_HOST_EVENT_AUDIO_DEVICE_REMOVAL         22

/* USB Disk storage name for filesystem mount */
#define USB_DISK_STORAGE_MOUNT_NAME                 "usb0"

/* USB maximum number of audio device handle */
#define USB_AUDIO_DEVICE_HANDLE_MAX                 5

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/
/* USB Host Event Callback function */
typedef wiced_result_t (*wiced_usb_host_event_callback_t)( uint32_t evt, void *param1, void *param2 );

/******************************************************
 *                    Structures
 ******************************************************/
/* USB User Config */
typedef struct
{
    uint32_t                        host_max_class;
    uint32_t                        host_max_hcd;
    uint32_t                        host_max_devices;
    uint32_t                        host_max_ed;
    uint32_t                        host_max_td;
    uint32_t                        host_max_iso_td;
    wiced_usb_host_event_callback_t host_event_callback;
} wiced_usb_user_config_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

/** Initialises USB Host stack
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_usb_host_init( wiced_usb_user_config_t *config );


/** De-initialises USB Host stack
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_usb_host_deinit( void );



#ifdef __cplusplus
} /*extern "C" */
#endif
