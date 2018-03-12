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
/**   Transmission Control Protocol (TCP)                                 */ 
/**                                                                       */ 
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  COMPONENT DEFINITION                                   RELEASE        */ 
/*                                                                        */ 
/*    nx_tcp.h                                            PORTABLE C      */ 
/*                                                           5.7          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    William E. Lamie, Express Logic, Inc.                               */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the NetX Transmission Control Protocol component, */ 
/*    including all data types and external references.  It is assumed    */ 
/*    that nx_api.h and nx_port.h have already been included.             */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  12-12-2005     William E. Lamie         Initial Version 5.0           */ 
/*  08-09-2007     William E. Lamie         Modified comment(s), removed  */ 
/*                                            the SACK option from the    */ 
/*                                            end of TCP options, and     */ 
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
/*  10-10-2011     Yuxin Zhou               Modified comment(s),          */
/*                                            added support for ACK every */
/*                                            N packets feature, added    */
/*                                            support for window scaling, */
/*                                            added src IPv6 address param*/
/*                                            in TCP no connection reset, */
/*                                            added extended TCP callback,*/ 
/*                                            resulting in version 5.6    */
/*  01-31-2013     Yuxin Zhou               Modified comment(s), fixed    */
/*                                            a compiler warning,         */ 
/*                                            added support for queue     */
/*                                            depth notify callback,      */
/*                                            resulting in version 5.7    */
/*                                                                        */ 
/**************************************************************************/ 

#ifndef NX_TCP_H
#define NX_TCP_H


/* Define TCP constants.  */

#define NX_TCP_ID           ((ULONG) 0x54435020)


/* Define the TCP header typical size.  */

#define NX_TCP_HEADER_SIZE  ((ULONG) 0x50000000)    /* Typical 5 word TCP header    */ 
#define NX_TCP_SYN_HEADER   ((ULONG) 0x70000000)    /* SYN header with MSS option   */ 
#define NX_TCP_HEADER_MASK  ((ULONG) 0xF0000000)    /* TCP header size mask         */ 
#define NX_TCP_HEADER_SHIFT 28                      /* Shift down to pickup length  */ 


/* Define the TCP header control fields.  */

#define NX_TCP_CONTROL_MASK ((ULONG) 0x00170000)    /* ACK, RST, SYN, and FIN bits  */ 
#define NX_TCP_URG_BIT      ((ULONG) 0x00200000)    /* Urgent data bit              */ 
#define NX_TCP_ACK_BIT      ((ULONG) 0x00100000)    /* Acknowledgement bit          */ 
#define NX_TCP_PSH_BIT      ((ULONG) 0x00080000)    /* Push bit                     */ 
#define NX_TCP_RST_BIT      ((ULONG) 0x00040000)    /* Reset bit                    */ 
#define NX_TCP_SYN_BIT      ((ULONG) 0x00020000)    /* Sequence bit                 */ 
#define NX_TCP_FIN_BIT      ((ULONG) 0x00010000)    /* Finish bit                   */ 


/* Define the MSS option for the TCP header.  */

#define NX_TCP_MSS_OPTION   ((ULONG) 0x02040000)    /* Maximum Segment Size option  */ 
#define NX_TCP_RWIN_OPTION  ((ULONG) 0x03030000)    /* 24 bits, so NOP, 0x3, 0x3, scale value  */ 
#define NX_TCP_MSS_SIZE     1460                    /* Maximum Segment Size         */ 
#define NX_TCP_OPTION_END   ((ULONG) 0x01010100)    /* NOPs and end of TCP options  */ 
#define NX_TCP_EOL_KIND     0x00                    /* EOL option kind              */ 
#define NX_TCP_NOP_KIND     0x01                    /* NOP option kind              */ 
#define NX_TCP_MSS_KIND     0x02                    /* MSS option kind              */ 
#define NX_TCP_RWIN_KIND    0x03                    /* RWIN option kind             */


/* Define constants for the optional TCP keepalive Timer.  To enable this 
   feature, the TCP source must be compiled with NX_TCP_ENABLE_KEEPALIVE
   defined.  */

