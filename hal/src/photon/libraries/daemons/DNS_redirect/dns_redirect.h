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

#include "wiced.h"

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

typedef struct
{
    volatile char       dns_quit_flag;
    wiced_udp_socket_t  socket;
    wiced_thread_t      dns_thread;
    wiced_interface_t   interface;
} dns_redirector_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/**
 *  Start a daemon which redirects DNS queries
 *
 *  DNS queries from other devices will be replied with
 *  an answer that indicates this WICED device handles the requested domain.
 *
 *  The redirector can be set to reply to all DNS queries, or to a specific list of domains.
 *
 * @param[in] dns_server  Structure workspace that will be used for this DNS redirector instance - allocated by caller.
 * @param[in] interface   Which network interface the DNS redirector should listen on.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_dns_redirector_start( dns_redirector_t* dns_server, wiced_interface_t interface );


/**
 *  Stop a daemon which redirects DNS queries
 *
 * @param[in] dns_server  Structure workspace that was previously used with @ref wiced_dns_redirector_start
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_dns_redirector_stop ( dns_redirector_t* server );

/**
 * Return the length of the query resolved, or 0 if none matched.
 */
int dns_resolve_query(const char* query);


typedef struct
{
    const char* query;
    uint8_t     length;
} dns_query_table_entry_t;

int dns_resolve_query_default(const char* query);

int dns_resolve_query_table(const char* query, const dns_query_table_entry_t* table, unsigned table_size);

#ifdef __cplusplus
} /* extern "C" */
#endif
