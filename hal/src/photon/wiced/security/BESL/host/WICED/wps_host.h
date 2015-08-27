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

#include "wwd_structures.h"
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