#ifndef NX_TCP_KEEPALIVE_INITIAL
#define NX_TCP_KEEPALIVE_INITIAL    7200            /* Number of seconds for initial */ 
#endif                                              /*   keepalive expiration, the   */ 
                                                    /*   default is 2 hours (120 min)*/ 
#ifndef NX_TCP_KEEPALIVE_RETRY  
#define NX_TCP_KEEPALIVE_RETRY      75              /* After initial expiration,     */ 
#endif                                              /*   retry every 75 seconds      */ 

#ifndef NX_TCP_KEEPALIVE_RETRIES
#define NX_TCP_KEEPALIVE_RETRIES    10              /* Retry a maximum of 10 times   */ 
#endif

#ifndef NX_TCP_MAXIMUM_TX_QUEUE
#define NX_TCP_MAXIMUM_TX_QUEUE     20              /* Maximum number of transmit    */ 
#endif                                              /*   packets queued              */ 

#ifndef NX_TCP_MAXIMUM_RETRIES
#define NX_TCP_MAXIMUM_RETRIES      10              /* Maximum number of transmit    */ 
#endif                                              /*   retries allowed             */ 

#ifndef NX_TCP_RETRY_SHIFT
#define NX_TCP_RETRY_SHIFT          0               /* Shift that is applied to      */ 
#endif                                              /*   last timeout for back off,  */ 
                                                    /*   i.e. a value of zero means  */ 
                                                    /*   constant timeouts, a value  */ 
                                                    /*   of 1 causes each successive */  
                                                    /*   be multiplied by two, etc.  */ 


/* Define the rate for the TCP fast periodic timer.  This timer is used to process
   delayed ACKs and packet re-transmission.  Hence, it must have greater resolution 
   than the 200ms delayed ACK requirement.  By default, the fast periodic timer is 
   setup on a 100ms periodic.  The number supplied is used to divide the 
   _nx_system_ticks_per_second variable to actually derive the ticks.  Dividing
   by 10 yields a 100ms base periodic.  */ 

#ifndef NX_TCP_FAST_TIMER_RATE
#define NX_TCP_FAST_TIMER_RATE      10
#endif


/* Define the rate for the TCP delayed ACK timer, which by default is 200ms.  The 
   number supplied is used to divide the _nx_system_ticks_per_second variable to 
   actually derive the ticks.  Dividing by 5 yields a 200ms periodic.  */

#ifndef NX_TCP_ACK_TIMER_RATE
#define NX_TCP_ACK_TIMER_RATE       5
#endif

/* Define the rate for the TCP retransmit timer, which by default is set to 
   one second.  The number supplied is used to divide the _nx_system_ticks_per_second 
   variable to actually derive the ticks.  Dividing by 1 yields a 1 second periodic.  */

#ifndef NX_TCP_TRANSMIT_TIMER_RATE
#define NX_TCP_TRANSMIT_TIMER_RATE  1
#endif


/* Define Basic TCP packet header data type.  This will be used to
   build new TCP packets and to examine incoming packets into NetX.  */

typedef  struct NX_TCP_HEADER_STRUCT
{

    /* Define the first 32-bit word of the TCP header.  This word contains 
       the following information:  

            bits 31-16  TCP 16-bit source port number
            bits 15-0   TCP 16-bit destination port number
     */
    ULONG       nx_tcp_header_word_0;

    /* Define the second word of the TCP header.  This word contains
       the following information:

            bits 31-0   TCP 32-bit sequence number
     */
    ULONG       nx_tcp_sequence_number;

    /* Define the third word of the TCP header.  This word contains
       the following information:

            bits 31-0   TCP 32-bit acknowledgment number
     */
    ULONG       nx_tcp_acknowledgment_number;

    /* Define the fourth 32-bit word of the TCP header.  This word contains 
       the following information:  

            bits 31-28  TCP 4-bit header length
            bits 27-22  TCP 6-bit reserved field
            bit  21     TCP Urgent bit (URG)
            bit  20     TCP Acknowledgement bit (ACK)
            bit  19     TCP Push bit (PSH)
            bit  18     TCP Reset connection bit (RST)
            bit  17     TCP Synchronize sequence numbers bit (SYN)
            bit  16     TCP Sender has reached the end of its byte stream (FIN)
            bits 15-0   TCP 16-bit window size
     */
    ULONG       nx_tcp_header_word_3;

    /* Define the fifth 32-bit word of the TCP header.  This word contains 
       the following information:  

            bits 31-16  TCP 16-bit TCP checksum
            bits 15-0   TCP 16-bit TCP urgent pointer
     */
    ULONG       nx_tcp_header_word_4;

} NX_TCP_HEADER;


