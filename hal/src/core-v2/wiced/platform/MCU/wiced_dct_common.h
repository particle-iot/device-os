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
/**
 * DCT section
 */
typedef enum
{
    DCT_APP_SECTION,
    DCT_SECURITY_SECTION,
    DCT_MFG_INFO_SECTION,
    DCT_WIFI_CONFIG_SECTION,
    DCT_INTERNAL_SECTION, /* Do not use in apps */
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
void*          wiced_dct_get_current_address    ( dct_section_t section );

#ifdef __cplusplus
} /*extern "C" */
#endif
