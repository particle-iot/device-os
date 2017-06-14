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
/**   Packet Pool Management (Packet)                                     */ 
/**                                                                       */ 
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  COMPONENT DEFINITION                                   RELEASE        */ 
/*                                                                        */ 
/*    nx_packet.h                                         PORTABLE C      */ 
/*                                                           5.7          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    William E. Lamie, Express Logic, Inc.                               */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the NetX packet memory management component,      */ 
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
/*  12-30-2007     Yuxin Zhou               Modified comment(s),          */ 
/*                                            resulting in version 5.2    */ 
/*  08-03-2009     William E. Lamie         Modified comment(s),          */
/*                                            resulting in version 5.3    */
/*  11-23-2009     Yuxin Zhou               Modified comment(s),          */
/*                                            resulting in version 5.4    */
/*  06-01-2010     Yuxin Zhou               Removed internal debug logic, */
/*                                            resulting in version 5.5    */
/*  10-10-2011     Yuxin Zhou               Modified comment(s), added    */
/*                                            function prototypes,        */ 
/*                                            resulting in version 5.6    */
/*  01-31-2013     Yuxin Zhou               Modified comment(s),          */
/*                                            resulting in version 5.7    */
/*                                                                        */ 
/**************************************************************************/ 

#ifndef NX_PAC_H
#define NX_PAC_H


#define NX_PACKET_POOL_ID   ((ULONG) 0x5041434B)


/* Define constants for packet free, allocated, enqueued, and driver transmit done.  
   These will be used in the nx_packet_tcp_queue_next field to indicate the state of 
   the packet.  */

#define NX_PACKET_FREE      ((ULONG) 0xFFFFFFFF)    /* Packet is available and in the pool  */
#define NX_PACKET_ALLOCATED ((ULONG) 0xAAAAAAAA)    /* Packet has been allocated            */ 
#define NX_PACKET_ENQUEUED  ((ULONG) 0xEEEEEEEE)    /* Packet is the tail of TCP queue.     */ 
                                                    /* A value that is none of the above    */ 
                                                    /*   also indicates the packet is in a  */ 
                                                    /*   TCP queue                          */ 

/* Define the constant for driver done and receive packet is available. These will be
   used in the nx_packet_queue_next field to indicate the state of a TCP packet.  */

#define NX_DRIVER_TX_DONE   ((ULONG) 0xDDDDDDDD)    /* Driver has sent the TCP packet       */ 
#define NX_PACKET_READY     ((ULONG) 0xBBBBBBBB)    /* Packet is ready for retrieval        */ 


/* Define packet pool management function prototypes.  */

UINT  _nx_packet_allocate(NX_PACKET_POOL *pool_ptr,  NX_PACKET **packet_ptr, 
                            ULONG packet_type, ULONG wait_option);
UINT  _nx_packet_copy(NX_PACKET *packet_ptr, NX_PACKET **new_packet_ptr, 
                            NX_PACKET_POOL *pool_ptr, ULONG wait_option);
UINT  _nx_packet_data_append(NX_PACKET *packet_ptr, const VOID *data_start, ULONG data_size,
                            NX_PACKET_POOL *pool_ptr, ULONG wait_option);
UINT  _nx_packet_data_extract_offset(NX_PACKET *packet_ptr, ULONG offset, VOID *buffer_start, ULONG buffer_length, ULONG *bytes_copied);
UINT  _nx_packet_data_retrieve(NX_PACKET *packet_ptr, VOID *buffer_start, ULONG *bytes_copied);
UINT  _nx_packet_length_get(NX_PACKET *packet_ptr, ULONG *length);
UINT  _nx_packet_pool_create(NX_PACKET_POOL *pool_ptr, const CHAR *name, ULONG payload_size,
                            VOID *memory_ptr, ULONG memory_size);
UINT  _nx_packet_pool_delete(NX_PACKET_POOL *pool_ptr);
UINT  _nx_packet_pool_info_get(NX_PACKET_POOL *pool_ptr, ULONG *total_packets, ULONG *free_packets, 
                            ULONG *empty_pool_requests, ULONG *empty_pool_suspensions,
                            ULONG *invalid_packet_releases);
UINT  _nx_packet_release(NX_PACKET *packet_ptr);
UINT  _nx_packet_transmit_release(NX_PACKET *packet_ptr);
VOID  _nx_packet_pool_cleanup(TX_THREAD *thread_ptr);
VOID  _nx_packet_pool_initialize(VOID);


/* Define error checking shells for API services.  These are only referenced by the 
   application.  */

UINT  _nxe_packet_allocate(NX_PACKET_POOL *pool_ptr,  NX_PACKET **packet_ptr, 
                            ULONG packet_type, ULONG wait_option);
UINT  _nxe_packet_copy(NX_PACKET *packet_ptr, NX_PACKET **new_packet_ptr, 
                            NX_PACKET_POOL *pool_ptr, ULONG wait_option);
UINT  _nxe_packet_data_append(NX_PACKET *packet_ptr, const VOID *data_start, ULONG data_size,
                            NX_PACKET_POOL *pool_ptr, ULONG wait_option);
UINT  _nxe_packet_data_extract_offset(NX_PACKET *packet_ptr, ULONG offset, VOID *buffer_start, ULONG buffer_length, ULONG *bytes_copied);
UINT  _nxe_packet_data_retrieve(NX_PACKET *packet_ptr, VOID *buffer_start, ULONG *bytes_copied);
UINT  _nxe_packet_length_get(NX_PACKET *packet_ptr, ULONG *length);
UINT  _nxe_packet_pool_create(NX_PACKET_POOL *pool_ptr, const CHAR *name, ULONG payload_size,
                            VOID *memory_ptr, ULONG memory_size, UINT pool_control_block_size);
UINT  _nxe_packet_pool_delete(NX_PACKET_POOL *pool_ptr);
UINT  _nxe_packet_pool_info_get(NX_PACKET_POOL *pool_ptr, ULONG *total_packets, ULONG *free_packets, 
                            ULONG *empty_pool_requests, ULONG *empty_pool_suspensions,
                            ULONG *invalid_packet_releases);
UINT  _nxe_packet_release(NX_PACKET **packet_ptr_ptr);
UINT  _nxe_packet_transmit_release(NX_PACKET **packet_ptr_ptr);


/* Packet pool management component data declarations follow.  */

/* Determine if the initialization function of this component is including
   this file.  If so, make the data definitions really happen.  Otherwise,
   make them extern so other functions in the component can access them.  */

#ifdef NX_PACKET_POOL_INIT
#define PACKET_POOL_DECLARE 
#else
#define PACKET_POOL_DECLARE extern
#endif


/* Define the head pointer of the created packet pool list.  */

PACKET_POOL_DECLARE  NX_PACKET_POOL     *_nx_packet_pool_created_ptr;


/* Define the variable that holds the number of created packet pools.  */

PACKET_POOL_DECLARE  ULONG              _nx_packet_pool_created_count;


#endif
