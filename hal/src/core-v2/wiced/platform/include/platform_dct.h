/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 * Defines Device Configuration Table (DCT) structures
 */
#pragma once

#include <stdint.h>
#include "wwd_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#ifndef PRIVATE_KEY_SIZE
#define PRIVATE_KEY_SIZE  (2*1024)
#endif

#ifndef CERTIFICATE_SIZE
#define CERTIFICATE_SIZE  (4*1024)
#endif

#ifndef CONFIG_AP_LIST_SIZE
#define CONFIG_AP_LIST_SIZE   (5)
#endif

#ifndef COOEE_KEY_SIZE
#define COOEE_KEY_SIZE   (16)
#endif

#define CONFIG_VALIDITY_VALUE        0xCA1BDF58

#define DCT_FR_APP_INDEX            ( 0 )
#define DCT_DCT_IMAGE_INDEX         ( 1 )
#define DCT_OTA_APP_INDEX           ( 2 )
#define DCT_FILESYSTEM_IMAGE_INDEX  ( 3 )
#define DCT_WIFI_FIRMWARE_INDEX     ( 4 )
#define DCT_APP0_INDEX              ( 5 )
#define DCT_APP1_INDEX              ( 6 )
#define DCT_APP2_INDEX              ( 7 )

#define DCT_MAX_APP_COUNT      ( 8 )

#define DCT_APP_LOCATION_OF(APP_INDEX) (uint32_t )((uint8_t *)&((platform_dct_header_t *)0)->apps_locations + sizeof(image_location_t) * ( APP_INDEX ))
//#define DCT_APP_LOCATION_OF(APP_INDEX) (uint32_t )(OFFSETOF(platform_dct_header_t, apps_locations) + sizeof(image_location_t) * ( APP_INDEX ))

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef void (*dct_load_app_func_t)( void );

typedef struct
{
        uint32_t location;
        uint32_t size;
} fixed_location_t;


typedef enum
{
    NONE,
    INTERNAL,
    EXTERNAL_FIXED_LOCATION,
    EXTERNAL_FILESYSTEM_FILE,
} image_location_id_t;

typedef struct
{
    image_location_id_t id;
    union
    {
        fixed_location_t internal_fixed;
        fixed_location_t external_fixed;
        char     filesytem_filename[32];
    } detail;
} image_location_t;

typedef struct
{
        image_location_t source;
        image_location_t destination;
        char load_once;
        char valid;
} load_details_t;

typedef struct
{
        load_details_t load_details;

        uint32_t entry_point;
} boot_detail_t;

typedef struct
{
        unsigned long full_size;
        unsigned long used_size;
        char write_incomplete;
        char is_current_dct;
        char app_valid;
        char mfg_info_programmed;
        unsigned long magic_number;
        boot_detail_t boot_detail;
        image_location_t apps_locations[ DCT_MAX_APP_COUNT ];
        void (*load_app_func)( void ); /* WARNING: TEMPORARY */
} platform_dct_header_t;

typedef struct
{
        char manufacturer[ 32 ];
        char product_name[ 32 ];
        char BOM_name[ 24 ];
        char BOM_rev[ 8 ];
        char serial_number[ 20 ];
        char manufacture_date_time[ 20 ];
        char manufacture_location[ 12 ];
        char bootloader_version[ 8 ];
} platform_dct_mfg_info_t;

typedef struct
{
        wiced_ap_info_t details;
        uint8_t security_key_length;
        char security_key[ 64 ];
} wiced_config_ap_entry_t;

typedef struct
{
        wiced_ssid_t SSID;
        wiced_security_t security;
        uint8_t channel;
        uint8_t security_key_length;
        char security_key[ 64 ];
        uint32_t details_valid;
} wiced_config_soft_ap_t;

typedef struct
{
        wiced_bool_t device_configured;
        wiced_config_ap_entry_t stored_ap_list[ CONFIG_AP_LIST_SIZE ];
        wiced_config_soft_ap_t soft_ap_settings;
        wiced_config_soft_ap_t config_ap_settings;
        wiced_country_code_t country_code;
        wiced_mac_t mac_address;
        uint8_t padding[ 2 ]; /* to ensure 32bit aligned size */
} platform_dct_wifi_config_t;

typedef struct
{
        char private_key[ PRIVATE_KEY_SIZE ];
        char certificate[ CERTIFICATE_SIZE ];
        uint8_t cooee_key[ COOEE_KEY_SIZE ];
} platform_dct_security_t;

typedef struct
{
        platform_dct_header_t dct_header;
        platform_dct_mfg_info_t mfg_info;
        platform_dct_security_t security_credentials;
        platform_dct_wifi_config_t wifi_config;
} platform_dct_data_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif
