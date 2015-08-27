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

#ifndef htobe16   /* This is defined in POSIX platforms */
static inline uint16_t htobe16(uint16_t v)
{
    return (uint16_t)(((v&0x00FF) << 8) | ((v&0xFF00)>>8));
}
#endif /* ifndef htobe16 */

#ifndef htobe32   /* This is defined in POSIX platforms */
static inline uint32_t htobe32(uint32_t v)
{
    return (uint32_t)(((v&0x000000FF) << 24) | ((v&0x0000FF00) << 8) | ((v&0x00FF0000) >> 8) | ((v&0xFF000000) >> 24));
}
#endif /* ifndef htobe32 */

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

/* Macros for comparing MAC addresses */
#define CMP_MAC( a, b )  (((((unsigned char*)a)[0])==(((unsigned char*)b)[0]))&& \
                          ((((unsigned char*)a)[1])==(((unsigned char*)b)[1]))&& \
                          ((((unsigned char*)a)[2])==(((unsigned char*)b)[2]))&& \
                          ((((unsigned char*)a)[3])==(((unsigned char*)b)[3]))&& \
                          ((((unsigned char*)a)[4])==(((unsigned char*)b)[4]))&& \
                          ((((unsigned char*)a)[5])==(((unsigned char*)b)[5])))

#define NULL_MAC( a )  (((((unsigned char*)a)[0])==0)&& \
                        ((((unsigned char*)a)[1])==0)&& \
                        ((((unsigned char*)a)[2])==0)&& \
                        ((((unsigned char*)a)[3])==0)&& \
                        ((((unsigned char*)a)[4])==0)&& \
                        ((((unsigned char*)a)[5])==0))


#define MEMORY_BARRIER_AGAINST_COMPILER_REORDERING()  __asm__ __volatile__ ("" : : : "memory") /* assume registers are Device memory, so have implicit CPU memory barriers */

#define REGISTER_WRITE_WITH_BARRIER( type, address, value ) do {*(volatile type *)(address) = (type)(value); MEMORY_BARRIER_AGAINST_COMPILER_REORDERING();} while (0)
#define REGISTER_READ( type, address )                      (*(volatile type *)(address))

#define wiced_jump_when_not_true( condition, label ) \
    do \
    { \
        if( ( condition ) == 0 ) \
        { \
            goto label; \
        } \
    } while(0)

#define wiced_action_jump_when_not_true( condition, jump_label, action ) \
    do \
    { \
        if( ( condition ) == 0 ) \
        { \
            { action; } \
            goto jump_label; \
        } \
    } while(0)


typedef enum
{
    LEAK_CHECK_THREAD,
    LEAK_CHECK_GLOBAL,
} leak_check_scope_t;

#ifdef WICED_ENABLE_MALLOC_DEBUG
#include "malloc_debug.h"
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

/* Define macros to assist operation on host MCUs that require aligned memory access */
#ifndef WICED_HOST_REQUIRES_ALIGNED_MEMORY_ACCESS

#define WICED_MEMCPY(destination, source, size)   memcpy(destination, source, size)

#define WICED_WRITE_16( pointer, value )      (*((uint16_t*)pointer) = value)
#define WICED_WRITE_32( pointer, value )      (*((uint32_t*)pointer) = value)
#define WICED_READ_16( pointer )              *((uint16_t*)pointer)
#define WICED_READ_32( pointer )              *((uint32_t*)pointer)

#else /* WICED_HOST_REQUIRES_ALIGNED_MEMORY_ACCESS */

#define WICED_MEMCPY( destination, source, size )   mem_byte_cpy( destination, source, size )

#define WICED_WRITE_16( pointer, value )      do { ((uint8_t*)pointer)[0] = (uint8_t)value; ((uint8_t*)pointer)[1]=(uint8_t)(value>>8); } while(0)
#define WICED_WRITE_32( pointer, value )      do { ((uint8_t*)pointer)[0] = (uint8_t)value; ((uint8_t*)pointer)[1]=(uint8_t)(value>>8); ((uint8_t*)pointer)[2]=(uint8_t)(value>>16); ((uint8_t*)pointer)[3]=(uint8_t)(value>>24); } while(0)
#define WICED_READ_16( pointer )              (((uint8_t*)pointer)[0] + (((uint8_t*)pointer)[1] << 8))
#define WICED_READ_32( pointer )              (((uint8_t*)pointer)[0] + ((((uint8_t*)pointer)[1] << 8)) + (((uint8_t*)pointer)[2] << 16) + (((uint8_t*)pointer)[3] << 24))

#endif /* WICED_HOST_REQUIRES_ALIGNED_MEMORY_ACCESS */

/******************************************************
 *                    Constants
 ******************************************************/

#define WICED_NEVER_TIMEOUT   (0xFFFFFFFF)
#define WICED_WAIT_FOREVER    (0xFFFFFFFF)
#define WICED_NO_WAIT         (0)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    WEP_KEY_ASCII_FORMAT,
    WEP_KEY_HEX_FORMAT,
} wep_key_format_t;

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

/*!
 ******************************************************************************
 * Convert a decimal or hexidecimal string to an integer.
 *
 * @param[in] str  The string containing the value.
 *
 * @return    The value represented by the string.
 */

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

/*!
 ******************************************************************************
 * Convert a decimal or hexidecimal string to an integer.
 *
 * @param[in] str  The string containing the value.
 *
 * @return    The value represented by the string.
 */

uint32_t generic_string_to_unsigned( const char* str );

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
 * Verifies the provided string is a collection of digits.
 *
 * @param str[in]        : The string to verify
 *
 * @return 1 if string is valid digits, 0 otherwise
 *
 */
int is_digit_str( const char* str );

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



void format_wep_keys( char* wep_key_ouput, const char* wep_key_data, uint8_t* wep_key_length, wep_key_format_t wep_key_format );

#ifdef __cplusplus
} /*extern "C" */
#endif
