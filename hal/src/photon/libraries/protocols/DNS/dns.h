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

/**************************************************************************************************************
 * INCLUDES
 **************************************************************************************************************/

#include "wiced_tcpip.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************************
 * CONSTANTS
 **************************************************************************************************************/

#define DNS_MESSAGE_IS_A_RESPONSE           0x8000
#define DNS_MESSAGE_OPCODE                  0x7800
#define DNS_MESSAGE_AUTHORITATIVE           0x0400
#define DNS_MESSAGE_TRUNCATION              0x0200
#define DNS_MESSAGE_RECURSION_DESIRED       0x0100
#define DNS_MESSAGE_RECURSION_AVAILABLE     0x0080
#define DNS_MESSAGE_RESPONSE_CODE           0x000F

typedef enum
{
    DNS_NO_ERROR        = 0,
    DNS_FORMAT_ERROR    = 1,
    DNS_SERVER_FAILURE  = 2,
    DNS_NAME_ERROR      = 3,
    DNS_NOT_IMPLEMENTED = 4,
    DNS_REFUSED         = 5
} dns_message_response_code_t;

typedef enum
{
    RR_TYPE_A      = 1,
    RR_TYPE_NS     = 2,
    RR_TYPE_MD     = 3,
    RR_TYPE_MF     = 4,
    RR_TYPE_CNAME  = 5,
    RR_TYPE_SOA    = 6,
    RR_TYPE_MB     = 7,
    RR_TYPE_MG     = 8,
    RR_TYPE_MR     = 9,
    RR_TYPE_NULL   = 10,
    RR_TYPE_WKS    = 11,
    RR_TYPE_PTR    = 12,
    RR_TYPE_HINFO  = 13,
    RR_TYPE_MINFO  = 14,
    RR_TYPE_MX     = 15,
    RR_TYPE_TXT    = 16,
    RR_TYPE_AAAA   = 28,
    RR_TYPE_SRV    = 33,
    RR_TYPE_NSEC   = 47,
    RR_QTYPE_AXFR  = 252,
    RR_QTYPE_MAILB = 253,
    RR_QTYPE_AILA  = 254,
    RR_QTYPE_ANY   = 255
} dns_resource_record_type_t;

typedef enum
{
    RR_CLASS_IN  = 1,
    RR_CLASS_CS  = 2,
    RR_CLASS_CH  = 3,
    RR_CLASS_HS  = 4,
    RR_CLASS_ALL = 255
} dns_resource_record_class_t;

#define RR_CACHE_FLUSH   0x8000

/**************************************************************************************************************
 * STRUCTURES
 **************************************************************************************************************/

#pragma pack(1)
typedef struct
{
    uint8_t* start_of_name;
    uint8_t* start_of_packet; /* Used for compressed names; */
} dns_name_t;

typedef struct
{
    uint16_t id;
    uint16_t flags;
    uint16_t question_count;
    uint16_t answer_count;
    uint16_t authoritative_answer_count;
    uint16_t additional_record_count;
} dns_message_header_t;

typedef struct
{
    dns_message_header_t* header; /* Also used as start of packet for compressed names */
    uint8_t*              iter;
    uint16_t              max_size;
} dns_message_iterator_t;

typedef struct
{
    uint16_t type;
    uint16_t class;
} dns_question_t;

typedef struct
{
    uint16_t  type;
    uint16_t  class;
    uint32_t  ttl;
    uint16_t  rd_length;
    uint8_t*  rdata;
} dns_record_t;

typedef struct
{
    const char* CPU;
    const char* OS;
} dns_hinfo_t;

typedef struct
{
    uint16_t  priority;
    uint16_t  weight;
    uint16_t  port;
    char      target[1]; /* Target name which will be larger than 1 byte */
} dns_srv_data_t;

typedef struct
{
    uint8_t block_number;
    uint8_t bitmap_size;
    uint8_t bitmap[1];
} dns_nsec_data_t;

typedef struct
{
    uint8_t block_number;
    uint8_t bitmap_size;
    uint8_t bitmap[1];
} dns_nsec_ipv4_only_t;

typedef struct
{
    uint8_t block_number;
    uint8_t bitmap_size;
    uint8_t bitmap[4];
} dns_nsec_ipv4_ipv6_t;

typedef struct
{
    uint8_t block_number;
    uint8_t bitmap_size;
    uint8_t bitmap[5];
} dns_nsec_service_t;

#pragma pack()

/**************************************************************************************************************
 * VARIABLES
 **************************************************************************************************************/

/**************************************************************************************************************
 * FUNCTION DECLARATIONS
 **************************************************************************************************************/

/* DNS Client API */
wiced_result_t dns_client_add_server_address          ( wiced_ip_address_t address );
wiced_result_t dns_client_remove_all_server_addresses ( void );
wiced_result_t dns_client_hostname_lookup             ( const char* hostname, wiced_ip_address_t* address, uint32_t timeout_ms );

/* Functions to generate a custom DNS query */
void     dns_write_question( dns_message_iterator_t* iter, const char* target, uint16_t class, uint16_t type );
void     dns_write_record  ( dns_message_iterator_t* iter, const char* name,   uint16_t class, uint16_t type, uint32_t TTL, const void* rdata );
void     dns_write_header  ( dns_message_iterator_t* iter, uint16_t id, uint16_t flags, uint16_t question_count, uint16_t answer_count, uint16_t authoritative_count, uint16_t additional_count );
uint8_t* dns_write_name    ( uint8_t* dest, const char* src );
uint8_t* dns_write_string  ( uint8_t* dest, const char* src );
void     dns_write_uint16  ( dns_message_iterator_t* iter, uint16_t data );
void     dns_write_uint32  ( dns_message_iterator_t* iter, uint32_t data );
void     dns_write_bytes   ( dns_message_iterator_t* iter, const uint8_t* data, uint16_t length );

/* Functions to aid processing received queries */
uint16_t     dns_read_uint16           ( const uint8_t* data );
uint32_t     dns_read_uint32           ( const uint8_t* data );
char*        dns_read_name             ( const uint8_t* data, const dns_message_header_t* start_of_packet ); /* Note: This function returns a dynamically allocated string */
void         dns_get_next_question     ( dns_message_iterator_t* iter, dns_question_t* q, dns_name_t* name );
void         dns_get_next_record       ( dns_message_iterator_t* iter, dns_record_t* r,   dns_name_t* name );
wiced_bool_t dns_compare_name_to_string( const dns_name_t* name, const char* string );
void         dns_skip_name             ( dns_message_iterator_t* iter );

#ifdef __cplusplus
} /* extern "C" */
#endif
