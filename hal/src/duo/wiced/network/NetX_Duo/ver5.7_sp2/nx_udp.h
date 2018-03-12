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
/**   User Datagram Protocol (UDP)                                        */ 
/**                                                                       */ 
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  COMPONENT DEFINITION                                   RELEASE        */ 
/*                                                                        */ 
/*    nx_udp.h                                            PORTABLE C      */ 
/*                                                           5.7          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    William E. Lamie, Express Logic, Inc.                               */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the NetX User Datagram Protocol (UDP) component,  */ 
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
/*  06-01-2010     Yuxin Zhou               Removed internal debug logic, */
/*                                            resulting in version 5.5    */
/*  10-10-2011     Yuxin Zhou               Modified comment(s), added    */
/*                                            new service APIS,           */
/*                                            resulting in version 5.6    */
/*  01-31-2013     Yuxin Zhou               Modified comment(s),          */
/*                                            resulting in version 5.7    */
/*                                                                        */ 
/**************************************************************************/ 

#ifndef NX_UDP_H
#define NX_UDP_H


/* Define UDP constants.  */

#define NX_UDP_ID                   ((ULONG) 0x55445020)


/* Define Basic UDP packet header data type.  This will be used to
   build new UDP packets and to examine incoming packets into NetX.  */

typedef struct NX_UDP_HEADER_STRUCT
{

    /* Define the first 32-bit word of the UDP header.  This word contains 
       the following information:  

            bits 31-16  UDP 16-bit source port number
            bits 15-0   UDP 16-bit destination port number
     */
    ULONG       nx_udp_header_word_0;

    /* Define the second and final word of the UDP header.  This word contains
       the following information:

            bits 31-16  UDP 16-bit UDP length (including 8 header bytes)
            bits 15-0   UDP 16-bit checksum (including header and pseudo IP header)
     */
    ULONG       nx_udp_header_word_1;

} NX_UDP_HEADER;


/* Define UDP component function prototypes.  */
UINT  _nxd_udp_socket_send(NX_UDP_SOCKET *socket_ptr, NX_PACKET *packet_ptr, NXD_ADDRESS *ip_address, UINT port);

VOID  _nx_udp_bind_cleanup(TX_THREAD *thread_ptr);
UINT  _nx_udp_enable(NX_IP *ip_ptr);
UINT  _nx_udp_free_port_find(NX_IP *ip_ptr, UINT port, UINT *free_port_ptr);
UINT  _nx_udp_info_get(NX_IP *ip_ptr, ULONG *udp_packets_sent, ULONG *udp_bytes_sent, 
                       ULONG *udp_packets_received, ULONG *udp_bytes_received,
                       ULONG *udp_invalid_packets, ULONG *udp_receive_packets_dropped,
                       ULONG *udp_checksum_errors);
VOID  _nx_udp_initialize(VOID);
VOID  _nx_udp_packet_receive(NX_IP *ip_ptr, NX_PACKET *packet_ptr);
VOID  _nx_udp_receive_cleanup(TX_THREAD *thread_ptr);
UINT  _nx_udp_socket_bind(NX_UDP_SOCKET *socket_ptr, UINT  port, ULONG wait_option);
UINT  _nx_udp_socket_bytes_available(NX_UDP_SOCKET *socket_ptr, ULONG *bytes_available);
UINT  _nx_udp_socket_checksum_disable(NX_UDP_SOCKET *socket_ptr);
UINT  _nx_udp_socket_checksum_enable(NX_UDP_SOCKET *socket_ptr);
UINT  _nx_udp_socket_create(NX_IP *ip_ptr, NX_UDP_SOCKET *socket_ptr, const CHAR *name,
                            ULONG type_of_service, ULONG fragment, UINT time_to_live, ULONG queue_maximum);
UINT  _nx_udp_socket_delete(NX_UDP_SOCKET *socket_ptr);
UINT  _nx_udp_socket_info_get(NX_UDP_SOCKET *socket_ptr, ULONG *udp_packets_sent, ULONG *udp_bytes_sent, 
                            ULONG *udp_packets_received, ULONG *udp_bytes_received, ULONG *udp_packets_queued,
                            ULONG *udp_receive_packets_dropped, ULONG *udp_checksum_errors);
UINT  _nxd_udp_socket_source_send(NX_UDP_SOCKET *socket_ptr, NX_PACKET *packet_ptr, NXD_ADDRESS *ip_address, UINT port, UINT address_index);
UINT  _nxde_udp_socket_source_send(NX_UDP_SOCKET *socket_ptr, NX_PACKET *packet_ptr, NXD_ADDRESS *ip_address, UINT port, UINT address_index);
UINT  _nx_udp_socket_port_get(NX_UDP_SOCKET *socket_ptr, UINT *port_ptr);
UINT  _nx_udp_socket_receive(NX_UDP_SOCKET *socket_ptr, NX_PACKET **packet_ptr, 
                            ULONG wait_option);
UINT  _nx_udp_socket_receive_notify(NX_UDP_SOCKET *socket_ptr, 
                            VOID (*udp_receive_notify)(NX_UDP_SOCKET *socket_ptr));

