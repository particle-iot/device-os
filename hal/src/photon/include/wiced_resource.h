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
#ifndef INCLUDED_RESOURCE_H_
#define INCLUDED_RESOURCE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************
 *                      Macros
 ******************************************************/

#ifndef MIN
#define MIN( x, y ) ((x) < (y) ? (x) : (y))
#endif /* ifndef MIN */

/* Suppress unused parameter warning */
#ifndef UNUSED_PARAMETER
#define UNUSED_PARAMETER( x ) ( (void)(x) )
#endif

#ifndef RESULT_ENUM
#define RESULT_ENUM( prefix, name, value )  prefix ## name = (value)
#endif /* ifndef RESULT_ENUM */

/* These Enum result values are for Resource errors
 * Values: 4000 - 4999
 */
#define RESOURCE_RESULT_LIST( prefix )  \
    RESULT_ENUM(  prefix, SUCCESS,                         0 ),   /**< Success */                           \
    RESULT_ENUM(  prefix, UNSUPPORTED,                     7 ),   /**< Unsupported function */              \
    RESULT_ENUM(  prefix, OFFSET_TOO_BIG,               4001 ),   /**< Offset past end of resource */       \
    RESULT_ENUM(  prefix, FILE_OPEN_FAIL,               4002 ),   /**< Failed to open resource file */      \
    RESULT_ENUM(  prefix, FILE_SEEK_FAIL,               4003 ),   /**< Failed to seek to requested offset in resource file */ \
    RESULT_ENUM(  prefix, FILE_READ_FAIL,               4004 ),   /**< Failed to read resource file */

#define resource_get_size( resource ) ((resource)->size)

/******************************************************
 *                    Constants
 ******************************************************/

#define RESOURCE_ENUM_OFFSET  ( 1300 )

/******************************************************
 *                   Enumerations
 ******************************************************/

/**
 * Result type for WICED Resource function
 */
typedef enum
{
    RESOURCE_RESULT_LIST( RESOURCE_ )
} resource_result_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef const void*   resource_data_t;
typedef unsigned long resource_size_t;

/******************************************************
 *                    Structures
 ******************************************************/

/**
 * Memory handle
 */
typedef struct
{
    /*@shared@*/ const char* data;
} memory_resource_handle_t;

/**
 * Filesystem handle
 */
typedef struct
{
    unsigned long offset;
    /*@shared@*/ const char* filename;
} filesystem_resource_handle_t;


typedef enum
{
    RESOURCE_IN_MEMORY,
    RESOURCE_IN_FILESYSTEM,
} resource_location_t;

/**
 * Resource handle structure
 */
typedef struct
{
    resource_location_t location;

    unsigned long size;
    union
    {
        filesystem_resource_handle_t fs;
        memory_resource_handle_t     mem;
    } val;
} resource_hnd_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/** Read resource using the handle specified
 *
 * @param[in]  resource : handle of the resource to read
 * @param[in]  offset   : offset from the beginning of the resource block
 * @param[in]  maxsize  : size of the buffer
 * @param[out] size     : size of the data successfully read
 * @param[in]  buffer   : pointer to a buffer to contain the read data
 *
 * @return @ref resource_result_t
 */
extern resource_result_t resource_read( const resource_hnd_t* resource, uint32_t offset, uint32_t maxsize, uint32_t* size, void* buffer );

/** Retrieve a read only resource buffer using the handle specified
 *
 * @param[in]  resource : handle of the resource to read
 * @param[in]  offset   : offset from the beginning of the resource block
 * @param[in]  maxsize  : size of the buffer
 * @param[out] size     : size of the data successfully read
 * @param[out] buffer   : pointer to a buffer pointer to point to the resource data
 *
 * @return @ref resource_result_t
 */
extern resource_result_t resource_get_readonly_buffer ( const resource_hnd_t* resource, uint32_t offset, uint32_t maxsize, uint32_t* size_out, const void** buffer );

/** Free a read only resource buffer using the handle specified
 *
 * @param[in]  resource : handle of the resource to read
 * @param[in]  buffer   : pointer to a buffer set using resource_get_readonly_buffer
 *
 * @return @ref resource_result_t
 */
extern resource_result_t resource_free_readonly_buffer( const resource_hnd_t* handle, const void* buffer );

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* ifndef INCLUDED_RESOURCE_H_ */
