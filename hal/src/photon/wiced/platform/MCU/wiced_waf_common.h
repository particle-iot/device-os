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

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/
typedef struct wiced_app_s
{
        uint8_t         app_id;
        uint32_t        offset;
        uint32_t        last_erased_sector;
        image_location_t  app_header_location;
}wiced_app_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
void           wiced_waf_start_app          ( uint32_t entry_point );
wiced_result_t wiced_waf_check_factory_reset( void );
wiced_result_t wiced_waf_reboot             ( void );
wiced_result_t wiced_waf_app_set_boot       ( uint8_t app_id, char load_once );
wiced_result_t wiced_waf_app_open           ( uint8_t app_id, wiced_app_t* app );
wiced_result_t wiced_waf_app_close          ( wiced_app_t* app );
wiced_result_t wiced_waf_app_erase          ( wiced_app_t* app );
wiced_result_t wiced_waf_app_get_size       ( wiced_app_t* app, uint32_t* size );
wiced_result_t wiced_waf_app_set_size       ( wiced_app_t* app, uint32_t size );
wiced_result_t wiced_waf_app_write_chunk    ( wiced_app_t* app, const uint8_t* data, uint32_t size );
wiced_result_t wiced_waf_app_read_chunk     ( wiced_app_t* app, uint32_t offset, uint8_t* data, uint32_t size );
wiced_result_t wiced_waf_app_load           ( const image_location_t* app_header_location, uint32_t* destination );

#ifdef __cplusplus
} /*extern "C" */
#endif