UINT  _nx_udp_socket_send(NX_UDP_SOCKET *socket_ptr, NX_PACKET *packet_ptr, 
                          ULONG ip_address, UINT port);
UINT  _nx_udp_socket_unbind(NX_UDP_SOCKET *socket_ptr);
UINT  _nx_udp_source_extract(NX_PACKET *packet_ptr, ULONG *ip_address, UINT *port);
UINT  _nxd_udp_source_extract(NX_PACKET *packet_ptr, NXD_ADDRESS *ip_address, UINT *port);

/* Define error checking shells for API services.  These are only referenced by the 
   application.  */

UINT  _nxde_udp_socket_send(NX_UDP_SOCKET *socket_ptr, NX_PACKET **packet_ptr, NXD_ADDRESS *ip_address, UINT port);
UINT  _nxe_udp_enable(NX_IP *ip_ptr);
UINT  _nxe_udp_free_port_find(NX_IP *ip_ptr, UINT port, UINT *free_port_ptr);
UINT  _nxe_udp_info_get(NX_IP *ip_ptr, ULONG *udp_packets_sent, ULONG *udp_bytes_sent, 
                        ULONG *udp_packets_received, ULONG *udp_bytes_received,
                        ULONG *udp_invalid_packets, ULONG *udp_receive_packets_dropped,
                        ULONG *udp_checksum_errors);
UINT  _nxe_udp_socket_bind(NX_UDP_SOCKET *socket_ptr, UINT  port, ULONG wait_option);
UINT  _nxe_udp_socket_bytes_available(NX_UDP_SOCKET *socket_ptr, ULONG *bytes_available);
UINT  _nxe_udp_socket_checksum_disable(NX_UDP_SOCKET *socket_ptr);
UINT  _nxe_udp_socket_checksum_enable(NX_UDP_SOCKET *socket_ptr);
UINT  _nxe_udp_socket_create(NX_IP *ip_ptr, NX_UDP_SOCKET *socket_ptr, const CHAR *name,
                             ULONG type_of_service, ULONG fragment, UINT time_to_live, ULONG queue_maximum, UINT udp_socket_size);
UINT  _nxe_udp_socket_delete(NX_UDP_SOCKET *socket_ptr);

UINT  _nxe_udp_source_extract(NX_PACKET *packet_ptr, ULONG *ip_address, UINT *port);
UINT  _nxde_udp_source_extract(NX_PACKET *packet_ptr, NXD_ADDRESS *ip_address, UINT *port);
UINT  _nxe_udp_socket_info_get(NX_UDP_SOCKET *socket_ptr, ULONG *udp_packets_sent, ULONG *udp_bytes_sent, 
                               ULONG *udp_packets_received, ULONG *udp_bytes_received, ULONG *udp_packets_queued,
                               ULONG *udp_receive_packets_dropped, ULONG *udp_checksum_errors);
UINT  _nxe_udp_socket_port_get(NX_UDP_SOCKET *socket_ptr, UINT *port_ptr);
UINT  _nxe_udp_socket_receive(NX_UDP_SOCKET *socket_ptr, NX_PACKET **packet_ptr, 
                              ULONG wait_option);
UINT  _nxe_udp_socket_receive_notify(NX_UDP_SOCKET *socket_ptr, 
                                     VOID (*udp_receive_notify)(NX_UDP_SOCKET *socket_ptr));
UINT  _nx_udp_socket_source_send(NX_UDP_SOCKET *socket_ptr, NX_PACKET *packet_ptr, 
                                    ULONG ip_address, UINT port, UINT address_index);
UINT  _nxe_udp_socket_source_send(NX_UDP_SOCKET *socket_ptr, NX_PACKET **packet_ptr, 
                                     ULONG ip_address, UINT port, UINT source_index);
UINT  _nxe_udp_socket_send(NX_UDP_SOCKET *socket_ptr, NX_PACKET **packet_ptr_ptr, 
                           ULONG ip_address, UINT port);
UINT  _nxe_udp_socket_unbind(NX_UDP_SOCKET *socket_ptr);



UINT  _nxd_udp_packet_info_extract(NX_PACKET *packet_ptr, NXD_ADDRESS *ip_address, 
                                   UINT *protocol, UINT *port, UINT *interface_index);
UINT  _nxde_udp_packet_info_extract(NX_PACKET *packet_ptr, NXD_ADDRESS *ip_address, 
                                    UINT *protocol, UINT *port, UINT *interface_index);
UINT  _nx_udp_packet_info_extract(NX_PACKET *packet_ptr, ULONG *ip_address, UINT *protocol, UINT *port, UINT *interface_index);
UINT  _nxe_udp_packet_info_extract(NX_PACKET *packet_ptr, ULONG *ip_address, UINT *protocol, UINT *port, UINT *interface_index);
UINT  _nxd_udp_socket_send(NX_UDP_SOCKET *socket_ptr, NX_PACKET *packet_ptr, NXD_ADDRESS *ip_address, UINT port);
UINT  _nxde_udp_socket_send(NX_UDP_SOCKET *socket_ptr, NX_PACKET **packet_ptr, NXD_ADDRESS *ip_address, UINT port);

/* UDP component data declarations follow.  */

#endif
