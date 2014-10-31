/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
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
wiced_result_t wiced_waf_app_erase          ( uint8_t app_id );
wiced_result_t wiced_waf_app_get_size       ( uint8_t app_id, uint32_t* size );
wiced_result_t wiced_waf_app_set_size       ( uint8_t app_id, uint32_t size );
wiced_result_t wiced_waf_app_write_chunk    ( uint8_t app_id, uint32_t offset, const uint8_t* data, uint32_t size, uint32_t* last_erased_sector );
wiced_result_t wiced_waf_app_read_chunk     ( uint8_t app_id, uint32_t offset, uint8_t* data, uint32_t size );
wiced_result_t wiced_waf_app_load           ( const image_location_t* app_header_location, uint32_t* destination );

#ifdef __cplusplus
} /*extern "C" */
#endif
