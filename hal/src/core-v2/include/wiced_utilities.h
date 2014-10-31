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

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 * @cond                       Macros
 ******************************************************/

#ifdef LINT
/* Lint does not know about inline functions */
extern uint16_t htobe16(uint16_t v);
extern uint32_t htobe32(uint32_t v);

#else /* ifdef LINT */

static inline uint16_t htobe16(uint16_t v)
{
    return (uint16_t)(((v&0x00FF) << 8) | ((v&0xFF00)>>8));
}

static inline uint32_t htobe32(uint32_t v)
{
    return (uint32_t)(((v&0x000000FF) << 24) | ((v&0x0000FF00) << 8) | ((v&0x00FF0000) >> 8) | ((v&0xFF000000) >> 24));
}

#endif /* ifdef LINT */

#ifndef MIN
#define MIN(x,y)  ((x) < (y) ? (x) : (y))
#endif /* ifndef MIN */

#ifndef MAX
#define MAX(x,y)  ((x) > (y) ? (x) : (y))
#endif /* ifndef MAX */

#ifndef ROUND_UP
#define ROUND_UP(x,y)    ((x) % (y) ? (x) + (y)-((x)%(y)) : (x))
#endif /* ifndef ROUND_UP */

#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(m, n)    (((m) + (n) - 1) / (n))
#endif /* ifndef DIV_ROUND_UP */

#define WICED_VERIFY(x)                               {wiced_result_t res = (x); if (res != WICED_SUCCESS){return res;}}

#define MEMCAT(destination, source, source_length)    (void*)((uint8_t*)memcpy((destination),(source),(source_length)) + (source_length))

#define MALLOC_OBJECT(name,object_type)               ((object_type*)malloc_named(name,sizeof(object_type)))

#define OFFSET(type, member)                          ((uint32_t)&((type *)0)->member)

typedef enum
{
    LEAK_CHECK_THREAD,
    LEAK_CHECK_GLOBAL,
} leak_check_scope_t;

#ifdef WICED_ENABLE_MALLOC_DEBUG
#include <stddef.h>
#include "wwd_rtos.h"
extern void* calloc_named                  ( const char* name, size_t nelems, size_t elemsize );
extern void * calloc_named_hideleak        ( const char* name, size_t nelem, size_t elsize );
extern void* malloc_named                  ( const char* name, size_t size );
extern void* malloc_named_hideleak         ( const char* name, size_t size );
extern void  malloc_set_name               ( const char* name );
extern void  malloc_leak_set_ignored       ( leak_check_scope_t global_flag );
extern void  malloc_leak_set_base          ( leak_check_scope_t global_flag );
extern void  malloc_leak_check             ( malloc_thread_handle thread, leak_check_scope_t global_flag );
extern void  malloc_transfer_to_curr_thread( void* block );
extern void  malloc_transfer_to_thread     ( void* block, malloc_thread_handle thread );
#else
#define calloc_named( name, nelems, elemsize) calloc ( nelems, elemsize )
#define calloc_named_hideleak( name, nelems, elemsize )  calloc ( nelems, elemsize )
#define realloc_named( name, ptr, size )      realloc( ptr, size )
#define malloc_named( name, size )            malloc ( size )
#define malloc_named_hideleak( name, size )   malloc ( size )
#define malloc_set_name( name )
#define malloc_leak_set_ignored( global_flag )
#define malloc_leak_set_base( global_flag )
#define malloc_leak_check( thread, global_flag )
#define malloc_transfer_to_curr_thread( block )
#define malloc_transfer_to_thread( block, thread )
#endif /* ifdef WICED_ENABLE_MALLOC_DEBUG */

/******************************************************
 *                    Constants
 ******************************************************/

#define WICED_NEVER_TIMEOUT   (0xFFFFFFFF)
#define WICED_WAIT_FOREVER    (0xFFFFFFFF)
#define WICED_NO_WAIT         (0)

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
 *                 Function Declarations
 * @endcond
 ******************************************************/

/**
 * Converts a decimal/hexidecimal string to an unsigned long int
 * Better than strtol or atol or atoi because the return value indicates if an error occurred
 *
 * @param string[in]     : The string buffer to be converted
 * @param str_length[in] : The maximum number of characters to process in the string buffer
 * @param value_out[out] : The unsigned in that will receive value of the the decimal string
 * @param is_hex[in]     : 0 = Decimal string, 1 = Hexidecimal string
 *
 * @return the number of characters successfully converted.  i.e. 0 = error
 *
 */
