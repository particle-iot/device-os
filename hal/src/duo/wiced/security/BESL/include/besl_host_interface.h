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
#include "besl_structures.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Endian management functions */
uint32_t besl_host_hton32(uint32_t intlong);
uint16_t besl_host_hton16(uint16_t intshort);
uint32_t besl_host_hton32_ptr(uint8_t* in, uint8_t* out);
uint16_t besl_host_hton16_ptr(uint8_t* in, uint8_t* out);


extern besl_result_t besl_host_get_mac_address(besl_mac_t* address, uint32_t interface );
extern besl_result_t besl_host_set_mac_address(besl_mac_t* address, uint32_t interface );
extern void besl_host_random_bytes(uint8_t* buffer, uint16_t buffer_length);
extern void besl_host_get_time(besl_time_t* time);

/* Memory allocation functions */
extern void* besl_host_malloc( const char* name, uint32_t size );
extern void* besl_host_calloc( const char* name, uint32_t num, uint32_t size );
extern void  besl_host_free( void* p );

#ifdef __cplusplus
} /*extern "C" */
#endif
