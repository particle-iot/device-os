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

#include "dtls_types.h"

#ifdef __cplusplus
extern "C"
{
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
 *          DTLS -> Host Function Declarations
 ******************************************************/

extern dtls_result_t dtls_host_create_buffer( dtls_context_t* dtls, dtls_peer_t* peer, uint8_t** buffer, uint16_t buffer_size );
extern dtls_result_t dtls_host_free_packet( uint32_t* packet );
extern dtls_result_t dtls_host_send_tcp_packet( void* context, uint32_t* packet );
extern dtls_result_t dtls_host_get_packet_data( dtls_context_t* dtls, uint32_t* packet, uint32_t offset, uint8_t** data, uint16_t* data_length, uint16_t* available_data_length );
extern dtls_result_t dtls_host_set_packet_start( uint32_t* packet, uint8_t* start );
extern dtls_result_t dtls_host_packet_get_info( uint32_t* packet, dtls_session_t* session );

/*
 * This should wait for a specified amount of time to receive a packet.
 * If the DTLS context already has a received packet stored, it should append it to the previous packet either contiguously or via a linked list.
 */
extern dtls_result_t dtls_host_receive_packet( dtls_context_t* ssl, uint32_t** packet, uint32_t timeout );
extern uint64_t      dtls_host_get_time_ms( void );

extern void* dtls_host_malloc( const char* name, uint32_t size );
extern void  dtls_host_free( void* p );

extern dtls_result_t dtls_flush_output( dtls_context_t* dtls, dtls_session_t* session, uint8_t* buffer, uint32_t length );

/******************************************************
 *           Host -> DTLS Function Declarations
 ******************************************************/

extern dtls_result_t dtls_get_next_record( dtls_context_t* context, dtls_session_t* session, dtls_record_t** record, uint32_t timeout, dtls_packet_receive_option_t packet_receive_option );
extern int32_t       dtls_cleanup_record( wiced_dtls_context_t* context, dtls_record_t* record );
extern void          dtls_cleanup_current_record( dtls_context_t* context );
extern dtls_result_t dtls_skip_current_record( dtls_context_t* context );

#ifdef __cplusplus
} /*extern "C" */
#endif
