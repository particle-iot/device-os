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
/**   SHA1 Digest Algorithm (SHA1)                                        */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  COMPONENT DEFINITION                                   RELEASE        */ 
/*                                                                        */ 
/*    nx_sha1.h                                           PORTABLE C      */ 
/*                                                           5.0          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    William E. Lamie, Express Logic, Inc.                               */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the NetX SHA1 algorithm, derived principally from */ 
/*    RFC3174. From a user-specified number of input bytes, this routine  */ 
/*    produces a 20-byte (160-bit) digest or sometimes called a hash      */ 
/*    value. The resulting digest is returned in a 20-byte array supplied */ 
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

#ifndef  NX_SHA1_H
#define  NX_SHA1_H


/* Define the SHA1 context structure.  */

typedef struct NX_SHA1_STRUCT
{

    ULONG       nx_sha1_states[5];                      /* Contains each state (A,B,C,D)    */
    ULONG       nx_sha1_bit_count[2];                   /* Contains the 64-bit total bit    */ 
                                                        /*   count, where index 0 holds the */ 
                                                        /*   least significant bit count and*/ 
                                                        /*   index 1 contains the most      */ 
                                                        /*   significant portion of the bit */ 
                                                        /*   count                          */ 
    UCHAR       nx_sha1_buffer[64];                     /* Working buffer for SHA1 algorithm*/
                                                        /*   where partial buffers are      */ 
                                                        /*   accumulated until a full block */ 
                                                        /*   can be processed               */ 
    ULONG       nx_sha1_word_array[80];                 /* Working 80 word array            */ 
} NX_SHA1;


/* Define the function prototypes for SHA1.  */

UINT        _nx_sha1_initialize(NX_SHA1 *context);
UINT        _nx_sha1_update(NX_SHA1 *context, UCHAR *input_ptr, UINT input_length);
UINT        _nx_sha1_digest_calculate(NX_SHA1 *context, UCHAR digest[20]);
VOID        _nx_sha1_process_buffer(NX_SHA1 *context, UCHAR buffer[64]);

#endif
