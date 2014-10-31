/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#ifndef INCLUDED_TLV_H
#define INCLUDED_TLV_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#pragma pack(1)

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

extern tlv8_data_t*  tlv_find_tlv8    ( const uint8_t* message, uint16_t message_length, uint16_t type );
extern tlv16_data_t* tlv_find_tlv16   ( const uint8_t* message, uint16_t message_length, uint16_t type );
extern uint8_t*      tlv_write_header ( uint8_t* buffer, uint16_t type, uint16_t length );
extern tlv_result_t  tlv_read_value   ( uint16_t type, const uint8_t* message, uint16_t message_length, void* value, uint16_t value_size, tlv_data_type_t data_type );
extern uint8_t*      tlv_write_value  ( uint8_t* buffer, uint16_t type, uint16_t length, const void* data, tlv_data_type_t data_type );

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* INCLUDED_TLV_H */
