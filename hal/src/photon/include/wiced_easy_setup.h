/*
 * Copyright (c) 2015 Broadcom
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * 3. Neither the name of Broadcom nor the names of other contributors to this
 * software may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * 4. This software may not be used as a standalone product, and may only be used as
 * incorporated in your product or device that incorporates Broadcom wireless connectivity
 * products and solely for the purpose of enabling the functionalities of such Broadcom products.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY WARRANTIES OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT, ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
