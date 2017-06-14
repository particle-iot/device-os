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
/*    nx_mld.h                                           PORTABLE C       */ 
/*                                                           5.7          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    Yuxin Zhou,  Express Logic, Inc.                                    */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the IPv6 multicast services.                      */
/*                                                                        */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  01-31-2013     Yuxin Zhou               Initial Version 5.7           */
/*                                                                        */ 
/**************************************************************************/ 

#ifndef NX_MLD_H
#define NX_MLD_H

#include "nx_api.h"
#include "nx_ipv6.h"


#define NXD_MLD_MULTICAST_LISTENER_QUERY_TYPE  0x82000000
#define NXD_MLD_MULTICAST_LISTENER_REPORT      0x83000000
#define NXD_MLD_MULTICAST_LISTENER_DONE        0x84000000
#define NXD_MLD_TYPE_MASK                      0xFF000000
#define NXD_MLD_CHECHSUM_MASK                  0x0000FFFF
#define NXD_MLD_MAX_RESP_TIME_MASK             0xFFFF0000
#define NXD_MLD_MAX_UPDATE_TIME_MS             1000
#define NXD_MLD_HOP_LIMIT                      1
#define NXD_MLD_HEADER_SIZE                    sizeof(NXD_MLD_HEADER)

typedef  struct NXD_MLD_HEADER_STRUCT
{
    /* Define the first 32-bit word of the MLD header.  This word contains
       the following information:

            bits 31-24  TYPE defined as follows:
                        130           Multicast listener query
                        131           Multicast listener report
                        132           Multicast listener done

            bits 23-16  Code, set to 0
            bits 15-0   Checksum
     */

    ULONG       nxd_mld_header_word_0;

    /* Define the second word of the MLD header.  This word contains
       the following information:

            bits 31-16   Maximum response delay
            bits 15-0    reserved
     */
    ULONG       nxd_mld_header_word_1;
    ULONG       multicast_address[4];

} NXD_MLD_HEADER;

/* Define IGMP function prototypes.  */

UINT  _nxd_mld_enable(NX_IP *ip_ptr);
UINT  _nxd_mld_multicast_join(NX_IP *ip_ptr, ULONG* group_address);
UINT  _nxd_mld_multicast_leave(NX_IP *ip_ptr, ULONG* group_address);
UINT  _nxd_mld_multicast_interface_join(NX_IP *ip_ptr, ULONG* group_address, UINT interface_index);
UINT  _nxd_mld_multicast_interface_leave(NX_IP *ip_ptr, ULONG* group_address, UINT interface_index);
VOID  _nxd_mld_periodic_processing(NX_IP *ip_ptr);
VOID  _nxd_mld_packet_process(NX_IP *ip_ptr, NX_PACKET *packet_ptr);

#ifdef NX_IPV6_MULTICAST_ENABLE
UINT  _nxd_ipv6_multicast_interface_join(NX_IP *ip_ptr, NXD_ADDRESS *group_address, UINT interface_index);
UINT  _nxd_ipv6_multicast_interface_leave(NX_IP *ip_ptr, NXD_ADDRESS *group_address, UINT interface_index);
#endif /* NX_IPV6_MULTICAST_ENABLE  */

#endif
