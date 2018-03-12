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
