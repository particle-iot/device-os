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
/**   Semaphore                                                           */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  COMPONENT DEFINITION                                   RELEASE        */ 
/*                                                                        */ 
/*    tx_semaphore.h                                      PORTABLE C      */ 
/*                                                           5.6          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    William E. Lamie, Express Logic, Inc.                               */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the ThreadX semaphore management component,       */ 
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

#ifndef TX_SEMAPHORE_H
#define TX_SEMAPHORE_H


/* Define semaphore control specific data definitions.  */

#define TX_SEMAPHORE_ID                         ((ULONG) 0x53454D41)


/* Determine if in-line component initialization is supported by the 
   caller.  */
#ifdef TX_INVOKE_INLINE_INITIALIZATION
            /* Yes, in-line initialization is supported, remap the 
               semaphore initialization function.  */
#ifndef TX_SEMAPHORE_ENABLE_PERFORMANCE_INFO
#define _tx_semaphore_initialize() \
                    _tx_semaphore_created_ptr =        TX_NULL; \
                    _tx_semaphore_created_count =      0; 
#else
#define _tx_semaphore_initialize() \
                    _tx_semaphore_created_ptr =                   TX_NULL; \
                    _tx_semaphore_created_count =                 0; \
                    _tx_semaphore_performance_put_count =         0; \
                    _tx_semaphore_performance_get_count =         0; \
                    _tx_semaphore_performance_suspension_count =  0; \
                    _tx_semaphore_performance_timeout_count =     0;
#endif
#define TX_SEMAPHORE_INIT
#else
            /* No in-line initialization is supported, use standard 
               function call.  */
VOID        _tx_semaphore_initialize(VOID);
#endif


/* Define semaphore management function prototypes.  */

UINT        _tx_semaphore_ceiling_put(TX_SEMAPHORE *semaphore_ptr, ULONG ceiling);
UINT        _tx_semaphore_create(TX_SEMAPHORE *semaphore_ptr, const CHAR *name_ptr, ULONG initial_count);
UINT        _tx_semaphore_delete(TX_SEMAPHORE *semaphore_ptr);
UINT        _tx_semaphore_get(TX_SEMAPHORE *semaphore_ptr, ULONG wait_option);
UINT        _tx_semaphore_info_get(TX_SEMAPHORE *semaphore_ptr, const CHAR **name, ULONG *current_value,
                    TX_THREAD **first_suspended, ULONG *suspended_count, 
                    TX_SEMAPHORE **next_semaphore);
UINT        _tx_semaphore_performance_info_get(TX_SEMAPHORE *semaphore_ptr, ULONG *puts, ULONG *gets,
                    ULONG *suspensions, ULONG *timeouts);
UINT        _tx_semaphore_performance_system_info_get(ULONG *puts, ULONG *gets, ULONG *suspensions, ULONG *timeouts);
UINT        _tx_semaphore_prioritize(TX_SEMAPHORE *semaphore_ptr);
UINT        _tx_semaphore_put(TX_SEMAPHORE *semaphore_ptr);
UINT        _tx_semaphore_put_notify(TX_SEMAPHORE *semaphore_ptr, VOID (*semaphore_put_notify)(TX_SEMAPHORE *));
VOID        _tx_semaphore_cleanup(TX_THREAD *thread_ptr);


/* Define error checking shells for API services.  These are only referenced by the 
   application.  */

UINT        _txe_semaphore_ceiling_put(TX_SEMAPHORE *semaphore_ptr, ULONG ceiling);
UINT        _txe_semaphore_create(TX_SEMAPHORE *semaphore_ptr, const CHAR *name_ptr, ULONG initial_count, UINT semaphore_control_block_size);
UINT        _txe_semaphore_delete(TX_SEMAPHORE *semaphore_ptr);
UINT        _txe_semaphore_get(TX_SEMAPHORE *semaphore_ptr, ULONG wait_option);
UINT        _txe_semaphore_info_get(TX_SEMAPHORE *semaphore_ptr, const CHAR **name, ULONG *current_value,
                    TX_THREAD **first_suspended, ULONG *suspended_count, 
                    TX_SEMAPHORE **next_semaphore);
UINT        _txe_semaphore_prioritize(TX_SEMAPHORE *semaphore_ptr);
UINT        _txe_semaphore_put(TX_SEMAPHORE *semaphore_ptr);
UINT        _txe_semaphore_put_notify(TX_SEMAPHORE *semaphore_ptr, VOID (*semaphore_put_notify)(TX_SEMAPHORE *));


/* Semaphore management component data declarations follow.  */

/* Determine if the initialization function of this component is including
   this file.  If so, make the data definitions really happen.  Otherwise,
   make them extern so other functions in the component can access them.  */

#ifdef TX_SEMAPHORE_INIT
#define SEMAPHORE_DECLARE 
#else
#define SEMAPHORE_DECLARE extern
#endif


/* Define the head pointer of the created semaphore list.  */

SEMAPHORE_DECLARE  TX_SEMAPHORE *   _tx_semaphore_created_ptr;


/* Define the variable that holds the number of created semaphores. */

SEMAPHORE_DECLARE  ULONG            _tx_semaphore_created_count;


#ifdef TX_SEMAPHORE_ENABLE_PERFORMANCE_INFO

/* Define the total number of semaphore puts.  */

SEMAPHORE_DECLARE  ULONG            _tx_semaphore_performance_put_count;


/* Define the total number of semaphore gets.  */

SEMAPHORE_DECLARE  ULONG            _tx_semaphore_performance_get_count;


/* Define the total number of semaphore suspensions.  */

SEMAPHORE_DECLARE  ULONG            _tx_semaphore_performance_suspension_count;


/* Define the total number of semaphore timeouts.  */

SEMAPHORE_DECLARE  ULONG            _tx_semaphore_performance_timeout_count;


#endif

#endif

