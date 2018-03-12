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
/**   DES Encryption Standard (DES)                                       */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  COMPONENT DEFINITION                                   RELEASE        */ 
/*                                                                        */ 
/*    nx_des.h                                            PORTABLE C      */ 
/*                                                           5.0          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    William E. Lamie, Express Logic, Inc.                               */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the NetX DES encryption algorithm, derived        */ 
/*    principally from FIPS-46. From an 8 bytes of raw input, the DES     */ 
/*    encryption routine produces an 8-byte encryption of the input.      */ 
/*    Conversely, from an 8-byte encryption, the decryption routine       */ 
/*    produces the original 8 bytes of input. Note that the caller must   */ 
/*    ensure 8 bytes of input and output are provided.                    */ 
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

#ifndef  NX_DES_H
#define  NX_DES_H

/* Define the DES context structure.  */

typedef struct NX_DES_STRUCT
{

    ULONG       nx_des_encryption_keys[32];             /* Contains the encryption keys     */ 
    ULONG       nx_des_decryption_keys[32];             /* Contains the decryption keys     */ 
} NX_DES;


/* Define the function prototypes for DES.  */

UINT        _nx_des_key_set(NX_DES *context, UCHAR key[8]);
UINT        _nx_des_encrypt(NX_DES *context, UCHAR source[8], UCHAR destination[8]);
UINT        _nx_des_decrypt(NX_DES *context, UCHAR source[8], UCHAR destination[8]);
VOID        _nx_des_process_block(UCHAR source[8], UCHAR destination[8], ULONG keys[32]);

#endif

