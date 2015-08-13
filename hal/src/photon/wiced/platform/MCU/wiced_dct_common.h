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

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#ifndef OFFSETOF
#define OFFSETOF( type, member )  ( (uintptr_t)&((type *)0)->member )
#endif /* OFFSETOF */

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/* DCT section */
typedef enum
{
    DCT_APP_SECTION,
    DCT_SECURITY_SECTION,
    DCT_MFG_INFO_SECTION,
    DCT_WIFI_CONFIG_SECTION,
    DCT_INTERNAL_SECTION, /* Do not use in apps */
    DCT_ETHERNET_CONFIG_SECTION,
    DCT_NETWORK_CONFIG_SECTION,
#ifdef WICED_DCT_INCLUDE_BT_CONFIG
    DCT_BT_CONFIG_SECTION
#endif
} dct_section_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t wiced_dct_read_with_copy         ( void* info_ptr, dct_section_t section, uint32_t offset, uint32_t size );
wiced_result_t wiced_dct_update                 ( const void* info_ptr, dct_section_t section, uint32_t offset, uint32_t size );
wiced_result_t wiced_dct_get_app_header_location( uint8_t app_id, image_location_t* app_header_location );
wiced_result_t wiced_dct_set_app_header_location( uint8_t app_id, image_location_t* app_header_location );
wiced_result_t wiced_dct_restore_factory_reset  ( void );
void*          wiced_dct_get_current_address    ( dct_section_t section );

#ifdef __cplusplus
} /*extern "C" */
#endif
