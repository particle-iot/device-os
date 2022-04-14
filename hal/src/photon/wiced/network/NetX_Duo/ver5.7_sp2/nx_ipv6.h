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
/**   Internet Protocol version 6 (IPv6)                                  */ 
/**                                                                       */ 
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  COMPONENT DEFINITION                                   RELEASE        */ 
/*                                                                        */ 
/*    nx_ipv6.h                                           PORTABLE C      */ 
/*                                                           5.7          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    Yuxin Zhou,  Express Logic, Inc.                                    */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the NetX Internet Protocol component,             */ 
/*    including all data types and external references.  It is assumed    */ 
/*    that nx_api.h and nx_port.h have already been included.             */ 
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
/*  11-23-2009     Yuxin Zhou               Fixed compiler warnings,      */
/*                                            resulting in version 5.4    */
/*  06-01-2010     Yuxin Zhou               Modified comment(s),          */
/*                                            changed address types to    */
/*                                            bit map instead of UINT,    */
/*                                            modified struct field names */
/*                                            missing struct name prefix, */   
/*                                            resulting in version 5.5    */ 
/*  10-10-2011     Yuxin Zhou               Modified comment(s), added    */
/*                                            and modified API prototypes,*/
/*                                            resulting in version 5.6    */
/*  01-31-2013     Yuxin Zhou               Modified comment(s), added    */
/*                                            the IPv6 disable service,   */
/*                                            ipv6 address change_notify  */
/*                                            service, added API to allow */
/*                                            IPv6 stateless address      */
/*                                            autoconfig to be disabled,  */
/*                                            added APIs to retrieve IPv6 */
/*                                            default router entries,     */
/*                                            resulting in version 5.7    */
/*                                                                        */ 
/**************************************************************************/ 

#include "tx_api.h"
#ifndef NX_IPV6_H
#define NX_IPV6_H

#ifdef FEATURE_NX_IPV6

/* Define basic IP Header constant.  */

/* Define Basic Internet packet header data type.  This will be used to
   build new IP packets and to examine incoming packets into NetX.  */

typedef  struct NX_IPV6_HEADER_STRUCT
{
    /* 
       Define the first 32-bit word of the IP header.  This word contains 
       the following information:  

            bits 31-28  IP Version = 0x6  (IP Version6)
            bits 27-20  Traffic Class
            bits 19-00  Flow Lable
     */
    ULONG       nx_ip_header_word_0;

    /* 
       Define the second word of the IP header.  This word contains
       the following information:

            bits 31-16  Payload Length
            bits 15-8   Next Header;
            bits  7-0   Hop limit
     */
    ULONG       nx_ip_header_word_1;

    /* Sender IP address. */
    ULONG       nx_ip_header_source_ip[4];

    /* Define the destination IP address.  */
    ULONG       nx_ip_header_destination_ip[4];

} NX_IPV6_HEADER;


/* Define the data structure of the IPv6 optional field used in
   hop-by-hop option and destination option headers. */
typedef struct NX_IPV6_HEADER_OPTION_STRUCT 
{

    /* A hint to the protocol that follows. */
    UCHAR nx_ipv6_header_option_next_header;

    /* Size of this option field.*/
    UCHAR nx_ipv6_header_option_ext_length;

    /* ICMPv6 Option header type. */
    UCHAR nx_ipv6_header_option_type;      

    /* ICMPv6 Option-specific area. */
    UCHAR nx_ipv6_header_option_data;      
} NX_IPV6_HEADER_OPTION;


/* Hop by hop header optoin. */
typedef struct NX_IPV6_HOP_BY_HOP_OPTION_STRUCT 
{
    /* Option type. */
    UCHAR nx_ipv6_hop_by_hop_option_type;    

    /* Size of this option field.*/
    UCHAR nx_ipv6_hop_by_hop_length;  

    /* Start point of the option data. */
    USHORT nx_ipv6_hop_by_hop_data;   
} NX_IPV6_HOP_BY_HOP_OPTION;


/* Routing header option. */
typedef struct NX_IPV6_HEADER_ROUTING_OPTION_STRUCT
{

    /* A hint to the protocol that follows. */
    UCHAR   nx_ipv6_header_routing_option_next_header;    

    /* Header length. */
    UCHAR   nx_ipv6_header_routing_option_hdr_ext_len;    

    /* Router type. */
    UCHAR   nx_ipv6_header_routing_option_routing_type;   

    /* Segments left. */
    UCHAR   nx_ipv6_header_routing_option_segments_left;  

    /* Data. */
    ULONG   *nx_ipv6_header_routing_option_data;          
} NX_IPV6_HEADER_ROUTING_OPTION;

/* Fragment header option. */
typedef struct NX_IPV6_HEADER_FRAGMENT_OPTION_STRUCT
{
    /* A hint to the protocol that follows. */
    UCHAR   nx_ipv6_header_fragment_option_next_header;

    /* Unused field. */
    UCHAR   nx_ipv6_header_fragment_option_reserved;

    /* Fragment offset. Last 3 bits are used as flags. */
    USHORT  nx_ipv6_header_fragment_option_offset_flag;

    /* ID field. */
    ULONG   nx_ipv6_header_fragment_option_packet_id;
} NX_IPV6_HEADER_FRAGMENT_OPTION;

#define IPV6_ADDRESS_INVALID      0

/* Unicast address type.  Note that site local address types have been 
   deprectated in RFC 4291 and are treated as global address types. */
#define IPV6_ADDRESS_LINKLOCAL    0x00000001
#define IPV6_ADDRESS_SITELOCAL    0x00000002
#define IPV6_ADDRESS_GLOBAL       0x00000004

/* Multicast address type */
#define IPV6_ALL_NODE_MCAST       0x00000010
#define IPV6_ALL_ROUTER_MCAST     0x00000020
#define IPV6_SOLICITED_NODE_MCAST 0x00000040

#define IPV6_ADDRESS_UNICAST      0x80000000
#define IPV6_ADDRESS_MULTICAST    0x40000000
#define IPV6_ADDRESS_UNSPECIFIED  0x20000000
#define IPV6_ADDRESS_LOOPBACK     0x10000000
    


/* 
    The following symbols define the order of the IPv6 optional 
    headers.  Each value represents a state during the parse of the
    optional headers.  An error occurs when the parser encounters
    an optional header when it is not expecting the header, i.e. 
    the header is out of order. .


    IPv6 defines the order of the optional headers: (RFC2460 section 4.1)
    * IPv6 Header
    * Hop-by-Hop Options header
    * Destination Options header
    * Routnig header
    * Fragment header
    * Authentication header
    * Encapsulating Security Payload header
    * Destination Options header
    * Upper-layer header
 */
typedef enum 
{
    IPV6_BASE_HEADER,               /* 0 */
    HOP_BY_HOP_HEADER,              /* 1 */ 
    DESTINATION_HEADER_1,           /* 2 */
    ROUTING_HEADER,                 /* 3 */
    FRAGMENT_HEADER,                /* 4 */
    AUTHENTICATION_HEADER,          /* 5 */
    ENCAP_SECURITY_HEADER,          /* 6 */
    DESTINATION_HEADER_2            /* 7 */
} NX_IPV6_OPTION_STATE;
        
/* Define IPv6 API function prototype. */
UINT  _nxd_ipv6_enable(NX_IP *ip_ptr);    
UINT  _nxd_ipv6_disable(NX_IP *ip_ptr);    
UINT  _nxd_ipv6_address_get(NX_IP *ip_ptr, UINT address_index, NXD_ADDRESS *ip_address, ULONG *prefix_length, UINT *interface_index);
UINT  _nxd_ipv6_address_set(NX_IP *ip_ptr, UINT interface_index, NXD_ADDRESS *ip_address, ULONG prefix_length, UINT *address_index);
UINT  _nxd_ipv6_address_delete(NX_IP *ip_ptr, UINT address_index);
/* Define error checking shells for API services.  These are only referenced by the application.  */
UINT  _nxde_ipv6_enable(NX_IP *ip_ptr);    
UINT  _nxde_ipv6_stateless_address_autoconfig_disable(NX_IP *ip_ptr, UINT interface_index);    
UINT  _nxde_ipv6_stateless_address_autoconfig_enable(NX_IP *ip_ptr, UINT interface_index);    



/* Define IPv6 internal functions. */
ULONG IPv6_Address_Type(ULONG *ip_address);

UINT  _nxd_ipv6_default_router_add_internal(NX_IP *ip_ptr, ULONG *router_addr, ULONG router_lifetime, NX_INTERFACE *if_ptr, int router_type, NX_IPV6_DEFAULT_ROUTER_ENTRY **_new_entry);
UINT  _nxd_ipv6_default_router_add(NX_IP *ip_ptr, NXD_ADDRESS *router_addr, ULONG router_lifetime, UINT interface_index);
UINT  _nxd_ipv6_default_router_delete(NX_IP *ip_ptr, NXD_ADDRESS *router_address);
UINT  _nxd_ipv6_default_router_get(NX_IP *ip_ptr, UINT interface_index, NXD_ADDRESS *router_addr, ULONG *router_lifetime, ULONG *prefix_length);
UINT  _nxd_ipv6_default_router_entry_get(NX_IP *ip_ptr, UINT interface_index, UINT entry_index, NXD_ADDRESS *router_addr, ULONG *router_lifetime, ULONG *prefix_length, ULONG *configuration_method);
UINT  _nxd_ipv6_default_router_number_of_entries_get(NX_IP *ip_ptr, UINT interface_index, UINT *num_entries);
VOID  _nxd_ipv6_default_router_table_init(NX_IP *ip_ptr);
UINT  _nxd_ipv6_linklocal_address_get(NX_IP *ip_ptr, NXD_ADDRESS *ip_address);
UINT  _nxd_ipv6_global_address_set(NX_IP *ip_ptr, NXD_ADDRESS *ip_address, ULONG prefix_length);
UINT  _nxd_ipv6_find_max_prefix_length(ULONG *addr1, ULONG *addr, UINT max_length);
UINT  _nxd_ipv6_stateless_address_autoconfig_disable(NX_IP *ip_ptr, UINT interface_index);
UINT  _nxd_ipv6_stateless_address_autoconfig_enable(NX_IP *ip_ptr, UINT interface_index);
VOID  _nx_ipv6_address_change_endian(ULONG *address);
VOID  _nx_ipv6_address_find_prefix(NXD_IPV6_ADDRESS* addr, ULONG *prefix);
VOID  _nx_ipv6_fragment_process(struct NX_IP_DRIVER_STRUCT *driver_req_ptr, UINT mtu);
UINT  _nx_ipv6_packet_copy(NX_PACKET *source_pkt_head, NX_PACKET *dest_pkt_head, UINT size);
UINT  _nx_ipv6_multicast_join(NX_IP *, ULONG*, NX_INTERFACE *);
UINT  _nx_ipv6_multicast_leave(NX_IP *, ULONG*, NX_INTERFACE *);
UINT  _nx_ipv6_option_error(NX_IP *ip_ptr, NX_PACKET* packet_ptr, UCHAR option_type, UINT offset);
VOID  _nx_ipv6_packet_receive(NX_IP *ip_ptr, NX_PACKET *packet_ptr);
VOID  _nx_ipv6_packet_send(NX_IP *ip_ptr, NX_PACKET *packet_ptr, ULONG protocol, ULONG payload_size, int hop_limit);
UINT  _nx_ipv6_prefix_list_add_entry(NX_IP *ip_ptr, ULONG *prefix, ULONG prefix_length, ULONG valid_lifetime, NX_IPV6_PREFIX_ENTRY **_new_entry);
VOID  _nx_ipv6_prefix_list_delete(NX_IP *ip_ptr, ULONG *prefix, int prefix_length);
VOID  _nx_ipv6_prefix_list_delete_entry(NX_IP *ip_ptr, NX_IPV6_PREFIX_ENTRY *entry);
UINT  _nx_ipv6_process_hop_by_hop_option(NX_IP *ip_ptr, NX_PACKET *packet_ptr);
UINT  _nx_ipv6_process_routing_option(NX_IP *ip_ptr, NX_PACKET *packet_ptr);
UINT  _nx_ipv6_process_fragment_option(NX_IP *ip_ptr, NX_PACKET *packet_ptr);

UINT  _nxd_ipv6_interface_find(NX_IP *ip_ptr, NXD_ADDRESS *dest_addr, NXD_IPV6_ADDRESS **ipv6_addr);
UINT  _nxd_ipv6_router_lookup(NX_IP *ip_ptr, ULONG *router_address, void **nd_cache_entry);
int   _nxd_ipv6_search_onlink(NX_IP *ip_ptr, ULONG *dest_addr);
VOID  _nxd_ipv6_router_solicitation_check(NX_IP *ip_ptr);
UINT  _nxd_ipv6_raw_packet_send_internal(NX_IP *ip_ptr, NX_PACKET *packet_ptr,  NXD_ADDRESS *destination_ip, ULONG protocol);
VOID  _nxd_ipv6_prefix_router_timer_tick(NX_IP *ip_ptr);
NX_IPV6_DEFAULT_ROUTER_ENTRY* _nxd_ipv6_find_default_router_from_address(NX_IP *ip_ptr, ULONG *ip_addr);
int   _nxd_ipv6_search_onlink(NX_IP *ip_ptr, ULONG *dest_addr);
UINT  _nxd_ipv6_address_change_notify(NX_IP* ip_ptr, VOID (*ipv6_address_change_notfiy)(NX_IP *, UINT, UINT, UINT, ULONG *));
int   CHECK_IP_ADDRESSES_BY_PREFIX(ULONG *ip_addr1, ULONG *ip_addr2, ULONG prefix_len);
int   CHECK_IPV6_SOLICITED_NODE_MCAST_ADDRESS(ULONG *dest_ip, ULONG* myip);



/* 
   If NX_IPV6_UTIL_INLINE is defined, the functions are compiled as inline functions.  
   Inline functions improve execution speed.  However it also increases code size.  
   Applications concerning more about code size should have the symbol undefeind 
   when building NetX Duo library and the application.
*/
#ifndef NX_IPV6_UTIL_INLINE

int   CHECK_IPV6_ADDRESS_RANGE(ULONG *ip_addr_start, ULONG *ip_addr_end,ULONG *ip_addr);
int   CHECK_IPV6_ADDRESSES_SAME(ULONG *ip_dest, ULONG *myip);
int   CHECK_UNSPECIFIED_ADDRESS(ULONG *ip_addr);
void  SET_UNSPECIFIED_ADDRESS(ULONG *ip_addr);
void  COPY_IPV6_ADDRESS(ULONG *copy_from, ULONG *copy_to);
void  COPY_NXD_ADDRESS(NXD_ADDRESS *copy_from, NXD_ADDRESS  *copy_to);
void  SET_SOLICITED_NODE_MULTICAST_ADDRESS(ULONG *address, ULONG *unicast_address);
int   CHECK_ALL_ROUTER_MCAST_ADDRESS(ULONG *address);

#endif /* NX_IPV6_UTIL_INLINE */


#endif /* FEATURE_NX_IPV6 */
#endif /* NX_IPV6_H */
