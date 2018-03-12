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
#include "wiced_result.h"

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

typedef /*@abstract@*/ /*@immutable@*/ struct
{
    uint8_t*  buffer;
    uint32_t  size;
    volatile uint32_t  head; /* Read from */
    volatile uint32_t  tail; /* Write to */
} wiced_ring_buffer_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/* Ring Buffer API */
wiced_result_t ring_buffer_init       ( /*@out@*/ wiced_ring_buffer_t* ring_buffer, /*@keep@*/ uint8_t* buffer, uint32_t buffer_size );
wiced_result_t ring_buffer_deinit     ( wiced_ring_buffer_t* ring_buffer );
uint32_t       ring_buffer_write      ( wiced_ring_buffer_t* ring_buffer, const uint8_t* data, uint32_t data_length );
uint32_t       ring_buffer_used_space ( wiced_ring_buffer_t* ring_buffer );
uint32_t       ring_buffer_free_space ( wiced_ring_buffer_t* ring_buffer );
wiced_result_t ring_buffer_get_data   ( wiced_ring_buffer_t* ring_buffer, uint8_t** data, uint32_t* contiguous_bytes );
wiced_result_t ring_buffer_consume    ( wiced_ring_buffer_t* ring_buffer, uint32_t bytes_consumed );
wiced_result_t ring_buffer_read       ( wiced_ring_buffer_t* ring_buffer, uint8_t* data, uint32_t data_length, uint32_t* number_of_bytes_read );


#ifdef __cplusplus
} /* extern "C" */
#endif