/* Define TCP SYN packet header data type.  This will be used during the
   initial connection requests for NetX.  */

typedef  struct NX_TCP_SYN_STRUCT
{

    /* Define the first 32-bit word of the TCP header.  This word contains 
       the following information:  

            bits 31-16  TCP 16-bit source port number
            bits 15-0   TCP 16-bit destination port number
     */
    ULONG       nx_tcp_header_word_0;

    /* Define the second word of the TCP header.  This word contains
       the following information:

            bits 31-0   TCP 32-bit sequence number
     */
    ULONG       nx_tcp_sequence_number;

    /* Define the third word of the TCP header.  This word contains
       the following information:

            bits 31-0   TCP 32-bit acknowledgment number
     */
    ULONG       nx_tcp_acknowledgment_number;

    /* Define the fourth 32-bit word of the TCP header.  This word contains 
       the following information:  

            bits 31-28  TCP 4-bit header length
            bits 27-22  TCP 6-bit reserved field
            bit  21     TCP Urgent bit (URG)
            bit  20     TCP Acknowledgement bit (ACK)
            bit  19     TCP Push bit (PSH)
            bit  18     TCP Reset connection bit (RST)
            bit  17     TCP Synchronize sequence numbers bit (SYN)
            bit  16     TCP Sender has reached the end of its byte stream (FIN)
            bits 15-0   TCP 16-bit window size
     */
    ULONG       nx_tcp_header_word_3;

    /* Define the fifth 32-bit word of the TCP header.  This word contains 
       the following information:  

            bits 31-16  TCP 16-bit TCP checksum
            bits 15-0   TCP 16-bit TCP urgent pointer
     */
    ULONG       nx_tcp_header_word_4;

    /* Define the first option word of the TCP SYN header.  This word contains
       the MSS option type and the MSS itself.  */
    ULONG       nx_tcp_option_word_1;

    /* Define the second option word of the TCP SYN header.  This word contains
       NOPs primarily.  */
    ULONG       nx_tcp_option_word_2;

} NX_TCP_SYN;


/* Define TCP component API function prototypes.  */

UINT  _nxd_tcp_client_socket_connect(NX_TCP_SOCKET *socket_ptr, NXD_ADDRESS *server_ip, UINT server_port, ULONG wait_option);
UINT  _nxd_tcp_socket_peer_info_get(NX_TCP_SOCKET *socket_ptr, NXD_ADDRESS *peer_ip_address, ULONG *peer_port);
UINT  _nx_tcp_client_socket_bind(NX_TCP_SOCKET *socket_ptr, UINT port, ULONG wait_option);
UINT  _nx_tcp_client_socket_connect(NX_TCP_SOCKET *socket_ptr, ULONG server_ip, UINT server_port, ULONG wait_option);
UINT  _nx_tcp_client_socket_port_get(NX_TCP_SOCKET *socket_ptr, UINT *port_ptr);
UINT  _nx_tcp_client_socket_unbind(NX_TCP_SOCKET *socket_ptr);
UINT  _nx_tcp_enable(NX_IP *ip_ptr);
UINT  _nx_tcp_free_port_find(NX_IP *ip_ptr, UINT port, UINT *free_port_ptr);
UINT  _nx_tcp_info_get(NX_IP *ip_ptr, ULONG *tcp_packets_sent, ULONG *tcp_bytes_sent, 
                            ULONG *tcp_packets_received, ULONG *tcp_bytes_received,
                            ULONG *tcp_invalid_packets, ULONG *tcp_receive_packets_dropped,
                            ULONG *tcp_checksum_errors, ULONG *tcp_connections, 
                            ULONG *tcp_disconnections, ULONG *tcp_connections_dropped,
                            ULONG *tcp_retransmit_packets);
