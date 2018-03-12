/**************************************************************************/ 
/*                                                                        */ 
/*            Copyright (c) 1996-2012 by Express Logic Inc.               */ 
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
/** ThreadX Component                                                     */
/**                                                                       */
/**   Event Flags                                                         */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  COMPONENT DEFINITION                                   RELEASE        */ 
/*                                                                        */ 
/*    tx_event_flags.h                                    PORTABLE C      */ 
/*                                                           5.6          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    William E. Lamie, Express Logic, Inc.                               */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the ThreadX event flags management component,     */ 
/*    including all data types and external references.  It is assumed    */ 
/*    that tx_api.h and tx_port.h have already been included.             */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  12-12-2005     William E. Lamie         Initial Version 5.0           */ 
/*  04-02-2007     William E. Lamie         Modified comment(s), and      */ 
/*                                            replaced UL constant        */ 
/*                                            modifier with ULONG cast,   */ 
/*                                            resulting in version 5.1    */ 
/*  12-12-2008     William E. Lamie         Modified comment(s),          */ 
/*                                            resulting in version 5.2    */ 
/*  07-04-2009     William E. Lamie         Modified comment(s),          */ 
/*                                            resulting in version 5.3    */ 
/*  12-12-2009     William E. Lamie         Modified comment(s),          */ 
/*                                            resulting in version 5.4    */ 
/*  07-15-2011     William E. Lamie         Modified comment(s),          */ 
/*                                            resulting in version 5.5    */ 
/*  11-01-2012     William E. Lamie         Modified comment(s),          */ 
/*                                            resulting in version 5.6    */ 
/*                                                                        */ 
/**************************************************************************/ 

#ifndef TX_EVENT_FLAGS_H
#define TX_EVENT_FLAGS_H


/* Define event flags control specific data definitions.  */

#define TX_EVENT_FLAGS_ID                       ((ULONG) 0x4456444E)
#define TX_EVENT_FLAGS_AND_MASK                 0x2
#define TX_EVENT_FLAGS_CLEAR_MASK               0x1


/* Determine if in-line component initialization is supported by the 
   caller.  */
#ifdef TX_INVOKE_INLINE_INITIALIZATION

/* Yes, in-line initialization is supported, remap the event flag initialization 
   function.  */

#ifndef TX_EVENT_FLAGS_ENABLE_PERFORMANCE_INFO
#define _tx_event_flags_initialize() \
                    _tx_event_flags_created_ptr =                   TX_NULL; \
                    _tx_event_flags_created_count =                 0;
#else
#define _tx_event_flags_initialize() \
                    _tx_event_flags_created_ptr =                   TX_NULL; \
                    _tx_event_flags_created_count =                 0; \
                    _tx_event_flags_performance_set_count =         0; \
                    _tx_event_flags_performance_get_count =         0; \
                    _tx_event_flags_performance_suspension_count =  0; \
                    _tx_event_flags_performance_timeout_count =     0;
#endif
#define TX_EVENT_FLAGS_INIT
#else

/* No in-line initialization is supported, use standard function call.  */
VOID        _tx_event_flags_initialize(VOID);
#endif


/* Define event flags management function prototypes.  */

VOID        _tx_event_flags_cleanup(TX_THREAD *thread_ptr);
UINT        _tx_event_flags_create(TX_EVENT_FLAGS_GROUP *group_ptr, const CHAR *name_ptr);
UINT        _tx_event_flags_delete(TX_EVENT_FLAGS_GROUP *group_ptr);
UINT        _tx_event_flags_get(TX_EVENT_FLAGS_GROUP *group_ptr, ULONG requested_flags,
                    UINT get_option, ULONG *actual_flags_ptr, ULONG wait_option);
UINT        _tx_event_flags_info_get(TX_EVENT_FLAGS_GROUP *group_ptr, const CHAR **name, ULONG *current_flags,
                    TX_THREAD **first_suspended, ULONG *suspended_count, 
                    TX_EVENT_FLAGS_GROUP **next_group);
UINT        _tx_event_flags_performance_info_get(TX_EVENT_FLAGS_GROUP *group_ptr, ULONG *sets, ULONG *gets,
                    ULONG *suspensions, ULONG *timeouts);
UINT        _tx_event_flags_performance_system_info_get(ULONG *sets, ULONG *gets,
                    ULONG *suspensions, ULONG *timeouts);
UINT        _tx_event_flags_set(TX_EVENT_FLAGS_GROUP *group_ptr, ULONG flags_to_set, 
                    UINT set_option);
UINT        _tx_event_flags_set_notify(TX_EVENT_FLAGS_GROUP *group_ptr, VOID (*events_set_notify)(TX_EVENT_FLAGS_GROUP *));


/* Define error checking shells for API services.  These are only referenced by the 
   application.  */

UINT        _txe_event_flags_create(TX_EVENT_FLAGS_GROUP *group_ptr, const CHAR *name_ptr, UINT event_control_block_size);
UINT        _txe_event_flags_delete(TX_EVENT_FLAGS_GROUP *group_ptr);
UINT        _txe_event_flags_get(TX_EVENT_FLAGS_GROUP *group_ptr, ULONG requested_flags,
                    UINT get_option, ULONG *actual_flags_ptr, ULONG wait_option);
UINT        _txe_event_flags_info_get(TX_EVENT_FLAGS_GROUP *group_ptr, const CHAR **name, ULONG *current_flags,
                    TX_THREAD **first_suspended, ULONG *suspended_count, 
                    TX_EVENT_FLAGS_GROUP **next_group);
UINT        _txe_event_flags_set(TX_EVENT_FLAGS_GROUP *group_ptr, ULONG flags_to_set, 
                    UINT set_option);
UINT        _txe_event_flags_set_notify(TX_EVENT_FLAGS_GROUP *group_ptr, VOID (*events_set_notify)(TX_EVENT_FLAGS_GROUP *));


/* Event flags management component data declarations follow.  */

/* Determine if the initialization function of this component is including
   this file.  If so, make the data definitions really happen.  Otherwise,
   make them extern so other functions in the component can access them.  */

#ifdef TX_EVENT_FLAGS_INIT
#define EVENT_FLAGS_DECLARE 
#else
#define EVENT_FLAGS_DECLARE extern
#endif


/* Define the head pointer of the created event flags list.  */

EVENT_FLAGS_DECLARE  TX_EVENT_FLAGS_GROUP * _tx_event_flags_created_ptr;


/* Define the variable that holds the number of created event flag groups. */

EVENT_FLAGS_DECLARE  ULONG                  _tx_event_flags_created_count;


#ifdef TX_EVENT_FLAGS_ENABLE_PERFORMANCE_INFO

/* Define the total number of event flag sets.  */

EVENT_FLAGS_DECLARE  ULONG                  _tx_event_flags_performance_set_count;


/* Define the total number of event flag gets.  */

EVENT_FLAGS_DECLARE  ULONG                  _tx_event_flags_performance_get_count;


/* Define the total number of event flag suspensions.  */

EVENT_FLAGS_DECLARE  ULONG                  _tx_event_flags_performance_suspension_count;


/* Define the total number of event flag timeouts.  */

EVENT_FLAGS_DECLARE  ULONG                  _tx_event_flags_performance_timeout_count;


#endif

#endif

