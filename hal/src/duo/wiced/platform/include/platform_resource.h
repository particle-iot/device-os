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
 * Defines globally accessible resource functions
 */
#pragma once
#include "platform_config.h"
#include "wiced_resource.h"

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

#ifdef USES_RESOURCE_FILESYSTEM
#include "wicedfs.h"

extern wicedfs_filesystem_t resource_fs_handle;
#endif /* ifdef USES_RESOURCE_FILESYSTEM */

/******************************************************
 *               Function Declarations
 ******************************************************/

/* Resource reading */
extern resource_result_t resource_get_readonly_buffer ( const resource_hnd_t* resource, uint32_t offset, uint32_t maxsize, uint32_t* size_out, const void** buffer );
extern resource_result_t resource_free_readonly_buffer( const resource_hnd_t* handle, const void* buffer );

extern resource_result_t platform_read_external_resource( const resource_hnd_t* resource, uint32_t offset, uint32_t maxsize, uint32_t* size, void* buffer );

#ifdef __cplusplus
} /*extern "C" */
#endif
