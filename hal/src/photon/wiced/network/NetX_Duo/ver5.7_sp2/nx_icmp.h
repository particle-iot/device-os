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
/*    nx_icmp.h                                           PORTABLE C      */ 
/*                                                           5.7          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    William E. Lamie, Express Logic, Inc.                               */ 
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
/*  12-12-2005     William E. Lamie         Initial Version 5.0           */ 
/*  08-09-2007     William E. Lamie         Modified comment(s),          */ 
/*                                            resulting in version 5.1    */ 
/*  12-30-2007     Yuxin Zhou               Modified comment(s),          */ 
/*                                            resulting in version 5.2    */
/*  08-03-2009     William E. Lamie         Modified comment(s),          */
/*                                            resulting in version 5.3    */ 
/*  11-23-2009     Yuxin Zhou               Modified comment(s),          */
/*                                            resulting in version 5.4    */
/*  06-01-2010     Yuxin Zhou               Modified comment(s),          */
/*                                            resulting in version 5.5    */
/*  10-10-2011     Yuxin Zhou               Modified comment(s), added    */
/*                                            an API to handle ping       */ 
/*                                            in a multihome systems,     */
/*                                            resulting in version 5.6    */
/*  01-31-2013     Yuxin Zhou               Modified comment(s),          */
/*                                            resulting in version 5.7    */
/*                                                                        */ 
/**************************************************************************/ 

#ifndef NX_ICMP_H
#define NX_ICMP_H




/* Define the ICMP history entry for the optional ICMP debug log.  */

typedef struct NX_ICMP_EVENT_STRUCT
{

    /* Define the unique sequence number for this event.  */
    ULONG       nx_icmp_event_sequence_number;

    /* Define the time stamp for the event.  */
    ULONG       nx_icmp_event_timestamp;

    /* Define the type of event.  */
    UCHAR       nx_icmp_event_type;

    /* Define the event context.  */
    TX_THREAD   *nx_icmp_event_context;

    /* Define the packet pointer.  */
    NX_PACKET   *nx_icmp_event_packet_ptr;

    /* Define the packet size.  */
    UINT        nx_icmp_event_packet_size;

    /* Define the IP address for the destination on a send or
       the sender on a receive ICMP message.  */
    ULONG       nx_icmp_event_ip_address;

    /* Define the ICMP header words for the packet.  */
    ULONG       nx_icmp_event_header_word_0;
    ULONG       nx_icmp_event_header_word_1;
    ULONG       nx_icmp_event_header_word_2;
    ULONG       nx_icmp_event_header_word_3;
    ULONG       nx_icmp_event_header_word_4;

    /* Define the ICMP instance associated with the event.  */
    NX_IP       *nx_icmp_event_ip_ptr;

} NX_ICMP_EVENT;

#include "nx_icmpv4.h"

VOID  _nx_icmp_packet_receive(NX_IP *ip_ptr, NX_PACKET *packet_ptr);
VOID  _nx_icmp_queue_process(NX_IP *ip_ptr);
VOID  _nx_icmp_packet_process(NX_IP *ip_ptr, NX_PACKET *packet_ptr);

UINT  _nxd_icmp_ping(NX_IP *ip_ptr, NXD_ADDRESS *ip_address, 
                     CHAR *data_ptr, ULONG data_size,
                     NX_PACKET **response_ptr, ULONG wait_option);
UINT  _nxd_icmp_source_ping(NX_IP *ip_ptr, NXD_ADDRESS *ip_address, 
                            UINT address_index, CHAR *data_ptr, ULONG data_size,
                            NX_PACKET **response_ptr, ULONG wait_option);

UINT  _nxd_icmp_enable(NX_IP *ip_ptr);

#ifdef FEATURE_NX_IPV6 
#include "nx_icmpv6.h"
#endif /* FEATURE_NX_IPV6 */

#endif
