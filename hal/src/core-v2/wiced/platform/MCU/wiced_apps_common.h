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

typedef struct
{
        uint16_t        start;
        uint16_t        count;
}app_entry_t;

typedef struct
{
        uint8_t         count;          /* Number of entries */
        app_entry_t     sectors[8];     /* An app can have a maximum of 8 amendments (for simplicity) */
}app_header_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t wiced_apps_get_size( const image_location_t* app_header_location, uint32_t* size );
wiced_result_t wiced_apps_set_size( image_location_t* app_header_location, uint32_t size );
wiced_result_t wiced_apps_erase( const image_location_t* app_header_location );
wiced_result_t wiced_apps_write( const image_location_t* app_header_location, const uint8_t* data, uint32_t offset, uint32_t size, uint32_t* last_erased_sector );
wiced_result_t wiced_apps_read( const image_location_t* app_header_location, uint8_t* data, uint32_t offset, uint32_t size );

#ifdef __cplusplus
} /*extern "C" */
#endif
