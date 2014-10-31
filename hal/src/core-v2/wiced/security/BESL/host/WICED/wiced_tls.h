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

#include "tls_types.h"
#include "wiced_network.h"

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
 *               Function Declarations
 ******************************************************/

wiced_result_t wiced_tls_encrypt_packet        ( wiced_tls_context_t* context, wiced_packet_t* packet );
wiced_result_t wiced_tls_decrypt_packet        ( wiced_tls_context_t* context, wiced_packet_t* packet );
wiced_result_t wiced_tls_receive_packet        ( wiced_tcp_socket_t* socket, wiced_packet_t** packet, uint32_t timeout );
wiced_bool_t   wiced_tls_is_encryption_enabled ( wiced_tcp_socket_t* socket );
wiced_result_t wiced_tls_close_notify          ( wiced_tcp_socket_t* socket );
wiced_result_t wiced_tls_calculate_overhead    ( wiced_tls_context_t* context, uint16_t content_length, uint16_t* header, uint16_t* footer );

#ifdef __cplusplus
} /*extern "C" */
#endif
