/**************************************************************************/ 
/*                                                                        */ 
/*            Copyright (c) 1996-2011 by Express Logic Inc.               */ 
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
/**   AutoIP (AutoIP)                                                     */ 
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  APPLICATION INTERFACE DEFINITION                       RELEASE        */ 
/*                                                                        */ 
/*    nx_auto_ip.h                                        PORTABLE C      */ 
/*                                                           5.2          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    William E. Lamie, Express Logic, Inc.                               */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the NetX AutoIP Protocol (AutoIP) component,      */ 
/*    including all data types and external references. It is assumed     */ 
/*    that nx_api.h and nx_port.h have already been included.             */
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  03-01-2006     William E. Lamie         Initial Version 5.0           */ 
/*  04-01-2010    Janet Christiansen        Added multi-home support,     */ 
/*                                            modified comments, resulting*/
/*                                            in version 5.1              */
/*  07-15-2011     Janet Christiansen       Modified comment(s),          */
/*                                            resulting in version 5.2    */
/*                                                                        */ 
/**************************************************************************/ 

#ifndef  NX_AUTO_IP_H
#define  NX_AUTO_IP_H

/* Determine if a C++ compiler is being used.  If so, ensure that standard
   C is used to process the API information.  */

#ifdef   __cplusplus

/* Yes, C++ compiler is present.  Use standard C.  */
extern   "C" {

#endif


/* Define the AutoIP ID that is used to mark the AutoIP structure as created.  */

#define NX_AUTO_IP_ID                       0x4155544FUL


/* Define the timing and retry constants for AutoIP.  */

#ifndef NX_AUTO_IP_PROBE_WAIT
#define NX_AUTO_IP_PROBE_WAIT               1
#endif

#ifndef NX_AUTO_IP_PROBE_NUM
#define NX_AUTO_IP_PROBE_NUM                3
#endif

#ifndef NX_AUTO_IP_PROBE_MIN
#define NX_AUTO_IP_PROBE_MIN                1
#endif

#ifndef NX_AUTO_IP_PROBE_MAX
#define NX_AUTO_IP_PROBE_MAX                2
#endif

#ifndef NX_AUTO_IP_MAX_CONFLICTS
#define NX_AUTO_IP_MAX_CONFLICTS            100
#endif

#ifndef NX_AUTO_IP_RATE_LIMIT_INTERVAL
#define NX_AUTO_IP_RATE_LIMIT_INTERVAL      60
#endif

#ifndef NX_AUTO_IP_ANNOUNCE_WAIT
#define NX_AUTO_IP_ANNOUNCE_WAIT            2
#endif

#ifndef NX_AUTO_IP_ANNOUNCE_NUM
#define NX_AUTO_IP_ANNOUNCE_NUM             1
#endif

#ifndef NX_AUTO_IP_ANNOUNCE_INTERVAL
#define NX_AUTO_IP_ANNOUNCE_INTERVAL        2
#endif

#ifndef NX_AUTO_IP_DEFEND_INTERVAL
#define NX_AUTO_IP_DEFEND_INTERVAL          10
#endif


/* Define the starting and ending local IP addresses.  */

#define NX_AUTO_IP_START_ADDRESS            IP_ADDRESS(169,254,1,0)
#define NX_AUTO_IP_END_ADDRESS              IP_ADDRESS(169,254,245,255)


/* Define error codes from AutoIP API.  */

#define NX_AUTO_IP_ERROR                    0xA00
#define NX_AUTO_IP_NO_LOCAL                 0xA01
#define NX_AUTO_IP_BAD_INTERFACE_INDEX      0xA02


/* Define the AutoIP structure that holds all the information necessary for this AutoIP 
   instance.  */

typedef struct NX_AUTO_IP_STRUCT 
{
    ULONG                   nx_auto_ip_id;
    const CHAR              *nx_auto_ip_name;
    NX_IP                   *nx_auto_ip_ip_ptr;
    UINT                    nx_ip_interface_index;  
    UINT                    nx_auto_ip_local_address_valid;
    ULONG                   nx_auto_ip_current_local_address;
    ULONG                   nx_auto_ip_restart_flag;
    ULONG                   nx_auto_ip_error_count;
    ULONG                   nx_auto_ip_conflict_count;
    ULONG                   nx_auto_ip_probe_count;
    ULONG                   nx_auto_ip_announce_count;
    ULONG                   nx_auto_ip_defend_count;
    ULONG                   nx_auto_ip_interval;
    TX_EVENT_FLAGS_GROUP    nx_auto_ip_conflict_event;
    TX_THREAD               nx_auto_ip_thread;
} NX_AUTO_IP;


#ifndef NX_AUTO_IP_SOURCE_CODE

/* Application caller is present, perform API mapping.  */

/* Determine if error checking is desired.  If so, map AutoIP API functions 
   to the appropriate error checking front-ends.  Otherwise, map API
   functions to the core functions that actually perform the work. 
   Note: error checking is enabled by default.  */

#ifdef NX_DISABLE_ERROR_CHECKING

/* Services without error checking.  */

#define nx_auto_ip_create               _nx_auto_ip_create
#define nx_auto_ip_get_address          _nx_auto_ip_get_address
#define nx_auto_ip_set_interface        _nx_auto_ip_set_interface      
#define nx_auto_ip_start                _nx_auto_ip_start
#define nx_auto_ip_stop                 _nx_auto_ip_stop
#define nx_auto_ip_delete               _nx_auto_ip_delete

#else

/* Services with error checking.  */

#define nx_auto_ip_create               _nxe_auto_ip_create
#define nx_auto_ip_get_address          _nxe_auto_ip_get_address
#define nx_auto_ip_set_interface        _nxe_auto_ip_set_interface    
#define nx_auto_ip_start                _nxe_auto_ip_start
#define nx_auto_ip_stop                 _nxe_auto_ip_stop
#define nx_auto_ip_delete               _nxe_auto_ip_delete

#endif

/* Define the prototypes accessible to the application software.  */

UINT        nx_auto_ip_create(NX_AUTO_IP *auto_ip_ptr, const CHAR *name, NX_IP *ip_ptr, VOID *stack_ptr, ULONG stack_size, UINT priority);
UINT        nx_auto_ip_get_address(NX_AUTO_IP *auto_ip_ptr, ULONG *local_ip_address);
UINT        nx_auto_ip_set_interface(NX_AUTO_IP *auto_ip_ptr, UINT interface_index); 
UINT        nx_auto_ip_start(NX_AUTO_IP *auto_ip_ptr, ULONG starting_local_address);
UINT        nx_auto_ip_stop(NX_AUTO_IP *auto_ip_ptr);
UINT        nx_auto_ip_delete(NX_AUTO_IP *auto_ip_ptr);


#else

/* AutoIP source code is being compiled, do not perform any API mapping.  */

UINT        _nxe_auto_ip_create(NX_AUTO_IP *auto_ip_ptr, const CHAR *name, NX_IP *ip_ptr, VOID *stack_ptr, ULONG stack_size, UINT priority);
UINT        _nx_auto_ip_create(NX_AUTO_IP *auto_ip_ptr, const CHAR *name, NX_IP *ip_ptr, VOID *stack_ptr, ULONG stack_size, UINT priority);
UINT        _nxe_auto_ip_get_address(NX_AUTO_IP *auto_ip_ptr, ULONG *local_ip_address);
UINT        _nx_auto_ip_get_address(NX_AUTO_IP *auto_ip_ptr, ULONG *local_ip_address);
UINT        _nxe_auto_ip_set_interface(NX_AUTO_IP *auto_ip_ptr, UINT interface_index);
UINT        _nx_auto_ip_set_interface(NX_AUTO_IP *auto_ip_ptr, UINT interface_index);
UINT        _nxe_auto_ip_start(NX_AUTO_IP *auto_ip_ptr, ULONG starting_local_address);
UINT        _nx_auto_ip_start(NX_AUTO_IP *auto_ip_ptr, ULONG starting_local_address);
UINT        _nxe_auto_ip_stop(NX_AUTO_IP *auto_ip_ptr);
UINT        _nx_auto_ip_stop(NX_AUTO_IP *auto_ip_ptr);
UINT        _nxe_auto_ip_delete(NX_AUTO_IP *auto_ip_ptr);
UINT        _nx_auto_ip_delete(NX_AUTO_IP *auto_ip_ptr);
VOID        _nx_auto_ip_thread_entry(ULONG auto_ip_address);
VOID        _nx_auto_ip_conflict(VOID *parameter);
VOID        _nx_auto_ip_arp_packet_send(NX_AUTO_IP *auto_ip_ptr, ULONG source_ip, ULONG source_physical_msw, ULONG source_physical_lsw, 
                                                 ULONG target_ip, ULONG target_physical_msw, ULONG target_physical_lsw);

#endif


/* Determine if a C++ compiler is being used.  If so, complete the standard
   C conditional started above.  */
#ifdef   __cplusplus
        }
#endif

#endif

