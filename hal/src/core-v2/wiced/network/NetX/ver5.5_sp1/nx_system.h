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
/**   System Management (System)                                          */ 
/**                                                                       */ 
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  COMPONENT DEFINITION                                   RELEASE        */ 
/*                                                                        */ 
/*    nx_system.h                                         PORTABLE C      */ 
/*                                                           5.5          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    William E. Lamie, Express Logic, Inc.                               */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the NetX system management component,             */ 
/*    including all data types and external references.  It is assumed    */ 
/*    that nx_api.h and nx_port.h have already been included.             */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  12-12-2005     William E. Lamie         Initial Version 5.0           */ 
/*  08-09-2007     William E. Lamie         Modified comment(s),          */ 
/*                                            resulting in version 5.1    */ 
/*  07-04-2009     William E. Lamie         Modified comment(s),          */ 
/*                                            resulting in version 5.2    */ 
/*  12-31-2009     Yuxin Zhou               Modified comment(s),          */
/*                                            removed bit definitions for */ 
/*                                            the internal debug logic,   */ 
/*                                            resulting in version 5.3    */
/*  06-30-2011     Yuxin Zhou               Modified comment(s),          */
/*                                            resulting in version 5.4    */
/*  04-30-2013     Yuxin Zhou               Modified comment(s),          */
/*                                            resulting in version 5.5    */
/*                                                                        */ 
/**************************************************************************/ 

#ifndef NX_SYS_H
#define NX_SYS_H



/* Define system management function prototypes.  */

VOID  _nx_system_initialize(VOID);


/* Define error checking shells for API services.  These are only referenced by the 
   application.  */

/* System management component data declarations follow.  */

/* Determine if the initialization function of this component is including
   this file.  If so, make the data definitions really happen.  Otherwise,
   make them extern so other functions in the component can access them.  */

#ifdef NX_SYSTEM_INIT
#define SYSTEM_DECLARE 
#else
#define SYSTEM_DECLARE extern
#endif

/* Define the variable that holds the number of system ticks per second.  */

SYSTEM_DECLARE UINT  _nx_system_ticks_per_second;


/* Define the global NetX build options variables. These variables contain a bit 
   map representing how the NetX library was built. The following are the bit 
   field definitions:

    _nx_system_build_options_1:

                    Bit(s)                   Meaning

                    31                  NX_LITTLE_ENDIAN
                    30                  NX_ARP_DISABLE_AUTO_ARP_ENTRY
                    29                  NX_TCP_ENABLE_KEEPALIVE
                    28                  NX_TCP_IMMEDIATE_ACK
                    27                  NX_DRIVER_DEFERRED_PROCESSING
                    26                  NX_DISABLE_FRAGMENTATION
                    25                  NX_DISABLE_IP_RX_CHECKSUM
                    24                  NX_DISABLE_IP_TX_CHECKSUM
                    23                  NX_DISABLE_TCP_RX_CHECKSUM
                    22                  NX_DISABLE_TCP_TX_CHECKSUM
                    21                  NX_DISABLE_RESET_DISCONNECT
                    20                  NX_DISABLE_RX_SIZE_CHECKING
                    19                  NX_DISABLE_ARP_INFO
                    18                  NX_DISABLE_IP_INFO
                    17                  NX_DISABLE_ICMP_INFO
                    16                  NX_DISABLE_IGMP_INFO
                    15                  NX_DISABLE_PACKET_INFO
                    14                  NX_DISABLE_RARP_INFO
                    13                  NX_DISABLE_TCP_INFO
                    12                  NX_DISABLE_UDP_INFO
                    3-0                 NX_TCP_RETRY_SHIFT

    _nx_system_build_options_2:

                    Bit(s)                   Meaning

                    31-16               NX_IP_PERIODIC_RATE
                    15-8                NX_ARP_EXPIRATION_RATE
                    7-0                 NX_ARP_UPDATE_RATE

    _nx_system_build_options_3:

                    Bit(s)                   Meaning

                    31-24               NX_TCP_ACK_TIMER_RATE
                    23-16               NX_TCP_FAST_TIMER_RATE
                    15-8                NX_TCP_TRANSMIT_TIMER_RATE
                    7-0                 NX_TCP_KEEPALIVE_RETRY

    _nx_system_build_options_4:

                    Bit(s)                   Meaning

                    31-16               NX_TCP_KEEPALIVE_INITIAL               
                    15-8                NX_ARP_MAXIMUM_RETRIES
                    7-4                 NX_ARP_MAX_QUEUE_DEPTH
                    3-0                 NX_TCP_KEEPALIVE_RETRIES

    _nx_system_build_options_5:

                    Bit(s)                   Meaning

                    31-24               NX_MAX_MULTICAST_GROUPS
                    23-16               NX_MAX_LISTEN_REQUESTS
                    15-8                NX_TCP_MAXIMUM_RETRIES
                    7-0                 NX_TCP_MAXIMUM_TX_QUEUE

   Note that values greater than the value that can be represented in the build options
   bit field are represented as all ones in the bit field. For example, if NX_TCP_ACK_TIMER_RATE
   is 256, the value in the bits 31-24 of _nx_system_build_options_3 is 0xFF, which is 255 
   decimal.  */

SYSTEM_DECLARE  ULONG       _nx_system_build_options_1;
SYSTEM_DECLARE  ULONG       _nx_system_build_options_2;
SYSTEM_DECLARE  ULONG       _nx_system_build_options_3;
SYSTEM_DECLARE  ULONG       _nx_system_build_options_4;
SYSTEM_DECLARE  ULONG       _nx_system_build_options_5;


#endif
