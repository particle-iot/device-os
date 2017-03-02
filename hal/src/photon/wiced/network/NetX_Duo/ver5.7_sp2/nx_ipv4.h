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
/**   Internet Protocol (IP)                                              */ 
/**                                                                       */ 
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  COMPONENT DEFINITION                                   RELEASE        */ 
/*                                                                        */ 
/*    nx_ipv4.h                                           PORTABLE C      */ 
/*                                                           5.7 (sp1)    */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    William E. Lamie, Express Logic, Inc.                               */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the NetX Internet Protocol component,             */ 
/*    including all data types and external references.  It is assumed    */ 
/*    that nx_api.h and nx_port.h have already been included.             */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  12-12-2005     William E. Lamie         Initial Version 5.0           */ 
/*  08-09-2007     William E. Lamie         Modified comment(s), and      */ 
/*                                            changed UL to ULONG cast,   */ 
/*                                            resulting in version 5.1    */ 
/*  12-30-2007     Yuxin Zhou               Modified comment(s), and      */ 
/*                                            added support for IPv6,     */
/*                                            resulting in version 5.2    */
/*  08-03-2009     William E. Lamie         Modified comment(s),          */
/*                                            resulting in version 5.3    */
/*  11-23-2009     Yuxin Zhou               Modified comment(s),          */
/*                                            resulting in version 5.4    */
/*  06-01-2010     Yuxin Zhou               Modified comment(s),          */
/*                                            resulting in version 5.5    */
/*  10-10-2011     Yuxin Zhou               Modified comment(s), added    */
/*                                            prototype for new service,  */ 
/*                                            resulting in version 5.6    */
/*  01-31-2013     Yuxin Zhou               Modified comment(s),          */
/*                                            resulting in version 5.7    */
/*  xx-xx-xxxx     Yuxin Zhou               Modified comment(s), and      */
/*                                            added router alert feature. */
/*                                            added new service           */
/*                                            nx_ip_gateway_address_clear,*/
/*                                            resulting in version 5.7-sp1*/
/*                                                                        */ 
/**************************************************************************/ 

#ifndef NX_IPV4_H
#define NX_IPV4_H


#define NX_IP_VERSION               0x45000000UL    /* Version 4, Length of 5   */

#define NX_IP_NORMAL_LENGTH         5               /* Normal IP header length  */ 
#define NX_IP_HEADER_LENGTH_ENCODE_6 6              /* IP header length 6. */

/* Define IP options.  */

#define NX_IP_OPTION_COPY_FLAG              0x80000000UL    /* All fragments must carry the option. */
#define NX_IP_OPTION_CLASS                  0x00000000UL    /* Control. */
#define NX_IP_OPTION_ROUTER_ALERT_NUMBER    0x14000000UL    
#define NX_IP_OPTION_ROUTER_ALERT_LENGTH    0x00040000UL   
#define NX_IP_OPTION_ROUTER_ALERT_VALUE     0x00000000UL  

/* Define IP fragmenting information.  */

#define NX_IP_DONT_FRAGMENT         0x00004000UL    /* Don't fragment bit       */
#define NX_IP_MORE_FRAGMENT         0x00002000UL    /* More fragments           */
#define NX_IP_FRAGMENT_MASK         0x00003FFFUL    /* Mask for fragment bits   */ 
#define NX_IP_OFFSET_MASK           0x00001FFFUL    /* Mask for fragment offset */ 
#define NX_IP_ALIGN_FRAGS           8               /* Fragment alignment       */ 

/* Define basic IP Header constant.  */

/* Define Basic Internet packet header data type.  This will be used to
   build new IP packets and to examine incoming packets into NetX.  */