uint8_t string_to_unsigned( const char* string, uint8_t str_length, uint32_t* value_out, uint8_t is_hex );

/**
 * Converts a unsigned long int to a decimal string
 *
 * @param value[in]      : The unsigned long to be converted
 * @param output[out]    : The buffer which will receive the decimal string
 * @param min_length[in] : the minimum number of characters to output (zero padding will apply if required)
 * @param max_length[in] : the maximum number of characters to output (up to 10 )
 *
 * @note: No trailing null is added.
 *
 * @return the number of characters returned.
 *
 */
uint8_t unsigned_to_decimal_string( uint32_t value, char* output, uint8_t min_length, uint8_t max_length );


/**
 * Converts a decimal/hexidecimal string (with optional sign) to a signed long int
 * Better than strtol or atol or atoi because the return value indicates if an error occurred
 *
 * @param string[in]     : The string buffer to be converted
 * @param str_length[in] : The maximum number of characters to process in the string buffer
 * @param value_out[out] : The unsigned in that will receive value of the the decimal string
 * @param is_hex[in]     : 0 = Decimal string, 1 = Hexidecimal string
 *
 * @return the number of characters successfully converted (including sign).  i.e. 0 = error
 *
 */
uint8_t string_to_signed( const char* string, uint8_t str_length, int32_t* value_out, uint8_t is_hex );


/**
 * Converts a signed long int to a decimal string
 *
 * @param value[in]      : The signed long to be converted
 * @param output[out]    : The buffer which will receive the decimal string
 * @param min_length[in] : the minimum number of characters to output (zero padding will apply if required)
 * @param max_length[in] : the maximum number of characters to output (up to 10 )
 *
 * @note: No trailing null is added.
 *
 * @return the number of characters returned.
 *
 */
uint8_t signed_to_decimal_string( int32_t value, char* output, uint8_t min_length, uint8_t max_length );

/**
 * Converts a unsigned long int to a hexidecimal string
 *
 * @param value[in]      : The unsigned long to be converted
 * @param output[out]    : The buffer which will receive the hexidecimal string
 * @param min_length[in] : the minimum number of characters to output (zero padding will apply if required)
 * @param max_length[in] : the maximum number of characters to output (up to 8 )
 *
 * @note: No leading '0x' or trailing null is added.
 *
 * @return the number of characters returned.
 *
 */
uint8_t unsigned_to_hex_string( uint32_t value, char* output, uint8_t min_length, uint8_t max_length );


/**
 ******************************************************************************
 * Convert a nibble into a hex character
 *
 * @param[in] nibble  The value of the nibble in the lower 4 bits
 *
 * @return    The hex character corresponding to the nibble
 */
static inline char nibble_to_hexchar( uint8_t nibble )
{
    if (nibble > 9)
    {
        return (char)('A' + (nibble - 10));
    }
    else
    {
        return (char) ('0' + nibble);
    }
}


/**
 ******************************************************************************
 * Convert a nibble into a hex character
 *
 * @param[in] nibble  The value of the nibble in the lower 4 bits
 *
 * @return    The hex character corresponding to the nibble
 */
static inline char hexchar_to_nibble( char hexchar, uint8_t* nibble )
{
    if ( ( hexchar >= '0' ) && ( hexchar <= '9' ) )
    {
        *nibble = (uint8_t)( hexchar - '0' );
        return 0;
    }
    else if ( ( hexchar >= 'A' ) && ( hexchar <= 'F' ) )
    {
        *nibble = (uint8_t) ( hexchar - 'A' + 10 );
        return 0;
    }
    else if ( ( hexchar >= 'a' ) && ( hexchar <= 'f' ) )
    {
        *nibble = (uint8_t) ( hexchar - 'a' + 10 );
        return 0;
    }
    return -1;
}

/**
 ******************************************************************************
 * Append the two character hex value of a byte to a string
 *
 * @note: no terminating null is added
 *
 * @param[out] string  The buffer which will receive the two bytes
 * @param[in]  byte    The byte which will be converted to hex
 *
 * @return    A pointer to the character after the two hex characters added
 */
static inline char* string_append_two_digit_hex_byte( char* string, uint8_t byte )
{
    *string = nibble_to_hexchar( ( byte & 0xf0 ) >> 4 );
    string++;
    *string = nibble_to_hexchar( ( byte & 0x0f ) >> 0 );
    string++;
    return string;
}

#ifdef __cplusplus
} /*extern "C" */
#endif
