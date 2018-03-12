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
/**   Byte Memory                                                         */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  COMPONENT DEFINITION                                   RELEASE        */ 
/*                                                                        */ 
/*    tx_byte_pool.h                                      PORTABLE C      */ 
/*                                                           5.6          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    William E. Lamie, Express Logic, Inc.                               */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the ThreadX byte memory management component,     */ 
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
/*  12-12-2009     William E. Lamie         Modified comment(s), and      */ 
/*                                            removed unused constant     */ 
/*                                            TX_BYTE_BLOCK_ALLOC,        */ 
/*                                            resulting in version 5.4    */ 
/*  07-15-2011     William E. Lamie         Modified comment(s),          */ 
/*                                            resulting in version 5.5    */ 
/*  11-01-2012     William E. Lamie         Modified comment(s),          */ 
/*                                            resulting in version 5.6    */ 
/*                                                                        */ 
/**************************************************************************/ 

#ifndef TX_BYTE_POOL_H
#define TX_BYTE_POOL_H


/* Define byte memory control specific data definitions.  */

#define TX_BYTE_POOL_ID                         ((ULONG) 0x42595445)
#define TX_BYTE_BLOCK_FREE                      ((ULONG) 0xFFFFEEEE)
#define TX_BYTE_BLOCK_MIN                       20
#define TX_BYTE_POOL_MIN                        100
#define TX_BYTE_SLEEP_TIME                      2


/* Determine if in-line component initialization is supported by the 
   caller.  */

#ifdef TX_INVOKE_INLINE_INITIALIZATION

/* Yes, in-line initialization is supported, remap the byte memory pool 
   initialization function.  */

#ifndef TX_BYTE_POOL_ENABLE_PERFORMANCE_INFO
#define _tx_byte_pool_initialize() \
                    _tx_byte_pool_created_ptr =                   TX_NULL; \
                    _tx_byte_pool_created_count =                 0;
#else
#define _tx_byte_pool_initialize() \
                    _tx_byte_pool_created_ptr =                   TX_NULL; \
                    _tx_byte_pool_created_count =                 0; \
                    _tx_byte_pool_performance_allocate_count =    0; \
                    _tx_byte_pool_performance_release_count =     0; \
                    _tx_byte_pool_performance_merge_count =       0; \
                    _tx_byte_pool_performance_split_count =       0; \
                    _tx_byte_pool_performance_search_count =      0; \
                    _tx_byte_pool_performance_suspension_count =  0; \
                    _tx_byte_pool_performance_timeout_count =     0;
#endif
#define TX_BYTE_POOL_INIT
#else

/* No in-line initialization is supported, use standard function call.  */
VOID        _tx_byte_pool_initialize(VOID);
#endif


/* Define byte memory pool management function prototypes.  */

UINT        _tx_byte_allocate(TX_BYTE_POOL *pool_ptr, VOID **memory_ptr, ULONG memory_size,
                    ULONG wait_option);
UINT        _tx_byte_pool_create(TX_BYTE_POOL *pool_ptr, const CHAR *name_ptr, VOID *pool_start,
                    ULONG pool_size);
UINT        _tx_byte_pool_delete(TX_BYTE_POOL *pool_ptr);
UINT        _tx_byte_pool_info_get(TX_BYTE_POOL *pool_ptr, const CHAR **name, ULONG *available_bytes,
                    ULONG *fragments, TX_THREAD **first_suspended, 
                    ULONG *suspended_count, TX_BYTE_POOL **next_pool);
UINT        _tx_byte_pool_performance_info_get(TX_BYTE_POOL *pool_ptr, ULONG *allocates, ULONG *releases,
                    ULONG *fragments_searched, ULONG *merges, ULONG *splits, ULONG *suspensions, ULONG *timeouts);
UINT        _tx_byte_pool_performance_system_info_get(ULONG *allocates, ULONG *releases,
                    ULONG *fragments_searched, ULONG *merges, ULONG *splits, ULONG *suspensions, ULONG *timeouts);
UINT        _tx_byte_pool_prioritize(TX_BYTE_POOL *pool_ptr);
UINT        _tx_byte_release(VOID *memory_ptr);
VOID       *_tx_byte_pool_search(TX_BYTE_POOL *pool_ptr, ULONG memory_size);
VOID        _tx_byte_pool_cleanup(TX_THREAD *thread_ptr);


/* Define error checking shells for API services.  These are only referenced by the 
   application.  */

UINT        _txe_byte_allocate(TX_BYTE_POOL *pool_ptr, VOID **memory_ptr, ULONG memory_size,
                    ULONG wait_option);
UINT        _txe_byte_pool_create(TX_BYTE_POOL *pool_ptr, const CHAR *name_ptr, VOID *pool_start,
                    ULONG pool_size, UINT pool_control_block_size);
UINT        _txe_byte_pool_delete(TX_BYTE_POOL *pool_ptr);
UINT        _txe_byte_pool_info_get(TX_BYTE_POOL *pool_ptr, const CHAR **name, ULONG *available_bytes,
                    ULONG *fragments, TX_THREAD **first_suspended, 
                    ULONG *suspended_count, TX_BYTE_POOL **next_pool);
UINT        _txe_byte_pool_prioritize(TX_BYTE_POOL *pool_ptr);
UINT        _txe_byte_release(VOID *memory_ptr);


/* Byte pool management component data declarations follow.  */

/* Determine if the initialization function of this component is including
   this file.  If so, make the data definitions really happen.  Otherwise,
   make them extern so other functions in the component can access them.  */

#ifdef TX_BYTE_POOL_INIT
#define BYTE_POOL_DECLARE 
#else
#define BYTE_POOL_DECLARE extern
#endif


/* Define the head pointer of the created byte pool list.  */

BYTE_POOL_DECLARE  TX_BYTE_POOL *   _tx_byte_pool_created_ptr;


/* Define the variable that holds the number of created byte pools. */

BYTE_POOL_DECLARE  ULONG            _tx_byte_pool_created_count;


#ifdef TX_BYTE_POOL_ENABLE_PERFORMANCE_INFO

/* Define the total number of allocates.  */

BYTE_POOL_DECLARE  ULONG            _tx_byte_pool_performance_allocate_count;


/* Define the total number of releases.  */

BYTE_POOL_DECLARE  ULONG            _tx_byte_pool_performance_release_count;


/* Define the total number of adjacent memory fragment merges.  */

BYTE_POOL_DECLARE  ULONG            _tx_byte_pool_performance_merge_count;


/* Define the total number of memory fragment splits.  */

BYTE_POOL_DECLARE  ULONG            _tx_byte_pool_performance_split_count;


/* Define the total number of memory fragments searched during allocation.  */

BYTE_POOL_DECLARE  ULONG            _tx_byte_pool_performance_search_count;


/* Define the total number of byte pool suspensions.  */

BYTE_POOL_DECLARE  ULONG            _tx_byte_pool_performance_suspension_count;


/* Define the total number of byte pool timeouts.  */

BYTE_POOL_DECLARE  ULONG            _tx_byte_pool_performance_timeout_count;


#endif


#endif
