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
/**   Internet Control Message Protocol (ICMP)                            */ 
/**                                                                       */ 
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  COMPONENT DEFINITION                                   RELEASE        */ 
/*                                                                        */ 
/*    nx_icmpv6.h                                         PORTABLE C      */ 
/*                                                           5.7          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    Yuxin Zhou, Express Logic, Inc.                                     */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the NetX Internet Control Message Protocol (ICMP) */ 
/*    component, including all data types and external references.  It is */ 
/*    assumed that nx_api.h and nx_port.h have already been included.     */ 
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
/*  06-01-2010     Yuxin Zhou               Modified comments, added      */ 
/*                                            IPv6 PMTU support,          */
/*                                            modified struct field names */
/*                                            missing struct name prefix, */ 
/*                                            added router solicitation   */
/*                                            default settings,           */   
/*                                            resulting in version 5.5    */
/*  10-10-2011     Yuxin Zhou               Modified comment(s), added    */
/*                                            multihome support,          */
/*                                            resulting in version 5.6    */
/*  01-31-2013     Yuxin Zhou               Modified comment(s),          */
/*                                            resulting in version 5.7    */
/*                                                                        */ 
/**************************************************************************/ 

#ifndef NX_ICMPV6_H
#define NX_ICMPV6_H


#include "nx_nd_cache.h"
#ifdef FEATURE_NX_IPV6




/* Define ICMP types and codes.  */

#define NX_ICMPV4_ECHO_REQUEST_TYPE             8
#define NX_ICMPV4_ECHO_REPLY_TYPE               0


#define NX_ICMPV6_DEST_UNREACHABLE_TYPE         1
#define NX_ICMPV6_PACKET_TOO_BIG_TYPE           2
#define NX_ICMPV6_TIME_EXCEED_TYPE              3
#define NX_ICMPV6_PARAMETER_PROBLEM_TYPE        4
#define NX_ICMPV6_ECHO_REPLY_TYPE             129
#define NX_ICMPV6_ECHO_REQUEST_TYPE           128
#define NX_ICMPV6_ROUTER_SOLICITATION_TYPE    133
#define NX_ICMPV6_ROUTER_ADVERTISEMENT_TYPE   134
#define NX_ICMPV6_NEIGHBOR_SOLICITATION_TYPE  135
#define NX_ICMPV6_NEIGHBOR_ADVERTISEMENT_TYPE 136
#define NX_ICMPV6_REDIRECT_MESSAGE_TYPE       137
#define NX_ICMPV6_MLD_MULTICAST_LISTENER_QUERY  130
#define NX_ICMPV6_MLD_MULTICAST_LISTENER_REPORT 131
#define NX_ICMPV6_MLD_MULTICAST_LISTENER_DONE   132


#define ICMPV6_OPTION_TYPE_SRC_LINK_ADDR      1
#define ICMPV6_OPTION_TYPE_TRG_LINK_ADDR      2
#define ICMPV6_OPTION_TYPE_PREFIX_INFO        3
#define ICMPV6_OPTION_REDIRECTED_HEADER       4
#define ICMPV6_OPTION_TYPE_MTU                5

 /* Flag indicicatting that the option field should not contain
    source link layer address. */
#define NX_NO_SLLA                            1

 /* Define symbols for ICMPv6 error code. */
#define NX_ICMPV6_NO_ROUTE_TO_DESTINATION_CODE                   0
#define NX_ICMPV6_COMMUNICATION_WITH_DESTINATION_PROHIBITED_CODE 1
#define NX_ICMPV6_BEYOND_SCOPE_OF_SOURCE_ADDRESS_CODE            2
#define NX_ICMPV6_ADDRESS_UNREACHABLE_CODE                       3
#define NX_ICMPV6_DEST_UNREACHABLE_CODE                          4
#define NX_ICMPV6_SOURCE_ADDRESS_FAILED_I_E_POLICY_CODE          5
#define NX_ICMPV6_REJECT_ROUTE_TO_DESTINATION_CODE               6


/* Set host constants as per RFC 2461 Section 10. */

#ifndef NXDUO_DISABLE_ICMPV6_ROUTER_SOLICITATION

/* Define the max number of router solicitations a host sends until a router response
   is received.  If no response is received, the host concludes no router is present. */

#ifndef NXDUO_ICMPV6_MAX_RTR_SOLICITATIONS
#define NXDUO_ICMPV6_MAX_RTR_SOLICITATIONS      3
#endif

/* Define the interval between which the host sends router solicitations in seconds. */

#ifndef NXDUO_ICMPV6_RTR_SOLICITATION_INTERVAL
#define NXDUO_ICMPV6_RTR_SOLICITATION_INTERVAL  4
#endif

#endif


/* Define the minimum size path MTUs recommended by RFC 2460. */

#define NX_MINIMUM_IPV6_PATH_MTU                1280 
#define NX_MINIMUM_IPV4_PATH_MTU                576   
 
/* Define an infinate timeout for static destination table entries.  
   The default router entry should have an infinate timeout. */
#define NX_PATH_MTU_INFINITE_TIMEOUT            0xFFFFFFFF


/* Define basic ICMPv6 packet header data type. */

typedef  struct NX_ICMPV6_HEADER_STRUCT
{

    /*  Header field for ICMPv6 message type */
    UCHAR            nx_icmpv6_header_type;

    /*  Header field for ICMPv6 message code (type context specific) */
    UCHAR            nx_icmpv6_header_code;

    /*  Header field for ICMPv6 header checksum */
    USHORT           nx_icmpv6_header_checksum;
} NX_ICMPV6_HEADER;


/* Define ICMPv6 error message type. */

typedef struct NX_ICMPV6_ERROR_STRUCT
{
    /* General ICMPv6 header. */
    NX_ICMPV6_HEADER nx_icmpv6_error_header;

    /* Pointer to the original IPv6 packet where error is detected. */
    ULONG            nx_icmpv6_error_pointer;
} NX_ICMPV6_ERROR;


/* ICMPv6 echo request message type. */

typedef struct NX_ICMPV6_ECHO_STRUCT
{
    /* General ICMPv6 header. */
    NX_ICMPV6_HEADER nx_icmpv6_echo_header;

    /* Echo request message ID */
    USHORT           nx_icmpv6_echo_identifier;

    /* Echo request message sequence number */
    USHORT           nx_icmpv6_echo_sequence_num;

    /* Start of the ICMP echo request data. */
    UINT             nx_icmpv6_echo_data;
} NX_ICMPV6_ECHO;

/* Define the ICMPv6 Neighbor Discovery message type. */

typedef struct NX_ICMPV6_ND_STRUCT
{
    /* General ICMPv6 header. */
    NX_ICMPV6_HEADER nx_icmpv6_nd_header;

    /* Neighbor Discovery flag. */
    ULONG            nx_icmpv6_nd_flag;
   
    /* Neighbor Discovery taget address. */
    ULONG            nx_icmpv6_nd_targetAddress[4];
} NX_ICMPV6_ND;

/* Define the ICMPv6 additional option type. */

typedef struct NX_ICMPV6_OPTION_STRUCT
{
    /* Option type. */
    UCHAR            nx_icmpv6_option_type;

    /* Size of the option. */
    UCHAR            nx_icmpv6_option_length;

    /* Option data. */
    USHORT           nx_icmpv6_option_data;
} NX_ICMPV6_OPTION;


/* Define the Prefix option type. */

typedef struct NX_ICMPV6_OPTION_PREFIX_STRUCT
{

    /* prefix type. */
    UCHAR            nx_icmpv6_option_prefix_type;
    
    /* option lenght. */
    UCHAR            nx_icmpv6_option_prefix_optionlength;

    /* Prefix length. */
    UCHAR            nx_icmpv6_option_prefix_length;

    /* Flag. */
    UCHAR            nx_icmpv6_option_prefix_flag;
    
    /* Valid life time, in seconds. */
    ULONG            nx_icmpv6_option_prefix_valid_lifetime;

    /* Preferred life time, in seconds. */
    ULONG            nx_icmpv6_option_prefix_preferred_lifetime;

    /* Unused  */
    ULONG            nx_icmpv6_option_prefix_reserved;

    /* Prefix. */
    ULONG            nx_icmpv6_option_prefix[4];

} NX_ICMPV6_OPTION_PREFIX;

/* Define the MTU option type. */

typedef struct NX_ICMPV6_OPTION_MTU_STRUCT
{

    /* prefix type. */
    UCHAR            nx_icmpv6_option_mtu_type;
    
    /* option length. */
    UCHAR            nx_icmpv6_option_mtu_length;

    USHORT           nx_icmpv6_option_mtu_checksum;

    /* MTU length. */
    ULONG            nx_icmpv6_option_mtu_path_mtu;

    /* Pointer to the probe message (not the ICMPv6 header) 
       which for path MTU discovery. */
    UCHAR             *nx_icmpv6_option_mtu_message;

} NX_ICMPV6_OPTION_MTU;

/* Define the Router solicitation message type.  */

typedef struct NX_ICMPV6_RS_STRUCT
{
    /* General ICMPv6 header. */
    NX_ICMPV6_HEADER nx_icmpv6_rs_icmpv6_header;

    /* Unused; reserved for future use. */
    ULONG            nx_icmpv6_rs_reserved;
} NX_ICMPV6_RS;

/* Define the Router advertisement type. */

