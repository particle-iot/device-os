/**************************************************************************/ 
/*                                                                        */ 
/*            Copyright (c) 1996-2013 by Express Logic Inc.               */ 
/*                                                                        */ 
/*  This software is copyrighted by and is the sole property of Express   */ 
/*  Logic, Inc.  All rights, title, ownership, or other interests         */ 
/*  in the software remain the property of Express Logic, Inc.  This      */ 
/*  software may only be used in accordance with the corresponding        */ 
/*  license agreement.  Any unauthorized use, duplication, transmission,  */ 
/*  distribution, or disclosure of this software is expressly forbidden.  */ 
/*                                                                        */
/*  This Copyright notice may not be removed or modified without prior    */ 
/*  written consent of Express Logic, Inc.                                */ 
/*                                                                        */ 
/*  Express Logic, Inc. reserves the right to modify this software        */ 
/*  without notice.                                                       */ 
/*                                                                        */ 
/*  Express Logic, Inc.                     info@expresslogic.com         */
/*  11423 West Bernardo Court               http://www.expresslogic.com   */
/*  San Diego, CA  92127                                                  */
/*                                                                        */
/**************************************************************************/


/**************************************************************************/
/**************************************************************************/
/**                                                                       */ 
/** NetX Component                                                        */
/**                                                                       */
/**   Neighbor Discovery Cache Management (ND CAHCE)                      */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  APPLICATION INTERFACE DEFINITION                       RELEASE        */ 
/*                                                                        */ 
/*    nx_nd_cache.h                                       PORTABLE C      */ 
/*                                                           5.7          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    Yuxin Zhou, Express Logic, Inc.                                     */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the basic Application Interface (API) to the      */ 
/*    high-performance NetX IPv6 Neighbor Discovery Cache implementation  */
/*    for the ThreadX real-time kernel.  All service prototypes and data  */
/*    structure definitions are defined in this file. Please note that    */
/*    basic data type definitions and other architecture-specific         */
/*    information is contained in the file nx_port.h.                     */ 
/*                                                                        */
/*    It is assumed that nx_api.h and nx_port.h have already been         */
/*    included.                                                           */ 
/*                                                                        */ 
/*    This header file is for NetX Duo internal use only.  Application    */ 
/*    shall not include this file directly.                               */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  12-30-2007     Yuxin Zhou               Initial Version 5.2           */ 
/*  08-03-2009     Yuxin Zhou               Modified comment(s),          */ 
/*                                            resulting in version 5.3    */ 
/*  11-23-2009     Yuxin Zhou               Modified comment(s),          */
/*                                            resulting in version 5.4    */
/*  06-01-2010     Yuxin Zhou               Modified comment(s),          */
/*                                            modified struct field names */
/*                                            missing struct name prefix, */   
/*                                            resulting in version 5.5    */ 
/*  10-10-2011     Yuxin Zhou               Modified comment(s), added    */
/*                                            new services, and changed   */
/*                                            function prototypes,        */ 
/*                                            resulting in version 5.6    */
/*  01-31-2013     Yuxin Zhou               Modified comment(s),          */
/*                                            resulting in version 5.7    */
/*                                                                        */ 
/**************************************************************************/ 

#ifndef NX_ND_CACHE_H
#define NX_ND_CACHE_H


#include "nx_ip.h"
#include "nx_ipv6.h"

#ifdef FEATURE_NX_IPV6


#ifndef NX_IPV6_NEIGHBOR_CACHE_SIZE
#define NX_IPV6_NEIGHBOR_CACHE_SIZE 16
#endif /* NX_IPV6_NEIGHBOR_CACHE_SIZE */


/* Symbols used the states of ND entries. */
#define ND_CACHE_STATE_INVALID    0
#define ND_CACHE_STATE_INCOMPLETE 1
#define ND_CACHE_STATE_REACHABLE  2
#define ND_CACHE_STATE_STALE      3
#define ND_CACHE_STATE_DELAY      4
#define ND_CACHE_STATE_PROBE      5
#define ND_CACHE_STATE_CREATED    6

/* 
 * Define values used for Neighbor Discovery protocol.
 * The default values are suggested by RFC2461, chapter 10. 
 */


#ifndef NX_MAX_MULTICAST_SOLICIT     
/* Max multicast NS messages */
#define NX_MAX_MULTICAST_SOLICIT       3
#endif

#ifndef NX_MAX_UNICAST_SOLICIT   
/* Max unicast NS messages. */
#define NX_MAX_UNICAST_SOLICIT         3
#endif

#ifndef NX_REACHABLE_TIME
/* Max reachable time, in seconds. */
#define NX_REACHABLE_TIME              4
#endif

#ifndef NX_RETRANS_TIMER
/* Max retrans timer, in milliseconds. */
#define NX_RETRANS_TIMER               1000
#endif 

#ifndef NX_DELAY_FIRST_PROBE_TIME
#define NX_DELAY_FIRST_PROBE_TIME      4
#endif


/* Define the ND cache entry structure. */
typedef struct ND_CACHE_ENTRY_STRUCT
{

    /* Neighbor IP address. */
    ULONG nx_nd_cache_dest_ip[4];                              

    /* Corresponding LLA.   */
    UCHAR nx_nd_cache_mac_addr[6];                             

    /* Number of Solicitation it needs to send before timing out. */
    UCHAR nx_nd_cache_num_solicit;                             

    /* Entry Status, such as INCOMPLETE, REACHABLE, and so on. */
    UCHAR nx_nd_cache_nd_status;                               

    /* Padding.  Reserved for future use. */
    UCHAR nx_nd_cache_reserved1;                               

    /* Padding.  Reserved for future use. */
    UCHAR nx_nd_cache_reserved2;                               

    /* Number of out going packets waiting for this entry to be resolved. */
    UCHAR nx_nd_cache_packet_waiting_queue_length;       

    /* Whether or not this entry is statically configured. */
    UCHAR nx_nd_cache_is_static;                                

    /* Timeout value. */
    ULONG nx_nd_cache_timer_tick;   

    /* Interface through which the destination can be reached. */
    NX_INTERFACE *nx_nd_cache_interface_ptr;

    /* Link to the default router table, if this neighbor is a router. */
    NX_IPV6_DEFAULT_ROUTER_ENTRY *nx_nd_cache_is_router;

    /* 
     * Queue head and queue tail of the out going packets. 
     * This queue is used for keeping outgoing packets while the stack is 
     * resolving the target link layer address. 
     */
    NX_PACKET *nx_nd_cache_packet_waiting_head;          
    NX_PACKET *nx_nd_cache_packet_waiting_tail;   

    /* 
     * Local interface associated with this neighbor entry. If outoing_address is known
     * outgoing packets shall be sent using this address. 
     */
    NXD_IPV6_ADDRESS *nx_nd_cache_outgoing_address;   
} ND_CACHE_ENTRY;

/* This is ND cache table mutex. */
extern TX_MUTEX nx_nd_cache_protection;  

/* This is the ND cache table.  */
extern ND_CACHE_ENTRY ND_CACHE[NX_IPV6_NEIGHBOR_CACHE_SIZE];

/* Declare internal functions */

/* Add en entry to the cache. */
UINT _nx_nd_cache_add(NX_IP* ip_ptr, ULONG *dest_ip, NX_INTERFACE *if_index, char*mac, int IsStatic, int status, NXD_IPV6_ADDRESS* nxd_interface, ND_CACHE_ENTRY **entry);

/* Clear the IsRouter field in an ND Entry. */
VOID _nx_nd_cache_clear_IsRouter_field(VOID* entry);

/* Create an IPv6-MAC mapping to the ND cache entry. */
UINT _nxd_nd_cache_entry_set(NX_IP *ip_ptr, ULONG *dest_ip, UINT if_index, char* mac);

/* Delete an entry based on IP address. */
UINT _nxd_nd_cache_entry_delete(NX_IP *ip_ptr, ULONG *dest_ip);

/* Delete an entry based on IP address. */
UINT _nx_nd_cache_delete_internal(NX_IP *ip_ptr, ND_CACHE_ENTRY *entry);

/* 100ms periodic update. */
VOID _nx_nd_cache_fast_periodic_update(NX_IP *ip_ptr);

/* Find cache entry based on neighbor IP address. */
UINT _nx_nd_cache_find_entry(NX_IP *ip_ptr, ULONG *dest_ip, ND_CACHE_ENTRY **entry);

/* Find cache entry based on neighbor IP address.  If not found, create new entry. */
UINT _nx_nd_cache_add_entry(NX_IP *ip_ptr, ULONG *dest_ip, NXD_IPV6_ADDRESS *nxd_address, ND_CACHE_ENTRY **entry);

/* Find cache entry based on LLA. */
UINT _nx_nd_cache_find_entry_by_mac_addr(NX_IP *ip_ptr, ULONG physical_msw, ULONG physical_lsw, ND_CACHE_ENTRY **entry);

/* Neighbor cache initialization. */
UINT _nx_nd_cache_init(VOID);

/* Invalidate the whole cache. */
UINT _nxd_nd_cache_invalidate(NX_IP *ip_ptr);

UINT  _nxd_nd_cache_ip_address_find(NX_IP *ip_ptr, NXD_ADDRESS *ip_address, ULONG physical_msw, ULONG physical_lsw, UINT *if_index);

/* The real function that invalidates the whole cache. */
VOID _nxd_nd_cache_invalidate_internal(NX_IP *ip_ptr);

UINT _nxd_nd_cache_hardware_address_find(NX_IP *ip_ptr, NXD_ADDRESS *ip_address, ULONG *physical_msw, ULONG *physical_lsw, UINT *if_index);

/* One second periodic update. */
VOID _nx_nd_cache_slow_periodic_update(NX_IP *ip_ptr);

/* Invalidate a given entry from the neighbor discovery table. */
VOID _nx_invalidate_destination_entry(ULONG *destination_ip);
#endif /* NX_ND_CACHE_H */



#endif /* FEATURE_NX_IPV6 */