UINT  _nx_tcp_server_socket_accept(NX_TCP_SOCKET *socket_ptr, ULONG wait_option);
UINT  _nx_tcp_server_socket_listen(NX_IP *ip_ptr, UINT port, NX_TCP_SOCKET *socket_ptr, UINT listen_queue_size,
                            VOID (*tcp_listen_callback)(NX_TCP_SOCKET *socket_ptr, UINT port));
UINT  _nx_tcp_server_socket_relisten(NX_IP *ip_ptr, UINT port, NX_TCP_SOCKET *socket_ptr);
UINT  _nx_tcp_server_socket_unaccept(NX_TCP_SOCKET *socket_ptr);
UINT  _nx_tcp_server_socket_unlisten(NX_IP *ip_ptr, UINT port);
UINT  _nx_tcp_socket_create(NX_IP *ip_ptr, NX_TCP_SOCKET *socket_ptr, const CHAR *name,
                            ULONG type_of_service, ULONG fragment, UINT time_to_live, ULONG window_size,
                            VOID (*tcp_urgent_data_callback)(NX_TCP_SOCKET *socket_ptr),
                            VOID (*tcp_disconnect_callback)(NX_TCP_SOCKET *socket_ptr));
UINT  _nx_tcp_socket_delete(NX_TCP_SOCKET *socket_ptr);
UINT  _nx_tcp_socket_disconnect(NX_TCP_SOCKET *socket_ptr, ULONG wait_option);
UINT  _nx_tcp_socket_info_get(NX_TCP_SOCKET *socket_ptr, ULONG *tcp_packets_sent, ULONG *tcp_bytes_sent, 
                            ULONG *tcp_packets_received, ULONG *tcp_bytes_received, 
                            ULONG *tcp_retransmit_packets, ULONG *tcp_packets_queued,
                            ULONG *tcp_checksum_errors, ULONG *tcp_socket_state,
                            ULONG *tcp_transmit_queue_depth, ULONG *tcp_transmit_window,
                            ULONG *tcp_receive_window);
UINT  _nx_tcp_socket_mss_get(NX_TCP_SOCKET *socket_ptr, ULONG *mss);
UINT  _nx_tcp_socket_mss_peer_get(NX_TCP_SOCKET *socket_ptr, ULONG *peer_mss);
UINT  _nx_tcp_socket_mss_set(NX_TCP_SOCKET *socket_ptr, ULONG mss);
UINT  _nx_tcp_socket_receive(NX_TCP_SOCKET *socket_ptr, NX_PACKET **packet_ptr, ULONG wait_option);
UINT  _nx_tcp_socket_receive_notify(NX_TCP_SOCKET *socket_ptr, 
                                    VOID (*tcp_receive_notify)(NX_TCP_SOCKET *socket_ptr));
UINT  _nx_tcp_socket_window_update_notify_set(NX_TCP_SOCKET *socket_ptr, 
                                              VOID (*tcp_windows_update_notify)(NX_TCP_SOCKET *socket_ptr));
UINT  _nx_tcp_socket_send(NX_TCP_SOCKET *socket_ptr, NX_PACKET *packet_ptr, ULONG wait_option);
UINT  _nx_tcp_socket_send_internal(NX_TCP_SOCKET *socket_ptr, NX_PACKET *packet_ptr, ULONG wait_option);
UINT  _nx_tcp_socket_state_wait(NX_TCP_SOCKET *socket_ptr, UINT desired_state, ULONG wait_option);
UINT  _nx_tcp_socket_transmit_configure(NX_TCP_SOCKET *socket_ptr, ULONG max_queue_depth, ULONG timeout, 
                                        ULONG max_retries, ULONG timeout_shift);
