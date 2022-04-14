/**************************************************************************/ 
/*                                                                        */ 
/*            Copyright (c) 1996-2006 by Express Logic Inc.               */ 
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
/**   MD5 Digest Algorithm (MD5)                                          */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  COMPONENT DEFINITION                                   RELEASE        */ 
/*                                                                        */ 
/*    nx_md5.h                                            PORTABLE C      */ 
/*                                                           5.0          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    William E. Lamie, Express Logic, Inc.                               */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the NetX MD5 algorithm, derived principally from  */ 
/*    RFC1321. From a user-specified number of input bytes, this routine  */ 
/*    produces a 16-byte (128-bit) digest or sometimes called a hash      */ 
/*    value. The resulting digest is returned in a 16-byte array supplied */ 
/*    by the caller.                                                      */ 
/*                                                                        */ 
/*    It is assumed that nx_api.h and nx_port.h have already been         */ 
/*    included.                                                           */
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  03-01-2006     William E. Lamie         Initial Version 5.0           */ 
/*                                                                        */ 
/**************************************************************************/ 

#ifndef  NX_MD5_H
#define  NX_MD5_H


/* Define the MD5 context structure.  */

typedef struct NX_MD5_STRUCT
{

    ULONG       nx_md5_states[4];                       /* Contains each state (A,B,C,D)    */
    ULONG       nx_md5_bit_count[2];                    /* Contains the 64-bit total bit    */ 
                                                        /*   count, where index 0 holds the */ 
                                                        /*   least significant bit count and*/ 
                                                        /*   index 1 contains the most      */ 
                                                        /*   significant portion of the bit */ 
                                                        /*   count                          */ 
    UCHAR       nx_md5_buffer[64];                      /* Working buffer for MD5 algorithm */
                                                        /*   where partial buffers are      */ 
                                                        /*   accumulated until a full block */ 
                                                        /*   can be processed               */ 
} NX_MD5;


/* Define the function prototypes for MD5.  */

UINT        _nx_md5_initialize(NX_MD5 *context);
UINT        _nx_md5_update(NX_MD5 *context, UCHAR *input_ptr, UINT input_length);
UINT        _nx_md5_digest_calculate(NX_MD5 *context, UCHAR digest[16]);
VOID        _nx_md5_process_buffer(NX_MD5 *context, UCHAR buffer[64]);

#endif
