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
 * Functions to read/write Type-Length-Value (TLV) data used in many common protocols.
 */

#ifndef INCLUDED_TLV_H
#define INCLUDED_TLV_H

#include <stdint.h>

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    TLV_UINT8     = 1,
    TLV_UINT16    = 2,
    TLV_UINT32    = 3,
    TLV_CHAR_PTR  = 4,
    TLV_UINT8_PTR = 5,
} tlv_data_type_t;

typedef enum
{
    TLV_SUCCESS,
    TLV_NOT_FOUND
} tlv_result_t;


/******************************************************
 *                 Type Definitions
 ******************************************************/

#pragma pack(1)

/* Structures for TLVs with 16-bit type and 16-bit length */
typedef struct
{
    uint16_t type;
    uint16_t length;
} tlv16_header_t;

typedef struct
{
    uint16_t type;
    uint16_t length;
    uint8_t data;
} tlv16_uint8_t;

typedef struct
{
    uint16_t type;
    uint16_t length;
    uint16_t data;
} tlv16_uint16_t;

typedef struct
{
    uint16_t type;
    uint16_t length;
    uint32_t data;
} tlv16_uint32_t;

typedef struct
{
    uint16_t type;
    uint16_t length;
    uint8_t data[1];
} tlv16_data_t;

/* Structures for TLVs with 8-bit type and 8-bit length */
typedef struct
{
    uint8_t type;
    uint8_t length;
} tlv8_header_t;

typedef struct
{
    uint8_t type;
    uint8_t length;
    uint8_t data;
} tlv8_uint8_t;

typedef struct
{
    uint8_t type;
    uint8_t length;
    uint16_t data;
} tlv8_uint16_t;

typedef struct
{
    uint8_t type;
    uint8_t length;
    uint32_t data;
} tlv8_uint32_t;

typedef struct
{
    uint8_t type;
    uint8_t length;
    uint8_t data[1];
} tlv8_data_t;

#pragma pack()

#ifdef __cplusplus
extern "C" {
#endif

/** Finds a particular type from a buffer of TLV data.
 *
 * Returns a pointer to the first TLV which has a type that matches the given type.
 * The pointer points to the 'type' field.
 * This is the 8-bit-Type & 8-bit-Lenght version
 *
 * @param[in] message        : A pointer to the source TLV data to be searched
 * @param[in] message_length : The length of the source TLV data to be searched
 * @param[in] type           : The TLV type to search for
 *
 * @return Pointer to start of first matching TLV, or NULL if not found
 */
tlv8_data_t*  tlv_find_tlv8    ( const uint8_t* message, uint32_t message_length, uint8_t type );

/** Finds a particular type from a buffer of TLV data.
 *
 * Returns a pointer to the first TLV which has a type that matches the given type.
 * The pointer points to the 'type' field.
 * This is the 16-bit-Type & 16-bit-Lenght version
 *
 * @param[in] message        : A pointer to the source TLV data to be searched
 * @param[in] message_length : The length of the source TLV data to be searched
 * @param[in] type           : The TLV type to search for
 *
 * @return Pointer to start of first matching TLV, or NULL if not found
 */
tlv16_data_t* tlv_find_tlv16   ( const uint8_t* message, uint32_t message_length, uint16_t type );
uint8_t*      tlv_write_header ( uint8_t* buffer, uint16_t type, uint16_t length );
tlv_result_t  tlv_read_value   ( uint16_t type, const uint8_t* message, uint16_t message_length, void* value, uint16_t value_size, tlv_data_type_t data_type );
uint8_t*      tlv_write_value  ( uint8_t* buffer, uint16_t type, uint16_t length, const void* data, tlv_data_type_t data_type );

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* INCLUDED_TLV_H */
