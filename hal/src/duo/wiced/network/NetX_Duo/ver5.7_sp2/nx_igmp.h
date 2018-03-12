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
/**   Internet Group Management Protocol (IGMP)                           */ 
/**                                                                       */ 
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  COMPONENT DEFINITION                                   RELEASE        */ 
/*                                                                        */ 
/*    nx_igmp.h                                           PORTABLE C      */ 
/*                                                           5.7 (sp1)    */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    William E. Lamie, Express Logic, Inc.                               */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the NetX Internet Group Management Protocol (IGMP)*/ 
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
/*  07-25-2008     Janet Christiansen       Added definitions and         */
/*                                            configurable options for    */
/*                                            IGMPv2                      */
/*  08-03-2009     William E. Lamie         Modified comment(s),          */
/*                                            resulting in version 5.3    */
/*  11-23-2009     Yuxin Zhou               Modified comment(s),          */
/*                                            resulting in version 5.4    */
/*  06-01-2010     Yuxin Zhou               Removed internal debug logic, */
/*                                            resulting in version 5.5    */
/*  10-10-2011     Yuxin Zhou               Modified comment(s), added    */
/*                                            multihome support, removed  */
/*                                            unused function prototype,  */
/*                                            resulting in version 5.6    */
/*  01-31-2013     Yuxin Zhou               Modified comment(s),          */
/*                                            resulting in version 5.7    */
/*  xx-xx-xxxx     Yuxin Zhou               Modified comment(s), and      */ 
/*                                            added new functions for     */
/*                                            local multicast without     */
/*                                            sending out IGMP messages,  */ 
/*                                            resulting in version 5.7-sp1*/
/*                                                                        */ 
/**************************************************************************/ 

#ifndef NX_IGMP_H
#define NX_IGMP_H




/* Define IGMP types and codes.  */

/* IGMPv2 combines version field with the type field ffectively, and calls the 8 bit
   field the Type field.  IGMPv2 uses Type 0x11, 0x16, and 0x17 in this field, while
   IGMPv1 uses Type 1 and 2 in the lower 4 bits, combined with version 1 in the upper 4 bits
   for 0x11 and 0x12 as the possible values. */


/* Define the IGMP version in the IGMP header.  */

#define NX_IGMP_VERSION                 0x10000000


/* Define the numeric version of IGMP protocol used by NetX. */

#define NX_IGMP_HOST_VERSION_1          1
#define NX_IGMP_HOST_VERSION_2          2


/* Define the IGMP query message type and mask in the IGMP header.  */

#define NX_IGMP_ROUTER_QUERY_TYPE       0x01000000
#define NX_IGMP_TYPE_MASK               0x0F000000


/* Define the IGMPv1 type for membership join reports. */

#define NX_IGMP_HOST_RESPONSE_TYPE      0x02000000


/* Define IGMPv2 types for membership join and leave reports. */

#define NX_IGMP_HOST_V2_JOIN_TYPE       0x16000000
#define NX_IGMP_HOST_V2_LEAVE_TYPE      0x17000000


/* Define the IGMPv2 type mask in the IGMP header.  */

#define NX_IGMPV2_TYPE_MASK             0xFF000000


/* Define the IGMPv2 maximum response time mask in the IGMP header.  Max value is 0x64,
   left intentionally blank in IGMPv1 queries and all IGMP reports. Maximum response time
   is in tenth's of a second. */

#define NX_IGMP_MAX_RESP_TIME_MASK      0x00FF0000


/* For IGMPv1 only, define the IGMP maximum delay time to 10 seconds as per RFC 1112.  This is achieved if 
   the slow NetX IP periodic thread timer executes every second (e.g. NX_IP_PERIODIC_RATE is set to 100) 
   and the threadx timer interrupt occurs every 10 ms). */

#define NX_IGMP_MAX_UPDATE_TIME         10


/* Define the time to live for IGMP packets. RFC 1112 and 2236 specify
   time to live should be set at 1. */

#define NX_IGMP_TTL                     1


/* Define the mask for IGMPv1 packets' type field. */

#define NX_IGMP_TYPE_MASK               0x0F000000


/* Define the IP "all hosts" multicast address.  */

#define NX_ALL_HOSTS_ADDRESS            IP_ADDRESS(224, 0, 0, 1)


/* Define the IP "all routers" multicast address.  */

#define NX_ALL_ROUTERS_ADDRESS          IP_ADDRESS(224, 0, 0, 2)


/* Define Basic IGMP packet header data type.  This will be used to
   build new IGMP packets and to examine incoming IGMP queries sent
   to NetX.  */

typedef  struct NX_IGMP_HEADER_STRUCT
{
    /* Define the first 32-bit word of the IGMP header.  This word contains 
       the following information:  

            bits 31-28  IGMP 4-bit version (Version 1)

            bits 27-24  IGMP 4-bit type defined as follows:

                                        Type Field      IGMP Message Type

                                            1           Router Query Request
                                            2           Host Query Response

            bits 15-0   IGMP 16-bit checksum

     */

    ULONG       nx_igmp_header_word_0;

    /* Define the second and final word of the IGMP header.  This word contains
       the following information:

            bits 31-0   32-bit group address (class D IP address)
     */
    ULONG       nx_igmp_header_word_1;

} NX_IGMP_HEADER;


/* Define the IGMP query response message size.  */

#define NX_IGMP_HEADER_SIZE             sizeof(NX_IGMP_HEADER)



/* Define IGMP function prototypes.  */

UINT  _nx_igmp_enable(NX_IP *ip_ptr);
UINT  _nx_igmp_info_get(NX_IP *ip_ptr, ULONG *igmp_reports_sent, ULONG *igmp_queries_received, 
                            ULONG *igmp_checksum_errors, ULONG *current_groups_joined);
UINT  _nx_igmp_loopback_disable(NX_IP *ip_ptr);
UINT  _nx_igmp_loopback_enable(NX_IP *ip_ptr);
UINT  _nx_igmp_multicast_join(NX_IP *ip_ptr, ULONG group_address);
UINT  _nx_igmp_multicast_interface_join(NX_IP *ip_ptr, ULONG group_address, UINT interface_index);
UINT  _nx_igmp_multicast_leave(NX_IP *ip_ptr, ULONG group_address);
UINT  _nx_igmp_multicast_interface_join_internal(NX_IP *ip_ptr, ULONG group_address, UINT interface_index, UINT update_time);
UINT  _nx_igmp_multicast_leave_internal(NX_IP *ip_ptr, ULONG group_address);
VOID  _nx_igmp_periodic_processing(NX_IP *ip_ptr);
VOID  _nx_igmp_packet_process(NX_IP *ip_ptr, NX_PACKET *packet_ptr);
VOID  _nx_igmp_packet_receive(NX_IP *ip_ptr, NX_PACKET *packet_ptr);
VOID  _nx_igmp_queue_process(NX_IP *ip_ptr);
UINT  _nx_igmp_multicast_check(NX_IP *ip_ptr, ULONG group_address, NX_INTERFACE *nx_interface);


/* Define error checking shells for API services.  These are only referenced by the 
   application.  */

UINT  _nxe_igmp_enable(NX_IP *ip_ptr);
UINT  _nxe_igmp_info_get(NX_IP *ip_ptr, ULONG *igmp_reports_sent, ULONG *igmp_queries_received, 
                            ULONG *igmp_checksum_errors, ULONG *current_groups_joined);
UINT  _nxe_igmp_loopback_disable(NX_IP *ip_ptr);
UINT  _nxe_igmp_loopback_enable(NX_IP *ip_ptr);
UINT  _nxe_igmp_multicast_join(NX_IP *ip_ptr, ULONG group_address);
UINT  _nxe_igmp_multicast_interface_join(NX_IP *ip_ptr, ULONG group_address, UINT interface_index);
UINT  _nxe_igmp_multicast_leave(NX_IP *ip_ptr, ULONG group_address);
UINT  _nxe_igmp_multicast_interface_join_internal(NX_IP *ip_ptr, ULONG group_address, UINT interface_index, UINT update_time);
UINT  _nxe_igmp_multicast_leave_internal(NX_IP *ip_ptr, ULONG group_address);


#endif
