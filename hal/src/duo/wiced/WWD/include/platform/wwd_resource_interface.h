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

#include "wwd_constants.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    WWD_RESOURCE_WLAN_FIRMWARE,
    WWD_RESOURCE_WLAN_NVRAM,
} wwd_resource_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

#if defined( WWD_DYNAMIC_NVRAM )
extern uint32_t dynamic_nvram_size;
extern void*    dynamic_nvram_image;
#endif

/******************************************************
 *               Function Declarations
 ******************************************************/

extern wwd_result_t host_platform_resource_size( wwd_resource_t resource, uint32_t* size_out );

/*
 * This function is used when WWD_DIRECT_RESOURCES is defined
 */
extern wwd_result_t host_platform_resource_read_direct( wwd_resource_t resource, const void** ptr_out );

/*
 * This function is used when WWD_DIRECT_RESOURCES is not defined
 */
extern wwd_result_t host_platform_resource_read_indirect( wwd_resource_t resource, uint32_t offset, void* buffer, uint32_t buffer_size, uint32_t* size_out );

#ifdef __cplusplus
} /* extern "C" */
#endif
