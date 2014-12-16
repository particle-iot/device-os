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
/*    nx_ip.h                                             PORTABLE C      */ 
/*                                                           5.5          */ 
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
/*  07-04-2009     William E. Lamie         Modified comment(s), and added*/ 
/*                                            logic for trace support,    */ 
/*                                            resulting in version 5.2    */
/*  12-31-2009     Yuxin Zhou               Added multi-home support,     */
/*                                            removed internal debug logic*/  
/*                                            resulting in version 5.3    */ 
/*  06-30-2011     Yuxin Zhou               Modified comment(s), and      */ 
/*                                            added macro to control the  */ 
/*                                            amount of wait in IP status */ 
/*                                            check, resulting in         */ 
/*                                            version 5.4                 */ 
/*  04-30-2013     Yuxin Zhou               Modified comment(s),          */
/*                                            resulting in version 5.5    */
/*  xx-xx-xxxx     Yuxin Zhou               Modified comment(s), added    */
/*                                            new service,                */ 
/*                                            resulting in version 5.5-sp1*/ 
/*                                                                        */ 
/**************************************************************************/ 

#ifndef NX_IP_H
#define NX_IP_H


/* Define IP constants.  */

#define NX_IP_ID                    ((ULONG) 0x49502020)


/* Define basic IP Header constant.  */

#define NX_IP_VERSION               ((ULONG) 0x45000000)    /* Version 4, Length of 5   */


/* Define the mask for the IP header length field.  */

#define NX_IP_LENGTH_MASK           ((ULONG) 0x0F000000)    /* Mask for length bit      */ 
#define NX_IP_NORMAL_LENGTH         5                       /* Normal IP header length  */ 


/* Define IP fragmenting information.  */

#define NX_IP_DONT_FRAGMENT         ((ULONG) 0x00004000)    /* Don't fragment bit       */
#define NX_IP_MORE_FRAGMENT         ((ULONG) 0x00002000)    /* More fragments           */
#define NX_IP_FRAGMENT_MASK         ((ULONG) 0x00003FFF)    /* Mask for fragment bits   */ 
#define NX_IP_OFFSET_MASK           ((ULONG) 0x00001FFF)    /* Mask for fragment offset */ 
#define NX_IP_ALIGN_FRAGS           8                       /* Fragment alignment       */ 


/* Define IP event flags.  These events are processed by the IP thread. */

#define NX_IP_ALL_EVENTS            ((ULONG) 0xFFFFFFFF)    /* All event flags              */
#define NX_IP_PERIODIC_EVENT        ((ULONG) 0x00000001)    /* Periodic ARP event           */
#define NX_IP_UNFRAG_EVENT          ((ULONG) 0x00000002)    /* Unfragment event             */ 
#define NX_IP_ICMP_EVENT            ((ULONG) 0x00000004)    /* ICMP message event           */ 
#define NX_IP_RECEIVE_EVENT         ((ULONG) 0x00000008)    /* IP receive packet event      */ 
#define NX_IP_ARP_REC_EVENT         ((ULONG) 0x00000010)    /* ARP deferred receive event   */ 
#define NX_IP_RARP_REC_EVENT        ((ULONG) 0x00000020)    /* RARP deferred receive event  */ 
#define NX_IP_IGMP_EVENT            ((ULONG) 0x00000040)    /* IGMP message event           */ 
#define NX_IP_TCP_EVENT             ((ULONG) 0x00000080)    /* TCP message event            */ 
#define NX_IP_TCP_FAST_EVENT        ((ULONG) 0x00000100)    /* Fast TCP timer event         */ 
#define NX_IP_DRIVER_PACKET_EVENT   ((ULONG) 0x00000200)    /* Driver Deferred packet event */ 
#define NX_IP_IGMP_ENABLE_EVENT     ((ULONG) 0x00000400)    /* IGMP enable event            */ 
#define NX_IP_DRIVER_DEFERRED_EVENT ((ULONG) 0x00000800)    /* Driver deferred processing   */ 
                                                            /*   event                      */ 
#define NX_IP_TCP_CLEANUP_DEFERRED  ((ULONG) 0x00001000)    /* Deferred TCP cleanup event   */ 


#ifndef NX_IP_DEBUG_LOG_SIZE
#define NX_IP_DEBUG_LOG_SIZE        100                     /* Maximum size of optional log  */ 
#endif


/* Define the amount of time to sleep in nx_ip_(interface_)status_check */
 
#ifndef NX_IP_STATUS_CHECK_WAIT_TIME
#define NX_IP_STATUS_CHECK_WAIT_TIME 1
#endif /* NX_IP_STATUS_CHECK_WAIT_TIME */


/* Define Basic Internet packet header data type.  This will be used to
   build new IP packets and to examine incoming packets into NetX.  */

typedef  struct NX_IP_HEADER_STRUCT
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

} NX_IP_HEADER;


/* Define IP function prototypes.  */

UINT  _nx_ip_address_change_notify(NX_IP *ip_ptr, VOID (*ip_address_change_notify)(NX_IP *, VOID *), VOID *additional_info);
UINT  _nx_ip_address_get(NX_IP *ip_ptr, ULONG *ip_address, ULONG *network_mask);
UINT  _nx_ip_address_set(NX_IP *ip_ptr, ULONG ip_address, ULONG network_mask);
UINT  _nx_ip_interface_address_get(NX_IP *ip_ptr, ULONG interface_index, ULONG *ip_address, ULONG *network_mask);
UINT  _nx_ip_interface_address_set(NX_IP *ip_ptr, ULONG interface_index, ULONG ip_address, ULONG network_mask);
UINT  _nx_ip_interface_info_get(NX_IP *ip_ptr, UINT interface_index, const CHAR **interface_name, ULONG *ip_address,
                                ULONG *network_mask, ULONG *mtu_size, ULONG *phsyical_address_msw, 
                                ULONG *physical_address_lsw);
UINT  _nx_ip_interface_status_check(NX_IP *ip_ptr, UINT interface_index, ULONG needed_status, ULONG *actual_status, 
                                    ULONG wait_option);
UINT  _nx_ip_create(NX_IP *ip_ptr, const CHAR *name, ULONG ip_address, ULONG network_mask,
                            NX_PACKET_POOL *default_pool, 
                            VOID (*ip_link_driver)(NX_IP_DRIVER *),
                            VOID *memory_ptr, ULONG memory_size, UINT priority);
UINT  _nx_ip_delete(NX_IP *ip_ptr);
VOID  _nx_ip_delete_queue_clear(NX_PACKET *head_ptr);
VOID  _nx_ip_driver_deferred_enable(NX_IP *ip_ptr, VOID (*driver_deferred_packet_handler)(NX_IP *ip_ptr, NX_PACKET *packet_ptr));
VOID  _nx_ip_driver_deferred_processing(NX_IP *ip_ptr);
VOID  _nx_ip_driver_deferred_receive(NX_IP *ip_ptr, NX_PACKET *packet_ptr);
UINT  _nx_ip_driver_direct_command(NX_IP *ip_ptr, UINT command, ULONG *return_value_ptr);
UINT  _nx_ip_driver_interface_direct_command(NX_IP *ip_ptr, UINT command, UINT interface_index, ULONG *return_value_ptr);
UINT  _nx_ip_forwarding_disable(NX_IP *ip_ptr);
UINT  _nx_ip_forwarding_enable(NX_IP *ip_ptr);
UINT  _nx_ip_fragment_disable(NX_IP *ip_ptr);
UINT  _nx_ip_fragment_enable(NX_IP *ip_ptr);
UINT  _nx_ip_gateway_address_set(NX_IP *ip_ptr, ULONG ip_address);
UINT  _nx_ip_info_get(NX_IP *ip_ptr, ULONG *ip_total_packets_sent, ULONG *ip_total_bytes_sent, 
                            ULONG *ip_total_packets_received, ULONG *ip_total_bytes_received,
                            ULONG *ip_invalid_packets, ULONG *ip_receive_packets_dropped,
                            ULONG *ip_receive_checksum_errors, ULONG *ip_send_packets_dropped,
                            ULONG *ip_total_fragments_sent, ULONG *ip_total_fragments_received);
