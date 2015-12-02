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

extern tls_result_t tls_host_create_buffer   ( wiced_tls_context_t* ssl, uint8_t** buffer, uint16_t buffer_size );
extern tls_result_t tls_host_free_packet     ( tls_packet_t* packet );
extern tls_result_t tls_host_send_tcp_packet ( void* context, tls_packet_t* packet );
extern tls_result_t tls_host_get_packet_data ( ssl_context* ssl, tls_packet_t* packet, uint32_t offset, uint8_t** data, uint16_t* data_length, uint16_t* available_data_length );
extern tls_result_t tls_host_set_packet_start( tls_packet_t* packet, uint8_t* start );

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
