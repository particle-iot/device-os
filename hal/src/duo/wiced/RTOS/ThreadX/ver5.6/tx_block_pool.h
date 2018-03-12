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
/**   Block Memory                                                        */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  COMPONENT DEFINITION                                   RELEASE        */ 
/*                                                                        */ 
/*    tx_block_pool.h                                     PORTABLE C      */ 
/*                                                           5.6          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    William E. Lamie, Express Logic, Inc.                               */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the ThreadX block memory management component,    */ 
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

#ifndef TX_BLOCK_POOL_H
#define TX_BLOCK_POOL_H


/* Define block memory control specific data definitions.  */

#define TX_BLOCK_POOL_ID                        ((ULONG) 0x424C4F43)


/* Determine if in-line component initialization is supported by the 
   caller.  */

#ifdef TX_INVOKE_INLINE_INITIALIZATION

/* Yes, in-line initialization is supported, remap the block memory pool 
   initialization function.  */

#ifndef TX_BLOCK_POOL_ENABLE_PERFORMANCE_INFO
#define _tx_block_pool_initialize() \
                    _tx_block_pool_created_ptr =                   TX_NULL; \
                    _tx_block_pool_created_count =                 0;
#else
#define _tx_block_pool_initialize() \
                    _tx_block_pool_created_ptr =                   TX_NULL; \
                    _tx_block_pool_created_count =                 0; \
                    _tx_block_pool_performance_allocate_count =    0; \
                    _tx_block_pool_performance_release_count =     0; \
                    _tx_block_pool_performance_suspension_count =  0; \
                    _tx_block_pool_performance_timeout_count =     0;
#endif
#define TX_BLOCK_POOL_INIT
#else

/* No in-line initialization is supported, use standard function call.  */
VOID        _tx_block_pool_initialize(VOID);
#endif


/* Define block memory pool management function prototypes.  */

UINT        _tx_block_allocate(TX_BLOCK_POOL *pool_ptr, VOID **block_ptr, ULONG wait_option);
UINT        _tx_block_pool_create(TX_BLOCK_POOL *pool_ptr, const CHAR *name_ptr, ULONG block_size,
                    VOID *pool_start, ULONG pool_size);
UINT        _tx_block_pool_delete(TX_BLOCK_POOL *pool_ptr);
UINT        _tx_block_pool_info_get(TX_BLOCK_POOL *pool_ptr, const CHAR **name, ULONG *available_blocks,
                    ULONG *total_blocks, TX_THREAD **first_suspended, 
                    ULONG *suspended_count, TX_BLOCK_POOL **next_pool);
UINT        _tx_block_pool_performance_info_get(TX_BLOCK_POOL *pool_ptr, ULONG *allocates, ULONG *releases,
                    ULONG *suspensions, ULONG *timeouts);
UINT        _tx_block_pool_performance_system_info_get(ULONG *allocates, ULONG *releases,
                    ULONG *suspensions, ULONG *timeouts);
UINT        _tx_block_pool_prioritize(TX_BLOCK_POOL *pool_ptr);
UINT        _tx_block_release(VOID *block_ptr);
VOID        _tx_block_pool_cleanup(TX_THREAD *thread_ptr);


/* Define error checking shells for API services.  These are only referenced by the 
   application.  */

UINT        _txe_block_allocate(TX_BLOCK_POOL *pool_ptr, VOID **block_ptr, ULONG wait_option);
UINT        _txe_block_pool_create(TX_BLOCK_POOL *pool_ptr, const CHAR *name_ptr, ULONG block_size,
                    VOID *pool_start, ULONG pool_size, UINT pool_control_block_size);
UINT        _txe_block_pool_delete(TX_BLOCK_POOL *pool_ptr);
UINT        _txe_block_pool_info_get(TX_BLOCK_POOL *pool_ptr, const CHAR **name, ULONG *available_blocks,
                    ULONG *total_blocks, TX_THREAD **first_suspended, 
                    ULONG *suspended_count, TX_BLOCK_POOL **next_pool);
UINT        _txe_block_pool_prioritize(TX_BLOCK_POOL *pool_ptr);
UINT        _txe_block_release(VOID *block_ptr);


/* Block pool management component data declarations follow.  */

/* Determine if the initialization function of this component is including
   this file.  If so, make the data definitions really happen.  Otherwise,
   make them extern so other functions in the component can access them.  */

#ifdef TX_BLOCK_POOL_INIT
#define BLOCK_POOL_DECLARE 
#else
#define BLOCK_POOL_DECLARE extern
#endif


/* Define the head pointer of the created block pool list.  */

BLOCK_POOL_DECLARE  TX_BLOCK_POOL *   _tx_block_pool_created_ptr;


/* Define the variable that holds the number of created block pools. */

BLOCK_POOL_DECLARE  ULONG            _tx_block_pool_created_count;


#ifdef TX_BLOCK_POOL_ENABLE_PERFORMANCE_INFO

/* Define the total number of block allocates.  */

BLOCK_POOL_DECLARE  ULONG                  _tx_block_pool_performance_allocate_count;


/* Define the total number of block releases.  */

BLOCK_POOL_DECLARE  ULONG                  _tx_block_pool_performance_release_count;


/* Define the total number of block pool suspensions.  */

BLOCK_POOL_DECLARE  ULONG                  _tx_block_pool_performance_suspension_count;


/* Define the total number of block pool timeouts.  */

BLOCK_POOL_DECLARE  ULONG                  _tx_block_pool_performance_timeout_count;


#endif


#endif
