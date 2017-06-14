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
/*  12-30-2007     Yuxin Zhou               Modified comment(s), and      */ 
/*                                            added support for IPv6,     */
/*                                            resulting in version 5.2    */ 
/*  08-03-2009     William E. Lamie         Modified comment(s),          */
/*                                            resulting in version 5.3    */
/*  11-23-2009     Yuxin Zhou               Modified comment(s),          */
/*                                            resulting in version 5.4    */
/*  06-01-2010     Yuxin Zhou               Removed internal debug logic, */
/*                                            resulting in version 5.5    */
/*  10-10-2011     Yuxin Zhou               Modified comment(s),          */
/*                                            resulting in version 5.6    */
/*  01-31-2013     Yuxin Zhou               Modified comment(s),          */
/*                                            resulting in version 5.7    */
/*                                                                        */ 
/**************************************************************************/ 

#ifndef NX_ICMPV4_H
#define NX_ICMPV4_H


/* Define ICMP types and codes.  */

#define NX_ICMP_ECHO_REPLY_TYPE         0
#define NX_ICMP_DEST_UNREACHABLE_TYPE   3
#define NX_ICMP_SOURCE_QUENCH_TYPE      4
#define NX_ICMP_REDIRECT_TYPE           5
#define NX_ICMP_ECHO_REQUEST_TYPE       8
#define NX_ICMP_TIME_EXCEEDED_TYPE      11
#define NX_ICMP_PARAMETER_PROB_TYPE     12
#define NX_ICMP_TIMESTAMP_REQ_TYPE      13
#define NX_ICMP_TIMESTAMP_REP_TYPE      14
#define NX_ICMP_ADDRESS_MASK_REQ_TYPE   17
#define NX_ICMP_ADDRESS_MASK_REP_TYPE   18

#define NX_ICMP_NETWORK_UNREACH_CODE    0
#define NX_ICMP_HOST_UNREACH_CODE       1
#define NX_ICMP_PROTOCOL_UNREACH_CODE   2
#define NX_ICMP_PORT_UNREACH_CODE       3
#define NX_ICMP_FRAMENT_NEEDED_CODE     4
#define NX_ICMP_SOURCE_ROUTE_CODE       5
#define NX_ICMP_NETWORK_UNKNOWN_CODE    6
#define NX_ICMP_HOST_UNKNOWN_CODE       7
#define NX_ICMP_SOURCE_ISOLATED_CODE    8
#define NX_ICMP_NETWORK_PROHIBIT_CODE   9
#define NX_ICMP_HOST_PROHIBIT_CODE      10
#define NX_ICMP_NETWORK_SERVICE_CODE    11
#define NX_ICMP_HOST_SERVICE_CODE       12



/* Define Basic ICMP packet header data type.  This will be used to
   build new ICMP packets and to examine incoming packets into NetX.  */

typedef  struct NX_ICMP_HEADER_STRUCT
{
    /* Define the first 32-bit word of the ICMP header.  This word contains 
       the following information:  

            bits 31-24  ICMP 8-bit type defined as follows:

                                        Type Field      ICMP Message Type

                                            0           Echo Reply
                                            3           Destination Unreachable
                                            4           Source Quench
                                            5           Redirect (change a route)
                                            8           Echo Request
                                            11          Time exceeded for Datagram
                                            12          Parameter Problem on a Datagram
                                            13          Timestamp Request
                                            14          Timestamp Reply
                                            17          Address Mask Request
                                            18          Address Mask Reply

            bits 23-16  ICMP 8-bit code defined as follows:

                                        Code Field      ICMP Code Meaning

                                            0           Network unreachable
                                            1           Host unreachable
                                            2           Protocol unreachable
                                            3           Port unreachable
                                            4           Fragmentation needed and DF is set
                                            5           Source route failed
                                            6           Destination network unknown
                                            7           Destination host unknown
                                            8           Source host isolated
                                            9           Communication with destination network
                                                            administratively prohibited
                                            10          Communication with destination host
                                                            administratively prohibited
                                            11          Network unreachable for type of service
                                            12          Host unreachable for type of service

            bits 15-0   ICMP 16-bit checksum

     */

    ULONG       nx_icmp_header_word_0;

    /* Define the second and final word of the ICMP header.  This word contains
       the following information:

            bits 31-16  ICMP 16-bit Identification
            bits 15-0   ICMP 16-bit Sequence Number
     */
    ULONG       nx_icmp_header_word_1;

} NX_ICMP_HEADER;


/* Define the ICMP echo request header message size.  */

#define NX_ICMP_HEADER_SIZE             sizeof(NX_ICMP_HEADER)



/* Define ICMP function prototypes.  */

UINT  _nx_icmp_enable(NX_IP *ip_ptr);
UINT  _nx_icmp_info_get(NX_IP *ip_ptr, ULONG *pings_sent, ULONG *ping_timeouts, 
                            ULONG *ping_threads_suspended, ULONG *ping_responses_received,
                            ULONG *icmp_checksum_errors, ULONG *icmp_unhandled_messages);
UINT  _nx_icmp_ping(NX_IP *ip_ptr, ULONG ip_address, CHAR *data, ULONG data_size,
                            NX_PACKET **response_ptr, ULONG wait_option);
VOID  _nx_icmp_initialize(VOID);
VOID  _nx_icmp_cleanup(TX_THREAD *thread_ptr);
VOID  _nx_icmpv4_packet_process(NX_IP *ip_ptr, NX_PACKET *packet_ptr);


/* Define error checking shells for API services.  These are only referenced by the 
   application.  */

UINT  _nxe_icmp_enable(NX_IP *ip_ptr);
UINT  _nxe_icmp_info_get(NX_IP *ip_ptr, ULONG *pings_sent, ULONG *ping_timeouts, 
                            ULONG *ping_threads_suspended, ULONG *ping_responses_received,
                            ULONG *icmp_checksum_errors, ULONG *icmp_unhandled_messages);
UINT  _nxe_icmp_ping(NX_IP *ip_ptr, ULONG ip_address, CHAR *data, ULONG data_size,
                            NX_PACKET **response_ptr, ULONG wait_option);


#endif
