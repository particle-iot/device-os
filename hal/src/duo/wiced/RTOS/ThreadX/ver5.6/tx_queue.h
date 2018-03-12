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
/**   Queue                                                               */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  COMPONENT DEFINITION                                   RELEASE        */ 
/*                                                                        */ 
/*    tx_queue.h                                          PORTABLE C      */ 
/*                                                           5.6          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    William E. Lamie, Express Logic, Inc.                               */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the ThreadX queue management component,           */ 
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
/*  12-12-2008     William E. Lamie         Modified comment(s), and      */ 
/*                                            added macro for copying     */ 
/*                                            queue message, resulting    */ 
/*                                            in version 5.2              */ 
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

#ifndef TX_QUEUE_H
#define TX_QUEUE_H


/* Define queue control specific data definitions.  */

#define TX_QUEUE_ID                             ((ULONG) 0x51554555)


/* Determine if in-line component initialization is supported by the 
   caller.  */
#ifdef TX_INVOKE_INLINE_INITIALIZATION

/* Yes, in-line initialization is supported, remap the queue initialization 
   function.  */

#ifndef TX_QUEUE_ENABLE_PERFORMANCE_INFO
#define _tx_queue_initialize() \
                    _tx_queue_created_ptr =                          TX_NULL; \
                    _tx_queue_created_count =                        0; 
#else
#define _tx_queue_initialize() \
                    _tx_queue_created_ptr =                          TX_NULL; \
                    _tx_queue_created_count =                        0; \
                    _tx_queue_performance_messages_sent_count =      0; \
                    _tx_queue_performance_messages_received_count =  0; \
                    _tx_queue_performance_empty_suspension_count =   0; \
                    _tx_queue_performance_full_suspension_count =    0; \
                    _tx_queue_performance_timeout_count =            0;
#endif
#define TX_QUEUE_INIT
#else

/* No in-line initialization is supported, use standard function call.  */
VOID        _tx_queue_initialize(VOID);
#endif


/* Define the message copy macro. Note that the source and destination 
   pointers must be modified since they are used subsequently.  */

#ifndef TX_QUEUE_MESSAGE_COPY
#define TX_QUEUE_MESSAGE_COPY(s, d, z) \
                    *d++ = *s++; \
                    if (z > 1) \
                    { \
                        while (--z) \
                        { \
                            *d++ =  *s++; \
                        } \
                    }
#endif


/* Define queue management function prototypes.  */

UINT        _tx_queue_create(TX_QUEUE *queue_ptr, const CHAR *name_ptr, UINT message_size,
                        VOID *queue_start, ULONG queue_size);
UINT        _tx_queue_delete(TX_QUEUE *queue_ptr);
UINT        _tx_queue_flush(TX_QUEUE *queue_ptr);
UINT        _tx_queue_info_get(TX_QUEUE *queue_ptr, const CHAR **name, ULONG *enqueued, ULONG *available_storage,
                    TX_THREAD **first_suspended, ULONG *suspended_count, TX_QUEUE **next_queue);
UINT        _tx_queue_performance_info_get(TX_QUEUE *queue_ptr, ULONG *messages_sent, ULONG *messages_received,
                    ULONG *empty_suspensions, ULONG *full_suspensions, ULONG *full_errors, ULONG *timeouts);
UINT        _tx_queue_performance_system_info_get(ULONG *messages_sent, ULONG *messages_received,
                    ULONG *empty_suspensions, ULONG *full_suspensions, ULONG *full_errors, ULONG *timeouts);
UINT        _tx_queue_prioritize(TX_QUEUE *queue_ptr);
UINT        _tx_queue_receive(TX_QUEUE *queue_ptr, VOID *destination_ptr, ULONG wait_option);
UINT        _tx_queue_send(TX_QUEUE *queue_ptr, VOID *source_ptr, ULONG wait_option);
UINT        _tx_queue_send_notify(TX_QUEUE *queue_ptr, VOID (*queue_send_notify)(TX_QUEUE *));
UINT        _tx_queue_front_send(TX_QUEUE *queue_ptr, VOID *source_ptr, ULONG wait_option);
VOID        _tx_queue_cleanup(TX_THREAD *thread_ptr);

/* Define error checking shells for API services.  These are only referenced by the 
   application.  */

UINT        _txe_queue_create(TX_QUEUE *queue_ptr, const CHAR *name_ptr, UINT message_size,
                        VOID *queue_start, ULONG queue_size, UINT queue_control_block_size);
UINT        _txe_queue_delete(TX_QUEUE *queue_ptr);
UINT        _txe_queue_flush(TX_QUEUE *queue_ptr);
UINT        _txe_queue_info_get(TX_QUEUE *queue_ptr, const CHAR **name, ULONG *enqueued, ULONG *available_storage,
                    TX_THREAD **first_suspended, ULONG *suspended_count, TX_QUEUE **next_queue);
UINT        _txe_queue_prioritize(TX_QUEUE *queue_ptr);
UINT        _txe_queue_receive(TX_QUEUE *queue_ptr, VOID *destination_ptr, ULONG wait_option);
UINT        _txe_queue_send(TX_QUEUE *queue_ptr, VOID *source_ptr, ULONG wait_option);
UINT        _txe_queue_send_notify(TX_QUEUE *queue_ptr, VOID (*queue_send_notify)(TX_QUEUE *));
UINT        _txe_queue_front_send(TX_QUEUE *queue_ptr, VOID *source_ptr, ULONG wait_option);


/* Queue management component data declarations follow.  */

/* Determine if the initialization function of this component is including
   this file.  If so, make the data definitions really happen.  Otherwise,
   make them extern so other functions in the component can access them.  */

#ifdef TX_QUEUE_INIT
#define QUEUE_DECLARE 
#else
#define QUEUE_DECLARE extern
#endif


/* Define the head pointer of the created queue list.  */

QUEUE_DECLARE  TX_QUEUE *   _tx_queue_created_ptr;


/* Define the variable that holds the number of created queues. */

QUEUE_DECLARE  ULONG        _tx_queue_created_count;


#ifdef TX_QUEUE_ENABLE_PERFORMANCE_INFO

/* Define the total number of messages sent.  */

QUEUE_DECLARE  ULONG        _tx_queue_performance_messages_sent_count;


/* Define the total number of messages received.  */

QUEUE_DECLARE  ULONG        _tx_queue_performance_messages_received_count;


/* Define the total number of queue empty suspensions.  */

QUEUE_DECLARE  ULONG        _tx_queue_performance_empty_suspension_count;


/* Define the total number of queue full suspensions.  */

QUEUE_DECLARE  ULONG        _tx_queue_performance_full_suspension_count;


/* Define the total number of queue full errors.  */

QUEUE_DECLARE  ULONG        _tx_queue_performance_full_error_count;


/* Define the total number of queue timeouts.  */

QUEUE_DECLARE  ULONG        _tx_queue_performance_timeout_count;


#endif

#endif

