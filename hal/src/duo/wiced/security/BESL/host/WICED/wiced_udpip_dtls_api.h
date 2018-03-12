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

wiced_result_t wiced_dtls_encrypt_packet( wiced_dtls_workspace_t* workspace, const wiced_ip_address_t* IP, uint16_t port, wiced_packet_t* packet );
wiced_result_t wiced_dtls_receive_packet ( wiced_udp_socket_t* socket, wiced_packet_t** packet, uint32_t timeout );
wiced_result_t wiced_dtls_close_notify    ( wiced_udp_socket_t* socket );
wiced_result_t wiced_dtls_calculate_overhead( wiced_dtls_workspace_t* context, uint16_t available_space, uint16_t* header, uint16_t* footer );

#ifdef __cplusplus
} /*extern "C" */
#endif