typedef struct NX_ICMPV6_RA_STRUCT
{
    /* General ICMPv6 header. */
    NX_ICMPV6_HEADER nx_icmpv6_ra_icmpv6_header;

    /* Hop limit. */
    UCHAR            nx_icmpv6_ra_hop_limit;

    /* Router advertisement flag. */
    UCHAR            nx_icmpv6_ra_flag;

    /* Router life time. */
    USHORT           nx_icmpv6_ra_router_lifetime;

    /* Router reachable time, in millisecond */
    ULONG            nx_icmpv6_ra_reachable_time;

    /* Local network retrans timer, in millisecond. */
    ULONG            nx_icmpv6_ra_retrans_time;
} NX_ICMPV6_RA;


/* Define the Redirect Message type. */

typedef struct NX_ICMPV6_REDIRECT_MESSAGE_STRUCT
{
    /* General ICMPv6 header. */
    NX_ICMPV6_HEADER nx_icmpv6_redirect_icmpv6_header;

    /* Unused field. */
    ULONG            nx_icmpv6_redirect_reserved;

    /* Next hop address. */
    ULONG            nx_icmpv6_redirect_target_address[4];

    /* Redirected host address. */
    ULONG            nx_icmpv6_redirect_destination_address[4];
} NX_ICMPV6_REDIRECT_MESSAGE;


/* Define the destination table entry type. */

typedef struct NX_IPV6_DESTINATION_ENTRY_STRUCT
{
    /* Flag indicates whether or not the entry is valid. */
    ULONG          nx_ipv6_destination_entry_valid;

    /* Destination IP address. */
    ULONG          nx_ipv6_destination_entry_destination_address[4];
    
    /* Next hop address.  Next hop could be the host, if it 
       is on the local network, or a router. */
    ULONG          nx_ipv6_destination_entry_next_hop[4];

    /* Cross link to the next hop entry in the ND cache. */
    ND_CACHE_ENTRY *nx_ipv6_destination_entry_nd_entry; 

#ifdef NX_ENABLE_IPV6_PATH_MTU_DISCOVERY

    /* Maximum transmission size for this destination. */
    ULONG  nx_ipv6_destination_entry_path_mtu;

    /* MTU Timeout value. */
    ULONG  nx_ipv6_destination_entry_MTU_timer_tick;                              
#endif

} NX_IPV6_DESTINATION_ENTRY;

/* Define the destination table. */
extern NX_IPV6_DESTINATION_ENTRY nx_destination_table[NXDUO_DESTINATION_TABLE_SIZE];

/* Define the destination table size. */
extern UINT  nx_ipv6_destination_table_size; 

/* Define the destination table mutex. */
extern TX_MUTEX nx_destination_table_protection;

#define NX_ICMPv6_HEADER_SIZE             sizeof(NX_ICMPv6_HEADER)

#ifndef NXDUO_DISABLE_ICMPV6_ERROR_MESSAGE
/* Define macros for sending out ICMPv6 error messages. */
#define NX_ICMPV6_SEND_DEST_UNREACHABLE(ip_ptr, packet, code)                \
    _nx_icmpv6_send_error_message((ip_ptr), (packet), ((NX_ICMPV6_DEST_UNREACHABLE_TYPE << 24) | ((code) << 16)), 0)

#define NX_ICMPV6_SEND_PACKET_TOO_BIG(ip_ptr, packet, code)                  \
    _nx_icmpv6_send_error_message((ip_ptr), (packet), ((NX_ICMPV6_PACKET_TOO_BIG_TYPE << 24) | ((code) << 16)), 0)

#define NX_ICMPV6_SEND_TIME_EXCEED(ip_ptr, packet, code)                     \
    _nx_icmpv6_send_error_message((ip_ptr), (packet), ((NX_ICMPV6_TIME_EXCEED_TYPE << 24) | ((code) << 16)), 0)

#define NX_ICMPV6_SEND_PARAMETER_PROBLEM(ip_ptr, packet, code, offset)       \
    _nx_icmpv6_send_error_message((ip_ptr), (packet), ((NX_ICMPV6_PARAMETER_PROBLEM_TYPE << 24) | ((code) << 16)), (offset))

#endif /* NXDUO_DISABLE_ICMPV6_ERROR_MESSAGE */

/* Define ICMPv6 API function prototypes. */

UINT  _nx_icmp_ping6(NX_IP *ip_ptr, NXD_ADDRESS *ip_address, 
                     CHAR *data_ptr, ULONG data_size,
                     NX_PACKET **response_ptr, ULONG wait_option);
UINT  _nx_icmp_interface_ping6(NX_IP *ip_ptr, NXD_ADDRESS *ip_address, 
                               CHAR *data_ptr, ULONG data_size, NXD_IPV6_ADDRESS *ipv6_address,
                               NX_PACKET **response_ptr, ULONG wait_option);
