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

#include "tls_types.h"

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

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *          TLS -> Host Function Declarations
 ******************************************************/

extern tls_result_t tls_host_create_buffer   ( wiced_tls_workspace_t* ssl, uint8_t** buffer, uint16_t buffer_size );
extern tls_result_t tls_host_free_packet     ( tls_packet_t* packet );
extern tls_result_t tls_host_send_tcp_packet ( void* context, tls_packet_t* packet );
extern tls_result_t tls_host_get_packet_data ( ssl_context* ssl, tls_packet_t* packet, uint32_t offset, uint8_t** data, uint16_t* data_length, uint16_t* available_data_length );
extern tls_result_t tls_host_set_packet_start( tls_packet_t* packet, uint8_t* start );
extern tls_result_t tls_calculate_encrypt_buffer_length( ssl_context* context, uint16_t* required_buff_size, uint16_t payload_size);

/*
 * This should wait for a specified amount of time to receive a packet.
 * If the SSL context already has a received packet stored, it should append it to the previous packet either contiguously or via a linked list.
 */
extern tls_result_t tls_host_receive_packet( ssl_context* ssl, tls_packet_t** packet, uint32_t timeout );

extern uint64_t tls_host_get_time_ms( void );

extern void* tls_host_malloc( const char* name, uint32_t size );
extern void  tls_host_free  ( void* p );

extern void* tls_host_get_defragmentation_buffer ( uint16_t size );
extern void  tls_host_free_defragmentation_buffer( void* buffer );
extern tls_result_t ssl_flush_output( ssl_context *ssl, uint8_t* buffer, uint32_t length );

/******************************************************
 *           Host -> TLS Function Declarations
 ******************************************************/

extern tls_result_t tls_get_next_record( ssl_context* ssl, tls_record_t** record, uint32_t timeout, tls_packet_receive_option_t packet_receive_option );
extern int32_t      ssl_cleanup_record(wiced_tls_context_t* ssl, tls_record_t* record);
extern void         tls_cleanup_current_record(ssl_context* ssl);
extern tls_result_t tls_skip_current_record( ssl_context* ssl );


int32_t tls1_prf( unsigned char *secret, int32_t slen, char *label, unsigned char *random, int32_t rlen, unsigned char *dstbuf, int32_t dlen );

#ifdef __cplusplus
} /*extern "C" */
#endif
