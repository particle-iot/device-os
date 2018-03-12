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
 *  Define cryptographic functions
 */

#pragma once

#include <stdint.h>
#include "wiced_result.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef uint32_t (*wiced_crypto_prng_get_random_t)( void );
typedef void     (*wiced_crypto_prng_add_entropy_t)( const void* buffer, uint16_t buffer_length );

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    wiced_crypto_prng_get_random_t  get_random;
    wiced_crypto_prng_add_entropy_t add_entropy;
} wiced_crypto_prng_t;

/******************************************************
 *               Function Declarations
 ******************************************************/

/**
 * Gets a 16 bit random numbers.
 *
 * Allows user applications to retrieve 16 bit random numbers.
 *
 * @param buffer : pointer to the buffer which will receive the
 *                 generated random data
 * @param buffer_length : size of the buffer
 *
 * @return WICED_SUCCESS or Error code
 */
extern wiced_result_t wiced_crypto_get_random( void* buffer, uint16_t buffer_length );

/**
 * Feed entropy into random number generator.
 *
 * @param buffer : pointer to the buffer which contains random data
 * @param buffer_length : size of the buffer
 *
 * @return WICED_SUCCESS or Error code
 */
extern wiced_result_t wiced_crypto_add_entropy( const void* buffer, uint16_t buffer_length );

/**
 * Set new PRNG implementation.
 *
 * @param prng : pointer to PRNG implementation, if NULL then default would be used
 *
 * @return WICED_SUCCESS or Error code
 */
extern wiced_result_t wiced_crypto_set_prng( wiced_crypto_prng_t* prng );

#ifdef __cplusplus
} /*extern "C" */
#endif