#ifdef NX_TCP_QUEUE_DEPTH_UPDATE_NOTIFY_ENABLE     
UINT  _nx_tcp_socket_queue_depth_notify_set(NX_TCP_SOCKET *socket_ptr,  VOID (*tcp_socket_queue_depth_notify)(NX_TCP_SOCKET *socket_ptr));
#endif
#ifndef NX_DISABLE_EXTENDED_NOTIFY_SUPPORT
UINT    _nx_tcp_socket_establish_notify(NX_TCP_SOCKET *socket_ptr, VOID (*tcp_establish_notify)(NX_TCP_SOCKET *socket_ptr));
UINT    _nx_tcp_socket_disconnect_complete_notify(NX_TCP_SOCKET *socket_ptr, VOID (*tcp_disconnect_complete_notify)(NX_TCP_SOCKET *socket_ptr));
UINT    _nx_tcp_socket_timed_wait_callback(NX_TCP_SOCKET *socket_ptr, VOID (*tcp_timed_wait_callback)(NX_TCP_SOCKET *socket_ptr));
#endif
/* Define TCP component internal function prototypes.  */
VOID  _nx_tcp_cleanup_deferred(TX_THREAD *thread_ptr);
VOID  _nx_tcp_client_bind_cleanup(TX_THREAD *thread_ptr);
VOID  _nx_tcp_deferred_cleanup_check(NX_IP *ip_ptr);
VOID  _nx_tcp_fast_periodic_processing(NX_IP *ip_ptr);
VOID  _nx_tcp_fast_periodic_timer_entry(ULONG ip_address);
VOID  _nx_tcp_connect_cleanup(TX_THREAD *thread_ptr);
VOID  _nx_tcp_disconnect_cleanup(TX_THREAD *thread_ptr);
VOID  _nx_tcp_initialize(VOID);
ULONG _nx_tcp_mss_option_get(UCHAR *option_ptr, ULONG option_area_size);
ULONG _nx_tcp_window_scaling_option_get(UCHAR *option_ptr, ULONG option_area_size);
VOID  _nx_tcp_no_connection_reset(NX_IP *ip_ptr, struct NXD_IPV6_ADDRESS_STRUCT *src_ipv6_address, UINT port, UINT source_port, NXD_ADDRESS *source_ip, NX_TCP_HEADER *tcp_header_ptr);
VOID  _nx_tcp_packet_process(NX_IP *ip_ptr, NX_PACKET *packet_ptr);
VOID  _nx_tcp_packet_receive(NX_IP *ip_ptr, NX_PACKET *packet_ptr);
VOID  _nx_tcp_packet_send_ack(NX_TCP_SOCKET *socket_ptr, ULONG tx_sequence);
VOID  _nx_tcp_packet_send_fin(NX_TCP_SOCKET *socket_ptr, ULONG tx_sequence);
VOID  _nx_tcp_packet_send_rst(NX_TCP_SOCKET *socket_ptr, NX_TCP_HEADER *header_ptr);
VOID  _nx_tcp_packet_send_syn(NX_TCP_SOCKET *socket_ptr, ULONG tx_sequence);
VOID  _nx_tcp_periodic_processing(NX_IP *ip_ptr);
VOID  _nx_tcp_queue_process(NX_IP *ip_ptr);
VOID  _nx_tcp_receive_cleanup(TX_THREAD *thread_ptr);
UINT  _nx_tcp_socket_bytes_available(NX_TCP_SOCKET *socket_ptr, ULONG *bytes_available);
VOID  _nx_tcp_socket_connection_reset(NX_TCP_SOCKET *socket_ptr);
VOID  _nx_tcp_socket_packet_process(NX_TCP_SOCKET *socket_ptr, NX_PACKET *packet_ptr);
UINT  _nx_tcp_socket_peer_info_get(NX_TCP_SOCKET *socket_ptr, ULONG *peer_ip_address, ULONG *peer_port);

VOID  _nx_tcp_socket_receive_queue_flush(NX_TCP_SOCKET *socket_ptr);
UINT  _nx_tcp_socket_state_ack_check(NX_TCP_SOCKET *socket_ptr, NX_TCP_HEADER *tcp_header_ptr);
VOID  _nx_tcp_socket_state_closing(NX_TCP_SOCKET *socket_ptr, NX_TCP_HEADER *tcp_header_ptr);
UINT  _nx_tcp_socket_state_data_check(NX_TCP_SOCKET *socket_ptr, NX_PACKET *packet_ptr);
VOID  _nx_tcp_socket_state_established(NX_TCP_SOCKET *socket_ptr, NX_TCP_HEADER *tcp_header_ptr);
VOID  _nx_tcp_socket_state_fin_wait1(NX_TCP_SOCKET *socket_ptr, NX_TCP_HEADER *tcp_header_ptr);
VOID  _nx_tcp_socket_state_fin_wait2(NX_TCP_SOCKET *socket_ptr, NX_TCP_HEADER *tcp_header_ptr);
VOID  _nx_tcp_socket_state_last_ack(NX_TCP_SOCKET *socket_ptr, NX_TCP_HEADER *tcp_header_ptr);
VOID  _nx_tcp_socket_state_syn_sent(NX_TCP_SOCKET *socket_ptr, NX_TCP_HEADER *tcp_header_ptr);
VOID  _nx_tcp_socket_state_syn_received(NX_TCP_SOCKET *socket_ptr, NX_TCP_HEADER *tcp_header_ptr);
VOID  _nx_tcp_socket_state_transmit_check(NX_TCP_SOCKET *socket_ptr);
VOID  _nx_tcp_socket_thread_resume(TX_THREAD **suspension_list_head, UINT status);
VOID  _nx_tcp_socket_thread_suspend(TX_THREAD **suspension_list_head, VOID (*suspend_cleanup)(TX_THREAD *), NX_TCP_SOCKET *socket_ptr, TX_MUTEX *mutex_ptr, ULONG wait_option);
VOID  _nx_tcp_socket_transmit_queue_flush(NX_TCP_SOCKET *socket_ptr);
VOID  _nx_tcp_transmit_cleanup(TX_THREAD *thread_ptr);


/* Define error checking shells for TCP API services.  These are only referenced by the 
   application.  */

UINT  _nxde_tcp_client_socket_connect(NX_TCP_SOCKET *socket_ptr, NXD_ADDRESS *server_ip, UINT server_port, ULONG wait_option);
UINT  _nxde_tcp_socket_peer_info_get(NX_TCP_SOCKET *socket_ptr, NXD_ADDRESS *peer_ip_address, ULONG *peer_port);
UINT  _nxe_tcp_client_socket_bind(NX_TCP_SOCKET *socket_ptr, UINT port, ULONG wait_option);
UINT  _nxe_tcp_client_socket_port_get(NX_TCP_SOCKET *socket_ptr, UINT *port_ptr);
UINT  _nxe_tcp_client_socket_unbind(NX_TCP_SOCKET *socket_ptr);
UINT  _nxe_tcp_enable(NX_IP *ip_ptr);
UINT  _nxe_tcp_free_port_find(NX_IP *ip_ptr, UINT port, UINT *free_port_ptr);
UINT  _nxe_tcp_info_get(NX_IP *ip_ptr, ULONG *tcp_packets_sent, ULONG *tcp_bytes_sent, 
                        ULONG *tcp_packets_received, ULONG *tcp_bytes_received,
                        ULONG *tcp_invalid_packets, ULONG *tcp_receive_packets_dropped,
                        ULONG *tcp_checksum_errors, ULONG *tcp_connections, 
                        ULONG *tcp_disconnections, ULONG *tcp_connections_dropped,
                        ULONG *tcp_retransmit_packets);
UINT  _nxe_tcp_server_socket_accept(NX_TCP_SOCKET *socket_ptr, ULONG wait_option);
UINT  _nxe_tcp_server_socket_listen(NX_IP *ip_ptr, UINT port, NX_TCP_SOCKET *socket_ptr, UINT listen_queue_size,
                                    VOID (*tcp_listen_callback)(NX_TCP_SOCKET *socket_ptr, UINT port));
UINT  _nxe_tcp_server_socket_relisten(NX_IP *ip_ptr, UINT port, NX_TCP_SOCKET *socket_ptr);
UINT  _nxe_tcp_server_socket_unaccept(NX_TCP_SOCKET *socket_ptr);
UINT  _nxe_tcp_server_socket_unlisten(NX_IP *ip_ptr, UINT port);
UINT  _nxe_tcp_socket_bytes_available(NX_TCP_SOCKET *socket_ptr, ULONG *bytes_available);
UINT  _nxe_tcp_socket_create(NX_IP *ip_ptr, NX_TCP_SOCKET *socket_ptr, const CHAR *name,
                             ULONG type_of_service, ULONG fragment, UINT time_to_live, ULONG window_size,
                             VOID (*tcp_urgent_data_callback)(NX_TCP_SOCKET *socket_ptr),
                             VOID (*tcp_disconnect_callback)(NX_TCP_SOCKET *socket_ptr),
                             UINT tcp_socket_size);
UINT  _nxe_tcp_socket_delete(NX_TCP_SOCKET *socket_ptr);
UINT  _nxe_tcp_socket_disconnect(NX_TCP_SOCKET *socket_ptr, ULONG wait_option);
UINT  _nxe_tcp_socket_info_get(NX_TCP_SOCKET *socket_ptr, ULONG *tcp_packets_sent, ULONG *tcp_bytes_sent, 
                               ULONG *tcp_packets_received, ULONG *tcp_bytes_received, 
                               ULONG *tcp_retransmit_packets, ULONG *tcp_packets_queued,
                               ULONG *tcp_checksum_errors, ULONG *tcp_socket_state,
                               ULONG *tcp_transmit_queue_depth, ULONG *tcp_transmit_window,
                               ULONG *tcp_receive_window);
UINT  _nxe_tcp_socket_mss_get(NX_TCP_SOCKET *socket_ptr, ULONG *mss);
UINT  _nxe_tcp_socket_mss_peer_get(NX_TCP_SOCKET *socket_ptr, ULONG *peer_mss);
UINT  _nxe_tcp_socket_mss_set(NX_TCP_SOCKET *socket_ptr, ULONG mss);
UINT  _nxe_tcp_socket_peer_info_get(NX_TCP_SOCKET *socket_ptr, ULONG *peer_ip_address, ULONG *peer_port);
UINT  _nxe_tcp_socket_receive(NX_TCP_SOCKET *socket_ptr, NX_PACKET **packet_ptr, ULONG wait_option);
UINT  _nxe_tcp_socket_receive_notify(NX_TCP_SOCKET *socket_ptr, 
                                     VOID (*tcp_receive_notify)(NX_TCP_SOCKET *socket_ptr));
UINT  _nxe_tcp_socket_send(NX_TCP_SOCKET *socket_ptr, NX_PACKET **packet_ptr_ptr, ULONG wait_option);
UINT  _nxe_tcp_socket_state_wait(NX_TCP_SOCKET *socket_ptr, UINT desired_state, ULONG wait_option);
UINT  _nxe_tcp_socket_transmit_configure(NX_TCP_SOCKET *socket_ptr, ULONG max_queue_depth, ULONG timeout, 
                                         ULONG max_retries, ULONG timeout_shift);

/* TCP component data declarations follow.  */

/* Determine if the initialization function of this component is including
   this file.  If so, make the data definitions really happen.  Otherwise,
   make them extern so other functions in the component can access them.  */

#ifdef NX_TCP_INIT
#define TCP_DECLARE 
#else
#define TCP_DECLARE extern
#endif

/* Define global data for the TCP component.  */

/* Define the actual number of ticks for the fast periodic timer.  */

TCP_DECLARE ULONG           _nx_tcp_fast_timer_rate;

/* Define the actual number of ticks for the delayed ACK timer.  */

TCP_DECLARE ULONG           _nx_tcp_ack_timer_rate;

/* Define the actual number of ticks for the retransmit timer.  */

TCP_DECLARE ULONG           _nx_tcp_transmit_timer_rate;


#endif