typedef  struct NX_IPV4_HEADER_STRUCT
{
    /* Define the first 32-bit word of the IP header.  This word contains 
       the following information:  

            bits 31-28  IP Version = 0x4  (IP Version4)
            bits 27-24  IP Header Length of 32-bit words (5 if no options)
            bits 23-16  IP Type of Service, where 0x00 -> Normal
                                                  0x10 -> Minimize Delay
                                                  0x08 -> Maximize Throughput
                                                  0x04 -> Maximize Reliability
                                                  0x02 -> Minimize Monetary Cost
            bits 15-0   IP Datagram length in bytes  
     */
    ULONG       nx_ip_header_word_0;

    /* Define the second word of the IP header.  This word contains
       the following information:

            bits 31-16  IP Packet Identification (just an incrementing number)
            bits 15-0   IP Flags and Fragment Offset (used for IP fragmenting)
                            bit  15         Zero
                            bit  14         Don't Fragment
                            bit  13         More Fragments
                            bits 12-0       (Fragment offset)/8
     */
    ULONG       nx_ip_header_word_1;

    /* Define the third word of the IP header.  This word contains 
       the following information:

            bits 31-24  IP Time-To-Live (maximum number of routers 
                                         packet can traverse before being
                                         thrown away.  Default values are typically
                                         32 or 64)
            bits 23-16  IP Protocol, where  0x01 -> ICMP Messages
                                            0x02 -> IGMP Messages
                                            0x06 -> TCP  Messages
                                            0x11 -> UDP  Messages
            bits 15-0   IP Checksum
     */
    ULONG       nx_ip_header_word_2;

    /* Define the source IP address.  */
    ULONG       nx_ip_header_source_ip;

    /* Define the destination IP address.  */
    ULONG       nx_ip_header_destination_ip;

} NX_IPV4_HEADER;


/* Define IPv4 function prototypes.  */

UINT  _nx_ip_address_get(NX_IP *ip_ptr, ULONG *ip_address, ULONG *network_mask);
UINT  _nx_ip_address_set(NX_IP *ip_ptr, ULONG ip_address, ULONG network_mask);
UINT  _nx_ip_gateway_address_set(NX_IP *ip_ptr, ULONG ip_address);
UINT  _nx_ip_gateway_address_clear(NX_IP *ip_ptr);
void  _nx_ip_packet_send(NX_IP *ip_ptr, NX_PACKET *packet_ptr, ULONG destination_ip, ULONG type_of_service, ULONG time_to_live, ULONG protocol, ULONG fragment);
VOID  _nx_ip_periodic_timer_entry(ULONG ip_address);
VOID  _nx_ipv4_packet_receive(NX_IP *ip_ptr, NX_PACKET *packet_ptr);
VOID  _nxe_ipv4_packet_receive(NX_IP *ip_ptr, NX_PACKET *packet_ptr);

/* Define error checking shells for API services.  These are only referenced by the 
   application.  */

UINT  _nxe_ip_address_get(NX_IP *ip_ptr, ULONG *ip_address, ULONG *network_mask);
UINT  _nxe_ip_address_set(NX_IP *ip_ptr, ULONG ip_address, ULONG network_mask);
UINT  _nxe_ip_gateway_address_set(NX_IP *ip_ptr, ULONG ip_address);
UINT  _nxe_ip_gateway_address_clear(NX_IP *ip_ptr);
UINT  _nxe_ip_raw_packet_send(NX_IP *ip_ptr, NX_PACKET **packet_ptr_ptr, 
                            ULONG destination_ip, ULONG type_of_service);
UINT  _nx_ip_raw_packet_send(NX_IP *ip_ptr, NX_PACKET *packet_ptr, 
                            ULONG destination_ip, ULONG type_of_service);
UINT  _nx_ip_raw_packet_source_send(NX_IP *ip_ptr, NX_PACKET *packet_ptr, ULONG destination_ip, UINT address_index, ULONG type_of_service);

#define CHECK_IPV4_ADDRESS_BY_PREFIX(ip1, ip2, prefix_len)      \
    (((ip1) & ((0xFFFFFFFF) << (32 - (prefix_len)))) ==         \
     ((ip2) & ((0xFFFFFFFF) << (32 - (prefix_len)))))

#ifndef FEATURE_NX_IPV6
#ifdef NX_IPSEC_ENABLE
#define AUTHENTICATION_HEADER   5
#define ENCAP_SECURITY_HEADER   6
#endif /* NX_IPSEC_ENABLE */
#endif /* FEATURE_NX_IPV6 */
#endif /* NX_IPV4_H */