UINT  _nx_ip_interface_attach(NX_IP *ip_ptr, const CHAR *interface_name, ULONG ip_address, ULONG network_mask, VOID(*ip_link_driver)(struct NX_IP_DRIVER_STRUCT *));
UINT  _nx_ip_raw_packet_disable(NX_IP *ip_ptr);
UINT  _nx_ip_raw_packet_enable(NX_IP *ip_ptr);
UINT  _nx_ip_raw_packet_receive(NX_IP *ip_ptr, NX_PACKET **packet_ptr, ULONG wait_option);
UINT  _nx_ip_raw_packet_send(NX_IP *ip_ptr, NX_PACKET *packet_ptr, 
                            ULONG destination_ip, ULONG type_of_service);
UINT  _nx_ip_raw_packet_interface_send(NX_IP *ip_ptr, NX_PACKET *packet_ptr, 
                                       ULONG destination_ip, UINT interface_index, ULONG type_of_service);
VOID  _nx_ip_initialize(VOID);
VOID  _nx_ip_forward_packet_process(NX_IP *ip_ptr, NX_PACKET *packet_ptr);
VOID  _nx_ip_fragment_timeout_check(NX_IP *ip_ptr);
VOID  _nx_ip_loopback_send(NX_IP *ip_ptr, NX_PACKET *packet_ptr, UINT packet_release);
ULONG _nx_ip_route_find(NX_IP *ip_ptr, ULONG destination_address, NX_INTERFACE **nx_ip_interface, ULONG *next_hop_address);
VOID  _nx_ip_periodic_timer_entry(ULONG ip_address);
VOID  _nx_ip_packet_send(NX_IP *ip_ptr, NX_PACKET *packet_ptr, ULONG destination_ip, ULONG type_of_service, ULONG time_to_live, ULONG protocol, ULONG fragment);
VOID  _nx_ip_packet_receive(NX_IP *ip_ptr, NX_PACKET *packet_ptr);
VOID  _nx_ip_packet_deferred_receive(NX_IP *ip_ptr, NX_PACKET *packet_ptr);
UINT  _nx_ip_status_check(NX_IP *ip_ptr, ULONG needed_status, ULONG *actual_status, 
                            ULONG wait_option);
VOID  _nx_ip_thread_entry(ULONG ip_ptr_value);
VOID  _nx_ip_raw_packet_cleanup(TX_THREAD *thread_ptr);
VOID  _nx_ip_raw_packet_processing(NX_IP *ip_ptr, NX_PACKET *packet_ptr);
VOID  _nx_ip_fragment_packet(struct NX_IP_DRIVER_STRUCT *driver_req_ptr);
VOID  _nx_ip_fragment_assembly(NX_IP *ip_ptr);

UINT  _nx_ip_static_route_add(NX_IP *ip_ptr, ULONG network_address, ULONG net_mask, ULONG next_hop);
UINT  _nx_ip_static_route_delete(NX_IP *ip_ptr, ULONG network_address, ULONG net_mask);
ULONG _nx_ip_static_route_find(NX_IP *ip_ptr, ULONG destination_address);

/* Define error checking shells for API services.  These are only referenced by the 
   application.  */

UINT  _nxe_ip_address_change_notify(NX_IP *ip_ptr, VOID (*ip_address_change_notify)(NX_IP *, VOID *), VOID *additional_info);
UINT  _nxe_ip_address_get(NX_IP *ip_ptr, ULONG *ip_address, ULONG *network_mask);
UINT  _nxe_ip_address_set(NX_IP *ip_ptr, ULONG ip_address, ULONG network_mask);
UINT  _nxe_ip_interface_address_get(NX_IP *ip_ptr, ULONG interface_index, ULONG *ip_address, ULONG *network_mask);
UINT  _nxe_ip_interface_address_set(NX_IP *ip_ptr, ULONG interface_index, ULONG ip_address, ULONG network_mask);
UINT  _nxe_ip_create(NX_IP *ip_ptr, const CHAR *name, ULONG ip_address, ULONG network_mask,
                            NX_PACKET_POOL *default_pool, 
                            VOID (*ip_link_driver)(NX_IP_DRIVER *),
                            VOID *memory_ptr, ULONG memory_size, UINT priority, UINT ip_control_block_size);
UINT  _nxe_ip_delete(NX_IP *ip_ptr);
UINT  _nxe_ip_driver_direct_command(NX_IP *ip_ptr, UINT command, ULONG *return_value_ptr);
UINT  _nxe_ip_driver_interface_direct_command(NX_IP *ip_ptr, UINT command, UINT interface_index, ULONG *return_value_ptr);
UINT  _nxe_ip_forwarding_disable(NX_IP *ip_ptr);
UINT  _nxe_ip_forwarding_enable(NX_IP *ip_ptr);
UINT  _nxe_ip_fragment_disable(NX_IP *ip_ptr);
UINT  _nxe_ip_fragment_enable(NX_IP *ip_ptr);
UINT  _nxe_ip_gateway_address_set(NX_IP *ip_ptr, ULONG ip_address);
UINT  _nxe_ip_info_get(NX_IP *ip_ptr, ULONG *ip_total_packets_sent, ULONG *ip_total_bytes_sent, 
                            ULONG *ip_total_packets_received, ULONG *ip_total_bytes_received,
                            ULONG *ip_invalid_packets, ULONG *ip_receive_packets_dropped,
                            ULONG *ip_receive_checksum_errors, ULONG *ip_send_packets_dropped,
                            ULONG *ip_total_fragments_sent, ULONG *ip_total_fragments_received);
UINT  _nxe_ip_raw_packet_disable(NX_IP *ip_ptr);
UINT  _nxe_ip_raw_packet_enable(NX_IP *ip_ptr);
UINT  _nxe_ip_raw_packet_receive(NX_IP *ip_ptr, NX_PACKET **packet_ptr, ULONG wait_option);
UINT  _nxe_ip_raw_packet_send(NX_IP *ip_ptr, NX_PACKET **packet_ptr_ptr, 
                            ULONG destination_ip, ULONG type_of_service);
UINT  _nxe_ip_raw_packet_interface_send(NX_IP *ip_ptr, NX_PACKET **packet_ptr_ptr, 
                                        ULONG destination_ip, UINT interface_index, ULONG type_of_service);
UINT  _nxe_ip_status_check(NX_IP *ip_ptr, ULONG needed_status, ULONG *actual_status, 
                            ULONG wait_option);

UINT  _nxe_ip_static_route_add(NX_IP *ip_ptr, ULONG network_address, ULONG net_mask, ULONG next_hop);
UINT  _nxe_ip_static_route_delete(NX_IP *ip_ptr, ULONG network_address, ULONG net_mask);
UINT  _nxe_ip_static_routing_disable(NX_IP *ip_ptr);
UINT  _nxe_ip_static_routing_enable(NX_IP *ip_ptr);


/* IP component data declarations follow.  */

/* Determine if the initialization function of this component is including
   this file.  If so, make the data definitions really happen.  Otherwise,
   make them extern so other functions in the component can access them.  */

#ifdef NX_IP_INIT
#define IP_DECLARE 
#else
#define IP_DECLARE extern
#endif


/* Define the head pointer of the created IP list.  */

IP_DECLARE  NX_IP *     _nx_ip_created_ptr;


/* Define the number of created IP instances.  */

IP_DECLARE  ULONG       _nx_ip_created_count; 


#endif

