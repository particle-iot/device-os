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

#include <stdint.h>
#include "wiced_wifi.h"
#include "wiced_management.h"
#include "dns_redirect.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/**
 * WICED Cooee Element Type
 */
typedef enum
{
    WICED_COOEE_SSID       = 0,
    WICED_COOEE_WPA_KEY    = 1,
    WICED_COOEE_IP_ADDRESS = 2,
    WICED_COOEE_WEP_KEY    = 3,
} wiced_cooee_element_type_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef void (*wiced_easy_setup_cooee_callback_t)( uint8_t* cooee_data, uint16_t data_length );
typedef void (*wiced_easy_setup_wps_callback_t)( wiced_wps_credential_t* malloced_credentials, uint16_t credential_count );

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/** Start the Broadcom Easy Setup procedure utilizing Cooee
 *
 * @param[in] callback : Callback to process received Cooee data
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_easy_setup_start_cooee( wiced_easy_setup_cooee_callback_t callback );


typedef struct softap_setup {
    /**
     * Config entires - must end with a null entry {0,0,0,0}
     */
    const configuration_entry_t* config;

    /**
     * Whether to force setup even if already done.
     */
    wiced_bool_t force;

    dns_redirector_t dns_redirector;
    wiced_result_t result;

} softap_setup_t;

/** Start the Broadcom Easy Setup procedure utilizing Soft AP with web server
 *
 * @param[in] config  : an array of user configurable variables in configuration_entry_t format.
 *                      The array must be terminated with a "null" entry {0,0,0,0}
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_easy_setup_start_softap( softap_setup_t* config );

extern wiced_result_t wiced_easy_setup_stop_softap( softap_setup_t* config );


/** Start the Broadcom Easy Setup procedure utilizing iAP
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_easy_setup_start_iap( void );


/** Start the Broadcom Easy Setup procedure utilizing WPS
 *
 * @param[in] mode     : Indicates whether to use Push-Button (PBC) or PIN Number mode for WPS
 * @param[in] details  : Pointer to a structure containing manufacturing details
 *                       of this device
 * @param[in] password : Password for WPS PIN mode connections
 * @param[in] callback : Callback to process received credentials
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_easy_setup_start_wps( wiced_wps_mode_t mode, const wiced_wps_device_detail_t* details, char* password, wiced_easy_setup_wps_callback_t callback );


/** Stop the all Easy Setup procedures
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_easy_setup_stop( void );


#ifdef __cplusplus
} /* extern "C" */
#endif