VOID  _nx_icmpv6_packet_process(NX_IP *ip_ptr, NX_PACKET *packet_ptr);


/* Define service for performing the Duplicate Address Detection protocol. */

#ifndef NXDUO_DISABLE_DAD
VOID  _nx_icmpv6_perform_DAD(NX_IP *, NXD_IPV6_ADDRESS *);
#endif /* NXDUO_DISABLE_DAD */


/* Define services for performing minimum Path MTU. */

#ifndef NX_DISABLE_IPV6_PATH_MTU_DISCOVERY
UINT _nx_icmpv6_perform_min_path_MTU_discovery(NX_IP *ip_ptr, NX_IPV6_DESTINATION_ENTRY *nd_cache_entry_ptr); 
UINT _nx_icmpv6_process_packet_too_big(NX_IP *ip_ptr, NX_PACKET *packet_ptr);
VOID _nx_icmpv6_destination_table_periodic_update(NX_IP *ip_ptr);
#endif

/* Define the destination search table function. */

#ifndef NXDUO_DISABLE_ICMPV6_ROUTER_ADVERTISEMENT_PROCESS
VOID _nx_icmpv6_process_ra(NX_IP *ip_ptr, NX_PACKET* packet_ptr);
UINT _nx_icmpv6_validate_ra(NX_IP *ip_ptr, NX_PACKET *packet_ptr);
#endif 

/* Define service for processing redirect packets. */

#ifndef NXDUO_DISABLE_ICMPV6_REDIRECT_PROCESS
VOID  _nx_icmpv6_process_redirect(NX_IP *ip_ptr, NX_PACKET *packet_ptr);
#endif /* NXDUO_DISABLE_ICMPV6_REDIRECT_PROCESS */


/* Define service for sending ICMPv6 error messages. */

#ifndef NXDUO_DISABLE_ICMPV6_ERROR_MESSAGE
VOID  _nx_icmpv6_send_error_message(NX_IP *ip_ptr, NX_PACKET *packet, ULONG word1, ULONG pointer);
#endif /* NXDUO_DISABLE_ICMPV6_ERROR_MESSAGE */

/* Define service for sending router solicitation requests. */

#ifndef NXDUO_DISABLE_ICMPV6_ROUTER_SOLICITATION
VOID  _nx_icmpv6_send_rs(NX_IP *ip_ptr, UINT if_index);
#endif 

/* Define internal ICMPv6 handling functions. */

VOID  _nx_icmpv6_send_queued_packets(NX_IP *ip_ptr, ND_CACHE_ENTRY *nd_entry);
UINT  _nx_icmpv6_validate_options(NX_ICMPV6_OPTION *option, int length, int additional_check);
UINT  _nxd_ipv6_destination_table_find_next_hop(ULONG *destination_ip, ND_CACHE_ENTRY **nd_entry, ULONG *next_hop);
UINT  _nx_icmpv6_dest_table_find(ULONG *destination_address, NX_IPV6_DESTINATION_ENTRY **dest_entry_ptr, ULONG *next_hop, ULONG path_mtu, ULONG mtu_timeout);
UINT  _nx_icmpv6_dest_table_add(NX_IP *ip_ptr, ULONG *destination_address, NX_IPV6_DESTINATION_ENTRY **dest_entry_ptr, ULONG *next_hop, ULONG path_mtu, ULONG mtu_timeout, NXD_IPV6_ADDRESS *ipv6_address);
VOID  _nx_icmpv6_process_echo_reply(NX_IP *ip_ptr, NX_PACKET *packet_ptr);
VOID  _nx_icmpv6_process_echo_request(NX_IP *ip_ptr, NX_PACKET *packet_ptr);
VOID  _nx_icmpv6_process_ns(NX_IP* ip_ptr, NX_PACKET *packet_ptr);
VOID  _nx_icmpv6_process_na(NX_IP *ip_ptr, NX_PACKET *packet_ptr);
void  nx_icmpv6_DAD_clear_NDCache_entry(NX_IP* ip_ptr, ULONG *ip_addr);
UINT  _nx_icmpv6_validate_neighbor_message(NX_IP *ip_ptr, NX_PACKET *packet_ptr);
VOID  _nx_icmpv6_DAD_failure(NX_IP *ip_ptr, NXD_IPV6_ADDRESS* iface);
VOID  _nx_icmpv6_send_ns (NX_IP *ip_ptr, ULONG *neighbor_IP_address, int send_slla, NXD_IPV6_ADDRESS *outgoing_address, int sendUnicast);

/* Define error checking shells for API services.  These are only referenced by the application.  */

UINT  _nxde_icmp_enable(NX_IP *ip_ptr);
UINT  _nxe_icmpv6_router_discovery_disable(NX_IP *ip_ptr);

#endif /* FEATURE_NX_IPV6 */ 
#endif
