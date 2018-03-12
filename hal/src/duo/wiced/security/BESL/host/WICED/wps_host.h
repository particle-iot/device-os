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

#include "wwd_structures.h"
#include "wwd_wlioctl.h"
#include "wwd_debug.h"
#include "wiced_utilities.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *            Constants
 ******************************************************/
#define WPS_ASSERT(x)

/******************************************************
 *            Enumerations
 ******************************************************/

typedef enum
{
    BESL_WPS_PBC_MODE = 1,
    BESL_WPS_PIN_MODE = 2
} besl_wps_mode_t;

typedef enum
{
    BESL_WPS_DEVICE_COMPUTER               = 1,
    BESL_WPS_DEVICE_INPUT                  = 2,
    BESL_WPS_DEVICE_PRINT_SCAN_FAX_COPY    = 3,
    BESL_WPS_DEVICE_CAMERA                 = 4,
    BESL_WPS_DEVICE_STORAGE                = 5,
    BESL_WPS_DEVICE_NETWORK_INFRASTRUCTURE = 6,
    BESL_WPS_DEVICE_DISPLAY                = 7,
    BESL_WPS_DEVICE_MULTIMEDIA             = 8,
    BESL_WPS_DEVICE_GAMING                 = 9,
    BESL_WPS_DEVICE_TELEPHONE              = 10,
    BESL_WPS_DEVICE_AUDIO                  = 11,
    BESL_WPS_DEVICE_OTHER                  = 0xFF,
} besl_wps_device_category_t;

/******************************************************
 *             Structures
 ******************************************************/

typedef void (*wps_scan_handler_t)(wl_escan_result_t* result, void* user_data);

typedef struct
{
    besl_wps_device_category_t device_category;
    uint16_t sub_category;
    char*    device_name;
    char*    manufacturer;
    char*    model_name;
    char*    model_number;
    char*    serial_number;
    uint32_t config_methods;
    uint32_t os_version;
    uint16_t authentication_type_flags;
    uint16_t encryption_type_flags;
    uint8_t  add_config_methods_to_probe_resp;
} besl_wps_device_detail_t;

typedef struct
{
    wiced_ssid_t       ssid;
    wiced_security_t security;
    uint8_t          passphrase[64];
    uint16_t         passphrase_length;
} besl_wps_credential_t;

typedef enum
{
    BESL_WPS_CONFIG_USBA                  = 0x0001,
    BESL_WPS_CONFIG_ETHERNET              = 0x0002,
    BESL_WPS_CONFIG_LABEL                 = 0x0004,
    BESL_WPS_CONFIG_DISPLAY               = 0x0008,
    BESL_WPS_CONFIG_EXTERNAL_NFC_TOKEN    = 0x0010,
    BESL_WPS_CONFIG_INTEGRATED_NFC_TOKEN  = 0x0020,
    BESL_WPS_CONFIG_NFC_INTERFACE         = 0x0040,
    BESL_WPS_CONFIG_PUSH_BUTTON           = 0x0080,
    BESL_WPS_CONFIG_KEYPAD                = 0x0100,
    BESL_WPS_CONFIG_VIRTUAL_PUSH_BUTTON   = 0x0280,
    BESL_WPS_CONFIG_PHYSICAL_PUSH_BUTTON  = 0x0480,
    BESL_WPS_CONFIG_VIRTUAL_DISPLAY_PIN   = 0x2008,
    BESL_WPS_CONFIG_PHYSICAL_DISPLAY_PIN  = 0x4008
} besl_wps_configuration_method_t;



/******************************************************
 *             Function declarations
 ******************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif
