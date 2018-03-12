/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 * Defines Device Configuration Table (DCT) structures
 *
 * Instructions for adding to the DCT
 *
 * 1) Basic Rules:
 *
 *  WE CANNOT CHANGE THE LAYOUT OF platform_dct_header_t FROM THE SDK VERSION THE BOOTLOADER WAS BUILT WITH
 *
 *    ---> DO NOT CHANGE platform_dct_header_t !!! <---
 *
 *      The Bootloader only knows about platform_dct_header_t for the SDK it was built upon.
 *      The Bootloader is not upgrade-able, so platform_dct_header_t MUST match the platform_dct_header_t
 *      and all fields be used in the same manner as the Bootloader's SDK.
 *
 *    ---> DO NOT CHANGE platform_dct_header_t !!! <---
 *
 *      If you need to add something that you believe should be in platform_dct_header_t.
 *      see if it makes sense to put the field into platform_dct_version_h. Doing this will make
 *      maintenance much much simpler.
 *
 *    ---> ONLY ADD DATA TO THE END of platform_dct_data_t after platform_dct_version_h !!! <---
 *
 *    Adding/removing/changing fields in existing structures will require
 *    code to update from the previous version of the DCT to the new version you are creating.
 *
 *    All sub-structures MUST be a multiple of 4 bytes in size.
 *
 *    All previously optional sub-structures are always defined. This simplifies updating SDKs in the future.
 *
 *    If you are adding an "optional" structure, see how bt, p2p, and ota2 are handled below using flags to indicate
 *    if the structures are in use or not.
 *              WICED_DCT_INCLUDE_BT_CONFIG
 *              WICED_DCT_INCLUDE_P2P_CONFIG
 *              WICED_DCT_INCLUDE_OTA2_CONFIG
 *
 *  2) Steps to ADD data to the end of platform_dct_data_t
 *     - Encapsulate the new data in a structure (or use an existing structure if appropriate)
 *     - Add the new structure inside platform_dct_data_t, after the last structure (currently platform_dct_version_h)
 *     - Add a new DCT_BOOTLOADER_SDK_CURRENT to the #defines below and comment changes
 *     - Add code to platform_dct_upgrade.c to support changes from the previous DCT version to the new DCT version
 *
 *  3) If you are deprecating a field in an existing structure
 *      - change the name of the field to "deprecated_xxxx" where xxx is teh previous field's name
 *      - This will keep the documentation of the change as part of the code, which may be important in future
 *
 *  4) If you must change or add fields in an existing DCT structure
 *     - provide the previous structure define with the SDK version at the end of the structure name. See the file
 *       platform_dct_old_sdk.h in this directory.
 *        (i.e. platform_dct_sdk_ver_t becomes platform_dct_sdk_ver_sdk_x_x_x_t)
 *     - modify the new structure with your changes
 *     - Add a new DCT_BOOTLOADER_SDK_CURRENT to the #defines below and comment changes
 *     - Add code to platform_dct_upgrade.c to support changing the previous
 *       DCT version to the new DCT version
 *     - Add support to call the upgrade routine in wiced_dct_external_common.c and wiced_dct_internal_common.c in
 *       functions wiced_dct_external_dct_update() and wiced_dct_internal_dct_update() respectively.
 *
 * Instructions for an application that is going to be upgraded on a system
 * built with an older SDK.
 *
 *  - Define the SDK used when the ORIGINAL bootloader was built on the command line
 *    ex:
 *       ./make <applicaiton>-<platform> UPDATE_FROM_SDK=3_3_1
 *  - Define the optional substructures (if used) when upgrading an SDK before SDK-3.6.x //: TODO: which rev are we releasing this update to?
 *
 *
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include "wwd_structures.h"
#include "wiced_constants.h"

#ifdef DEFINE_BOOTLOADER
#define BOOTLOADER
#endif

#ifdef DEFINE_PARTICLE_DCT_COMPATIBILITY
#define PARTICLE_DCT_COMPATIBILITY
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Add new SDK version defines to this list
 */
#define DCT_BOOTLOADER_SDK_UNKNOWN  0x7fff /* large, non-negative (if interpreted as int) */
#define DCT_BOOTLOADER_SDK_3_0_1    0x0301 /* No support for pre-SDK-3.0.1 */
#define DCT_BOOTLOADER_SDK_3_1_0    0x0310 /* Changes from SDK-3.0.0 to SDK-3.1.0
                                            *  TODO: check - NONE ??
                                            */
#define DCT_BOOTLOADER_SDK_3_1_1    0x0311 /* Changes from SDK-3.1.0 to SDK-3.1.1
                                            * platform_dct_header_t
                                            *    Addition of apps_locations[ DCT_MAX_APP_COUNT ];
                                            */
#define DCT_BOOTLOADER_SDK_3_1_2    0x0312 /* Changes from SDK-3.1.1 to SDK-3.1.2
                                            * platform_dct_header_t
                                            *    optional padding added to end of structure (DCT_HEADER_ALIGN_SIZE)
                                            * OPTIONAL STRUCTS:
                                            *     platform_dct_bt_config_t
                                            *         New structure added as OPTIONAL
                                            *         (wrapped with WICED_DCT_INCLUDE_BT_CONFIG)
                                            */
#define DCT_BOOTLOADER_SDK_3_3_0    0x0330 /* Baseline
                                            * Changes from SDK-3.1.2 to SDK-3.3.0
                                            * platform_dct_ethernet_config_t
                                            *    New structure added
                                            * platform_dct_network_config_t
                                            *    New structure added
                                            *
                                            */
#define DCT_BOOTLOADER_SDK_3_3_1    0x0331 /* Changes from SDK-3.3.0 to SDK-3.3.1
                                            * platform_dct_network_config_t
                                            *    changed char             hostname[ HOSTNAME_SIZE + 1 ];
                                            *    to      wiced_hostname_t hostname;
                                            *
                                            */
#define DCT_BOOTLOADER_SDK_3_4_0    0x0340 /* Changes from SDK-3.3.1 to SDK-3.4.0
                                            * platform_dct_bt_config_t
                                            *   added bluetooth_device_class
                                            *   changed padding
                                            */
#define DCT_BOOTLOADER_SDK_3_5_1    0x0351 /* Changes from SDK-3.4.0 to SDK-3.5.1
                                            * OPTIONAL STRUCTS:
                                            *     platform_p2p_config_t
                                            *        New structure added as OPTIONAL
                                            *         (wrapped with WICED_DCT_INCLUDE_P2P_CONFIG)
                                            */
#define DCT_BOOTLOADER_SDK_3_5_2    0x0352 /* Changes from SDK-3.5.1 to SDK-3.5.2
                                            * platform_dct_header_t
                                            *      moved magic_number, write_incomplete
                                            *      added CRC, sequence number
                                            *      removed is_current_dct
                                            * OPTIONAL STRUCTS:
                                            * platform_dct_ota2_config_t
                                            *         New structure added as OPTIONAL
                                            *         (wrapped with WICED_DCT_INCLUDE_OTA2_CONFIG)
                                            *
                                            */
#define DCT_BOOTLOADER_SDK_3_6_0    0x0360 /* Changes from SDK-3.5.2 to SDK-3.6.0
                                            * OPTIONAL STRUCTS:
                                            * platform_dct_ota2_header_t
                                            *      changed padding[1] to force_factory_reset
                                            *      field size not changed, struct size not changed
                                            */
#define DCT_BOOTLOADER_SDK_3_6_1    0x0361 /* Changes from SDK-3.6.0 to SDK-3.6.1 NONE */
#define DCT_BOOTLOADER_SDK_3_6_2    0x0362 /* Changes from SDK-3.6.1 to SDK-3.6.2 NONE */
#define DCT_BOOTLOADER_SDK_3_6_3    0x0363 /* Changes from SDK-3.6.2 to SDK-3.6.3 NONE */
#define DCT_BOOTLOADER_SDK_3_7_0    0x0370 /* Changes from SDK-3.6.3 to SDK-3.7.0
                                            * platform_dct_header_t
                                            *    reverted to match SDK-3.3.0 (Baseline )
                                            *      moved magic_number, write_incomplete
                                            *      removed CRC, sequence number
                                            *      added is_current_dct
                                            * platform_dct_sdk_ver_t
                                            *      new structure
                                            *      CRC, seq. #
                                            *      dct_sdk_ver #
                                            */

#define DCT_BOOTLOADER_SDK_CURRENT  DCT_BOOTLOADER_SDK_3_7_0


typedef uint16_t wiced_dct_sdk_ver_t;

/* If the Bootloader SDK was defined, used that to support
 * upgrading from that SDK's DCT layout to the current DCT layout
 */

#if   (defined(BOOTLOADER_SDK_3_0_0) || defined(BOOTLOADER_SDK_3_0_1))

#define DCT_BOOTLOADER_SDK_VERSION              DCT_BOOTLOADER_SDK_3_0_0
#define BOOTLOADER_NO_ETHER_CONFIG
#define BOOTLOADER_NO_NETWORK_CONFIG
#define BOOTLOADER_NO_BT_CONFIG
#define BOOTLOADER_NO_P2P_CONFIG
#define BOOTLOADER_NO_OTA2_CONFIG
#define BOOTLOADER_NO_VERSION
#define BOOTLOADER_APP_LUT_NO_SECURE_FLAG
#define dct_header_to_use                       platform_dct_header_sdk_3_0_0_t

#elif defined(BOOTLOADER_SDK_3_1_0)

#define DCT_BOOTLOADER_SDK_VERSION              DCT_BOOTLOADER_SDK_3_1_0
#define BOOTLOADER_NO_ETHER_CONFIG
#define BOOTLOADER_NO_NETWORK_CONFIG
#define BOOTLOADER_NO_BT_CONFIG
#define BOOTLOADER_NO_P2P_CONFIG
#define BOOTLOADER_NO_OTA2_CONFIG
#define BOOTLOADER_NO_VERSION
#define BOOTLOADER_APP_LUT_NO_SECURE_FLAG
#define dct_header_to_use                       platform_dct_header_sdk_3_1_0_t

#elif defined(BOOTLOADER_SDK_3_1_1)

#define DCT_BOOTLOADER_SDK_VERSION              DCT_BOOTLOADER_SDK_3_1_1
#define BOOTLOADER_NO_ETHER_CONFIG
#define BOOTLOADER_NO_NETWORK_CONFIG
#define BOOTLOADER_NO_BT_CONFIG
#define BOOTLOADER_NO_P2P_CONFIG
#define BOOTLOADER_NO_OTA2_CONFIG
#define BOOTLOADER_NO_VERSION
#define BOOTLOADER_APP_LUT_NO_SECURE_FLAG
#define dct_header_to_use                       platform_dct_header_sdk_3_1_1_t

#elif defined(BOOTLOADER_SDK_3_1_2)

#define DCT_BOOTLOADER_SDK_VERSION              DCT_BOOTLOADER_SDK_3_1_2
#define BOOTLOADER_NO_ETHER_CONFIG
#define BOOTLOADER_NO_NETWORK_CONFIG
#ifndef ORIGINAL_APP_SDK_USED_BT_CONFIG
#define BOOTLOADER_NO_BT_CONFIG
#else
#define bootloader_dct_bt_config_to_use         platform_dct_bt_config_sdk_3_1_2_t
#endif
#define BOOTLOADER_NO_P2P_CONFIG
#define BOOTLOADER_NO_OTA2_CONFIG
#define BOOTLOADER_NO_VERSION
#define BOOTLOADER_APP_LUT_NO_SECURE_FLAG
#define dct_header_to_use                       platform_dct_header_sdk_3_1_2_t

#elif defined(BOOTLOADER_SDK_3_3_0)

#define DCT_BOOTLOADER_SDK_VERSION              DCT_BOOTLOADER_SDK_3_3_0
#ifndef ORIGINAL_APP_SDK_USED_BT_CONFIG
#define BOOTLOADER_NO_BT_CONFIG
#else
#define bootloader_dct_bt_config_to_use         platform_dct_bt_config_sdk_3_1_2_t
#endif
#define BOOTLOADER_NO_P2P_CONFIG
#define BOOTLOADER_NO_OTA2_CONFIG
#define BOOTLOADER_NO_VERSION
#define BOOTLOADER_APP_LUT_NO_SECURE_FLAG
#define dct_header_to_use                       platform_dct_header_sdk_3_1_2_t
#define bootloader_dct_network_config_to_use    platform_dct_network_config_sdk_3_3_0_t

#elif defined(BOOTLOADER_SDK_3_3_0_PARTICLE)

#define DCT_BOOTLOADER_SDK_VERSION              DCT_BOOTLOADER_SDK_3_3_0
#ifndef ORIGINAL_APP_SDK_USED_BT_CONFIG
#define BOOTLOADER_NO_BT_CONFIG
#else
#define bootloader_dct_bt_config_to_use         platform_dct_bt_config_sdk_3_1_2_t
#endif
#define BOOTLOADER_NO_P2P_CONFIG
#define BOOTLOADER_NO_OTA2_CONFIG
#define BOOTLOADER_NO_VERSION
#define BOOTLOADER_APP_LUT_NO_SECURE_FLAG
#define BOOTLOADER_NO_ETHER_CONFIG
#define BOOTLOADER_NO_NETWORK_CONFIG
#define dct_header_to_use                       platform_dct_header_sdk_3_1_2_t

#elif defined(BOOTLOADER_SDK_3_3_1)

#define DCT_BOOTLOADER_SDK_VERSION              DCT_BOOTLOADER_SDK_3_3_1
#ifndef ORIGINAL_APP_SDK_USED_BT_CONFIG
#define BOOTLOADER_NO_BT_CONFIG
#else
#define bootloader_dct_bt_config_to_use         platform_dct_bt_config_sdk_3_1_2_t
#endif
#define BOOTLOADER_NO_P2P_CONFIG
#define BOOTLOADER_NO_OTA2_CONFIG
#define BOOTLOADER_NO_VERSION
#define BOOTLOADER_APP_LUT_NO_SECURE_FLAG
#define dct_header_to_use                       platform_dct_header_sdk_3_1_2_t
#define bootloader_dct_network_config_to_use    platform_dct_network_config_sdk_3_3_1_t

#elif defined(BOOTLOADER_SDK_3_4_0)

#define DCT_BOOTLOADER_SDK_VERSION              DCT_BOOTLOADER_SDK_3_4_0
#ifndef ORIGINAL_APP_SDK_USED_BT_CONFIG
#define BOOTLOADER_NO_BT_CONFIG
#else
#define bootloader_dct_bt_config_to_use         platform_dct_bt_config_sdk_3_4_0_t
#endif
#define BOOTLOADER_NO_P2P_CONFIG
#define BOOTLOADER_NO_OTA2_CONFIG
#define BOOTLOADER_NO_VERSION
#define dct_header_to_use                       platform_dct_header_sdk_3_1_2_t

#elif defined(BOOTLOADER_SDK_3_5_1)

#define DCT_BOOTLOADER_SDK_VERSION              DCT_BOOTLOADER_SDK_3_5_1
#ifndef ORIGINAL_APP_SDK_USED_BT_CONFIG
#define BOOTLOADER_NO_BT_CONFIG
#else
#define bootloader_dct_bt_config_to_use         platform_dct_bt_config_sdk_3_4_0_t
#endif
#ifndef ORIGINAL_APP_SDK_USED_P2P_CONFIG
#define BOOTLOADER_NO_P2P_CONFIG
#endif
#define BOOTLOADER_NO_OTA2_CONFIG
#define BOOTLOADER_NO_VERSION
#define dct_header_to_use                       platform_dct_header_sdk_3_1_2_t

#elif defined(BOOTLOADER_SDK_3_5_2)

#define DCT_BOOTLOADER_SDK_VERSION              DCT_BOOTLOADER_SDK_3_5_2
#define DCT_BOOTLOADER_CRC_IS_IN_HEADER
#ifndef ORIGINAL_APP_SDK_USED_BT_CONFIG
#define BOOTLOADER_NO_BT_CONFIG
#else
#define bootloader_dct_bt_config_to_use         platform_dct_bt_config_sdk_3_4_0_t
#endif
#ifndef ORIGINAL_APP_SDK_USED_P2P_CONFIG
#define BOOTLOADER_NO_P2P_CONFIG
#endif
#ifndef ORIGINAL_APP_SDK_USED_OTA2_CONFIG
#define BOOTLOADER_NO_OTA2_CONFIG
#else
#define bootloader_dct_ota2_config_to_use       platform_dct_ota2_config_sdk_3_5_2_t
#endif
#define BOOTLOADER_NO_VERSION
#define dct_header_to_use                       platform_dct_header_sdk_3_5_2_t

#elif defined(BOOTLOADER_SDK_3_6_0)

#define DCT_BOOTLOADER_SDK_VERSION              DCT_BOOTLOADER_SDK_3_6_0
#define DCT_BOOTLOADER_CRC_IS_IN_HEADER
#ifndef ORIGINAL_APP_SDK_USED_BT_CONFIG
#define BOOTLOADER_NO_BT_CONFIG
#else
#define bootloader_dct_bt_config_to_use         platform_dct_bt_config_sdk_3_4_0_t
#endif
#ifndef ORIGINAL_APP_SDK_USED_P2P_CONFIG
#define BOOTLOADER_NO_P2P_CONFIG
#endif
#ifndef ORIGINAL_APP_SDK_USED_OTA2_CONFIG
#define BOOTLOADER_NO_OTA2_CONFIG
#else
#define bootloader_dct_ota2_config_to_use       platform_dct_ota2_config_sdk_3_6_0_t
#endif
#define BOOTLOADER_NO_VERSION
#define dct_header_to_use                       platform_dct_header_sdk_3_5_2_t

#elif defined(BOOTLOADER_SDK_3_6_1)

#define DCT_BOOTLOADER_SDK_VERSION              DCT_BOOTLOADER_SDK_3_6_1
#define DCT_BOOTLOADER_CRC_IS_IN_HEADER
#ifndef ORIGINAL_APP_SDK_USED_BT_CONFIG
#define BOOTLOADER_NO_BT_CONFIG
#else
#define bootloader_dct_bt_config_to_use         platform_dct_bt_config_sdk_3_4_0_t
#endif
#ifndef ORIGINAL_APP_SDK_USED_P2P_CONFIG
#define BOOTLOADER_NO_P2P_CONFIG
#endif
#ifndef ORIGINAL_APP_SDK_USED_OTA2_CONFIG
#define BOOTLOADER_NO_OTA2_CONFIG
#else
#define bootloader_dct_ota2_config_to_use       platform_dct_ota2_config_sdk_3_6_0_t
#endif
#define BOOTLOADER_NO_VERSION
#define dct_header_to_use                       platform_dct_header_sdk_3_5_2_t

#elif defined(BOOTLOADER_SDK_3_6_2)

#define DCT_BOOTLOADER_SDK_VERSION              DCT_BOOTLOADER_SDK_3_6_2
#define DCT_BOOTLOADER_CRC_IS_IN_HEADER
#ifndef ORIGINAL_APP_SDK_USED_BT_CONFIG
#define BOOTLOADER_NO_BT_CONFIG
#else
#define bootloader_dct_bt_config_to_use         platform_dct_bt_config_sdk_3_4_0_t
#endif
#ifndef ORIGINAL_APP_SDK_USED_P2P_CONFIG
#define BOOTLOADER_NO_P2P_CONFIG
#endif
#ifndef ORIGINAL_APP_SDK_USED_OTA2_CONFIG
#define BOOTLOADER_NO_OTA2_CONFIG
#else
#define bootloader_dct_ota2_config_to_use       platform_dct_ota2_config_sdk_3_6_0_t
#endif
#define BOOTLOADER_NO_VERSION
#define dct_header_to_use                       platform_dct_header_sdk_3_5_2_t

#elif defined(BOOTLOADER_SDK_3_6_3)

#define DCT_BOOTLOADER_SDK_VERSION              DCT_BOOTLOADER_SDK_3_6_3
#define DCT_BOOTLOADER_CRC_IS_IN_HEADER
#ifndef ORIGINAL_APP_SDK_USED_BT_CONFIG
#define BOOTLOADER_NO_BT_CONFIG
#else
#define bootloader_dct_bt_config_to_use         platform_dct_bt_config_sdk_3_4_0_t
#endif
#ifndef ORIGINAL_APP_SDK_USED_P2P_CONFIG
#define BOOTLOADER_NO_P2P_CONFIG
#endif
#ifndef ORIGINAL_APP_SDK_USED_OTA2_CONFIG
#define BOOTLOADER_NO_OTA2_CONFIG
#else
#define bootloader_dct_ota2_config_to_use       platform_dct_ota2_config_sdk_3_6_0_t
#endif
#define BOOTLOADER_NO_VERSION
#define dct_header_to_use                       platform_dct_header_sdk_3_5_2_t

#elif defined(BOOTLOADER_SDK_3_7_0)

#define DCT_BOOTLOADER_SDK_VERSION              DCT_BOOTLOADER_SDK_3_7_0
#define DCT_BOOTLOADER_CRC_IS_IN_VERSION

#else

/* if there is no BOOTLOADER SDK defined in the makefile, use the current version */
#define DCT_BOOTLOADER_SDK_VERSION              DCT_BOOTLOADER_SDK_CURRENT
#define DCT_BOOTLOADER_CRC_IS_IN_VERSION
#endif

#define DCT_VERSION_MAGIC_NUMBER     0xDC0200CD


/******************************************************
 *                      Macros
 ******************************************************/

/* TODO: find platform-specific file to allow other defines for these */
#ifndef CRC_INIT_VALUE
#include "../../utilities/crc/crc.h"
#define CRC_INIT_VALUE                              CRC32_INIT_VALUE
#define CRC_FUNCTION(address, size, previous_value) (uint32_t)crc32(address, size, previous_value)
typedef uint32_t    CRC_TYPE;
#endif

/* the CRC was added into platform_dct_header_t in SDK-5.1.2, moved to platform_dct_version_t in SDK-3.6.4 */
#define IS_DCT_CRC_IN_HEADER(sdk)   ( ((sdk == DCT_BOOTLOADER_SDK_3_5_2) || (sdk == DCT_BOOTLOADER_SDK_3_6_0) || \
                                       (sdk == DCT_BOOTLOADER_SDK_3_6_1) || (sdk == DCT_BOOTLOADER_SDK_3_6_2) || \
                                       (sdk == DCT_BOOTLOADER_SDK_3_6_3)) ? \
                                      WICED_TRUE : WICED_FALSE )

#define IS_DCT_CRC_IN_VERSION(sdk)  ( (sdk >= DCT_BOOTLOADER_SDK_3_7_0) ?  WICED_TRUE : WICED_FALSE )

/******************************************************
 *                    Constants
 ******************************************************/

#ifndef PRIVATE_KEY_SIZE
#define PRIVATE_KEY_SIZE        (2*1024)
#endif

#ifndef CERTIFICATE_SIZE
#define CERTIFICATE_SIZE        (4*1024)
#endif

#ifndef CONFIG_AP_LIST_SIZE
#define CONFIG_AP_LIST_SIZE     (5)
#endif

#ifndef COOEE_KEY_SIZE
#define COOEE_KEY_SIZE          (16)
#endif

#ifndef SECURITY_KEY_SIZE
#define SECURITY_KEY_SIZE       (64)
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

#if (DCT_BOOTLOADER_SDK_VERSION >= DCT_BOOTLOADER_SDK_3_1_1)
#define DCT_APP_LOCATION_OF(APP_INDEX) (uint32_t)(ptrdiff_t)((uint8_t *)&((platform_dct_header_t *)0)->apps_locations + sizeof(image_location_t) * ( APP_INDEX ))
#else
     /* this SDK does not have apps_locations in bootloader_dct_header_t (platform_dct_header_t for the SDK) */
#endif

/******************************************************
 *                   Enumerations
 ******************************************************/
/* these indicate which sections are in use, all sections are included in latest builds */
#define WICED_DCT_BT_CONFIG_USE_FLAG    (1 << 0)
#define WICED_DCT_P2P_CONFIG_USE_FLAG   (1 << 1)
#define WICED_DCT_OTA2_CONFIG_USE_FLAG  (1 << 2)

#define WICED_DCT_ALL_USE_FLAGS         (WICED_DCT_BT_CONFIG_USE_FLAG | WICED_DCT_P2P_CONFIG_USE_FLAG | WICED_DCT_OTA2_CONFIG_USE_FLAG)

typedef uint16_t wiced_dct_config_flag_t;


#ifdef WICED_DCT_INCLUDE_BT_CONFIG
#define WICED_DCT_FLAG_BT    WICED_DCT_BT_CONFIG_USE_FLAG
#else
#define WICED_DCT_FLAG_BT    0
#endif

#ifdef WICED_DCT_INCLUDE_P2P_CONFIG
#define WICED_DCT_FLAG_P2P   WICED_DCT_P2P_CONFIG_USE_FLAG
#else
#define WICED_DCT_FLAG_P2P   0
#endif

#ifdef WICED_DCT_INCLUDE_OTA2_CONFIG
#define WICED_DCT_FLAG_OTA2  WICED_DCT_OTA2_CONFIG_USE_FLAG
#else
#define WICED_DCT_FLAG_OTA2  0
#endif

/* This defines the configurations that are in use
 *    in the current DCT
 * default_dct_data.dct_sdk_ver.data_dct_usage_flags =
 *                                            WICED_DCT_CONFIG_FLAGS;
 */
#define WICED_DCT_CONFIG_FLAGS   (WICED_DCT_FLAG_BT | WICED_DCT_FLAG_P2P | WICED_DCT_FLAG_OTA2)

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
        char             filesystem_filename[32];
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
    char manufacturer[ 32 ];
    char product_name[ 32 ];
    char BOM_name[24];
    char BOM_rev[8];
    char serial_number[20];
    char manufacture_date_time[20];
    char manufacture_location[12];
    char bootloader_version[8];
} platform_dct_mfg_info_t;

typedef struct
{
    char    private_key[ PRIVATE_KEY_SIZE ];
    char    certificate[ CERTIFICATE_SIZE ];
    uint8_t cooee_key  [ COOEE_KEY_SIZE ];
} platform_dct_security_t;

typedef struct
{
    wiced_ap_info_t details;
    uint8_t         security_key_length;
    char            security_key[ SECURITY_KEY_SIZE ];
} wiced_config_ap_entry_t;

typedef struct
{
    wiced_ssid_t     SSID;
    wiced_security_t security;
    uint8_t          channel;
    uint8_t          security_key_length;
    char             security_key[ SECURITY_KEY_SIZE ];
    uint32_t         details_valid;
} wiced_config_soft_ap_t;

typedef struct
{
    wiced_bool_t              device_configured;
    wiced_config_ap_entry_t   stored_ap_list[CONFIG_AP_LIST_SIZE];
    wiced_config_soft_ap_t    soft_ap_settings;
    wiced_config_soft_ap_t    config_ap_settings;
    wiced_country_code_t      country_code;
    wiced_mac_t               mac_address;
    uint8_t                   padding[2];  /* ensure 32bit aligned size */
} platform_dct_wifi_config_t;

typedef struct
{
    wiced_mac_t               mac_address;
    uint8_t                   padding[2];  /* ensure 32bit aligned size */
} platform_dct_ethernet_config_t;

typedef struct
{
    wiced_interface_t         interface;
    wiced_hostname_t          hostname;
    uint8_t                   padding[2];  /* ensure 32bit aligned size */
} platform_dct_network_config_t;

/**
 * If you change anything in platform_dct_bt_config_t, go back to the other
 * platform_dct_old_sdk.h files and make sure the old bt structure
 * is defined so as to allow an update to the new layout!
 */
typedef struct
{
    uint8_t bluetooth_device_address[6];
    uint8_t bluetooth_device_name[249];     /* including null termination */
    uint8_t bluetooth_device_class[3];
    wiced_bool_t ssp_debug_mode;
    uint8_t padding[2];                     /* ensure 32-bit aligned size */
} platform_dct_bt_config_t;

/**
 * If you change anything in platform_dct_p2p_config_t, go back to the other
 * platform_dct_old_sdk.h files and make sure the old p2p structure
 * is defined so as to allow an update to the new layout!
 */
typedef struct
{
    wiced_config_soft_ap_t  p2p_group_owner_settings;
    uint8_t                 padding[2];     /* ensure 32-bit aligned size */
} platform_dct_p2p_config_t;

/**
 * If you change anything in platform_dct_ota2_config_t, go back to the other
 * platform_dct_old_sdk.h files and make sure the old ota2 structure
 * is defined so as to allow an update to the new layout!
 */
typedef struct
{
        uint16_t        update_count;           /* 0x00 when first programmed, incremented when updated -or- factory reset              */
        uint8_t         boot_type;              /* value = ota2_boot_type_t                                                             */
        uint8_t         force_factory_reset;    /* call wiced_ota2_force_factory_reset_on_reboot() - set to non-zero to force reboot )  */
} platform_dct_ota2_config_t;

/*
 * if valid, use sequence to determine current DCT
 */
typedef struct
{
    uint32_t                magic_number;           /* DCT_VERSION_MAGIC_NUMBER                                             */
    wiced_dct_config_flag_t data_dct_usage_flags;   /* which of the optional sub-structures are in use                      */
    wiced_dct_sdk_ver_t     version;                /* current DCT_BOOTLOADER_SDK_XXXX version                              */
    CRC_TYPE                crc32;                  /* crc for this DCT (0 for SDKs 3.5.2 thru 3.6.2, CRC is dct_header)    */
    char                    initial_write;          /* 1 = first time DCT is written at manufacture (crc will also be 0x00) */
    uint8_t                 sequence;               /* sequence number to know which DCT is the latest                      */
    uint8_t                 padding[2];             /* ensure 32-bit aligned size                                           */
} platform_dct_version_t;

/* Include the headers for previous version DCT headers
 * so that we can use the proper ones for
 *  platform_dct_header_t -- THIS STRUCTURE MUST ALWAYS MATCH THE BOOTLOADER IT IS BUILT FOR --
 *  bootloader_dct_data_t  & sub structures
 */

#if (DCT_BOOTLOADER_SDK_VERSION != DCT_BOOTLOADER_SDK_CURRENT)
#include "platform_dct_old_sdk.h"
#endif


struct platform_dct_header_current_s        /* SDK-3.6.4 */
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
};

typedef struct        /* SDK-3.6.4 */
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
#ifdef DCT_HEADER_ALIGN_SIZE
        uint8_t padding[DCT_HEADER_ALIGN_SIZE - sizeof(struct platform_dct_header_current_s)];
#endif
} platform_dct_header_current_t;

/* if the dct header is not defined, we are building a new application (not an upgrade).
 * use the current structure definition
 */
#ifndef dct_header_to_use
#define dct_header_to_use           platform_dct_header_current_t
#endif
typedef dct_header_to_use           platform_dct_header_t;

/* The structure for the complete system DCT layout.
 * The application DCT data follows this structure in the DCT section of FLASH.
 */
typedef struct
{
    platform_dct_header_t               dct_header;
    platform_dct_mfg_info_t             mfg_info;
    platform_dct_security_t             security_credentials;
    platform_dct_wifi_config_t          wifi_config;
#ifndef PARTICLE_DCT_COMPATIBILITY
    platform_dct_ethernet_config_t      ethernet_config;
    platform_dct_network_config_t       network_config;
    platform_dct_bt_config_t            bt_config;
    platform_dct_p2p_config_t           p2p_config;
    platform_dct_ota2_config_t          ota2_config;
    platform_dct_version_t              dct_version;
#endif
} platform_dct_data_t;

#ifdef PARTICLE_DCT_COMPATIBILITY
typedef struct
{
    platform_dct_network_config_t       network_config;
    platform_dct_version_t              dct_version;
} platform_dct_data2_t;
#endif

/***********************************************************************************
 *
 * BOOTLOADER DCT may be from an older SDK (for OTA or OTA2 upgrades)
 *
 * if we are building an upgrade, some of the sub structures might be different than
 * the current build - use this structure to access them
 *
 ***********************************************************************************/

/* struct added 3.3.0 field hostname changed string to struct in SDK 3.3.1  */
#ifndef bootloader_dct_network_config_to_use
#define bootloader_dct_network_config_to_use    platform_dct_network_config_t
#endif
typedef bootloader_dct_network_config_to_use    bootloader_dct_network_config_t;

/* struct added 3.1.2 added bluetooth_device_class (and padding) in SDK 3.4.0  */
#ifndef bootloader_dct_bt_config_to_use
#define bootloader_dct_bt_config_to_use         platform_dct_bt_config_t
#endif
typedef bootloader_dct_bt_config_to_use         bootloader_dct_bt_config_t;

/* struct added 3.5.2 changed padding into force_factory_reset for SDK 3.6.0 */
#ifndef bootloader_dct_ota2_config_to_use
#define bootloader_dct_ota2_config_to_use           platform_dct_ota2_config_t
#endif
typedef bootloader_dct_ota2_config_to_use           bootloader_dct_ota2_config_t;

typedef struct
{
    platform_dct_header_t               dct_header;             /* always the same as the bootloader dct header     Changed often       */
    platform_dct_mfg_info_t             mfg_info;               /* has not changed                                                      */
    platform_dct_security_t             security_credentials;   /* has not changed                                                      */
    platform_dct_wifi_config_t          wifi_config;            /* has not changed                                                      */

#ifndef BOOTLOADER_NO_ETHER_CONFIG
    platform_dct_ethernet_config_t      ethernet_config;        /* struct added 3.3.0 field sizes, struct size has not changed              */
#endif
#ifndef BOOTLOADER_NO_NETWORK_CONFIG
    bootloader_dct_network_config_t     network_config;         /* struct added 3.3.0 field hostname changed string to struct in SDK 3.3.1  */
#endif
#ifndef BOOTLOADER_NO_BT_CONFIG
    bootloader_dct_bt_config_t          bt_config;              /* struct added 3.1.2 (was optional) bluetooth_device_class added 3.4.0     */
#endif
#ifndef BOOTLOADER_NO_P2P_CONFIG
    platform_dct_p2p_config_t           p2p_config;             /* struct added 3.5.1 (was optional)                                        */
#endif
#ifndef BOOTLOADER_NO_OTA2_CONFIG
    bootloader_dct_ota2_config_t        ota2_config;            /* struct added 3.5.2 (was optional) padding, changed force_factory_reset 3.6.0 */
#endif
#ifndef BOOTLOADER_NO_VERSION
    platform_dct_version_t              dct_version;            /* struct added 3.6.4                                                       */
#endif
} bootloader_dct_data_t;



/* determine smallest and largest sizes of bootloader and application DCTs for buffer sizes for upgrades */

#define APPLICATION_DCT_DATA_SIZE   sizeof(platform_dct_data_t)
#define BOOTLOADER_DCT_DATA_SIZE    sizeof(bootloader_dct_data_t)
#define SMALLER_DCT_DATA_SIZE      ((APPLICATION_DCT_DATA_SIZE < BOOTLOADER_DCT_DATA_SIZE) ? APPLICATION_DCT_DATA_SIZE : BOOTLOADER_DCT_DATA_SIZE )
#define BIGGER_DCT_DATA_SIZE       ((APPLICATION_DCT_DATA_SIZE > BOOTLOADER_DCT_DATA_SIZE) ? APPLICATION_DCT_DATA_SIZE : BOOTLOADER_DCT_DATA_SIZE )

#if defined(_app_dct)
#define APPLICATION_DCT_WITH_APP_DCT_DATA_SIZE  (APPLICATION_DCT_DATA_SIZE + sizeof(_app_dct))
#define BOOTLOADER_DCT_WITH_APP_DCT_DATA_SIZE   (BOOTLOADER_DCT_DATA_SIZE + sizeof(_app_dct))
#else
#define APPLICATION_DCT_WITH_APP_DCT_DATA_SIZE  APPLICATION_DCT_DATA_SIZE
#define BOOTLOADER_DCT_WITH_APP_DCT_DATA_SIZE   BOOTLOADER_DCT_DATA_SIZE
#endif

/*
 * For the DCT update, we don't want to allocate 2 buffers sizeof(platform_dct_data_t) and sizeof(bootloader_dct_data_t)
 *     as this would be ~ 16K. THis is too much RAM to pull aside for Internal Flash systems (some have small RAM sizes).
 * Iinstead, we break the update down enough so that we are using the largest field size (CERTIFICATE_SIZE).
 *
 *     NOTE: We also use the resulting source and destination buffers to load the
 *           header & version structures (src & dst) into these same buffers,
 *           so they can also not be < sizeof(platform_dct_header_t) + sizeof(platform_dct_version_t)  (~800 bytes)
 *
 */
#define LARGEST_DCT_SUB_STRUCTURE_SIZE      CERTIFICATE_SIZE
/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif
