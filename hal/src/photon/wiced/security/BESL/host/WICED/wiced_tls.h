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
#include "wiced_network.h"

#ifndef DISABLE_EAP_TLS
#include "wiced_supplicant.h"
#endif /* DISABLE_EAP_TLS */

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

wiced_result_t wiced_tls_encrypt_packet               ( wiced_tls_context_t* context, wiced_packet_t* packet );
wiced_result_t wiced_tls_decrypt_packet               ( wiced_tls_context_t* context, wiced_packet_t* packet );
wiced_result_t wiced_tls_receive_packet               ( wiced_tcp_socket_t* socket, wiced_packet_t** packet, uint32_t timeout );
wiced_bool_t   wiced_tls_is_encryption_enabled        ( wiced_tcp_socket_t* socket );
wiced_result_t wiced_tls_close_notify                 ( wiced_tcp_socket_t* socket );
wiced_result_t wiced_tls_calculate_overhead           ( wiced_tls_context_t* context, uint16_t content_length, uint16_t* header, uint16_t* footer );

#ifndef DISABLE_EAP_TLS
wiced_result_t wiced_supplicant_enable_tls            ( supplicant_workspace_t* supplicant, void* context );
wiced_result_t wiced_supplicant_start_tls             ( supplicant_workspace_t* supplicant, wiced_tls_endpoint_type_t type, wiced_tls_certificate_verification_t verification );
wiced_result_t wiced_supplicant_start_tls_with_ciphers( supplicant_workspace_t* supplicant, wiced_tls_endpoint_type_t type, wiced_tls_certificate_verification_t verification, const cipher_suite_t* cipher_list[] );
#endif /* DISABLE_EAP_TLS */

#ifdef __cplusplus
} /*extern "C" */
#endif
