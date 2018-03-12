/**************************************************************************/
/*                                                                        */
/*            Copyright (c) 1996-2010 by Express Logic Inc.               */
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
/*  Express Logic, Inc.                                                   */
/*  11423 West Bernardo Court               info@expresslogic.com         */
/*  San Diego, CA 92127                     http://www.expresslogic.com   */
/*                                                                        */
/**************************************************************************/

/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** NetX Component                                                        */
/**                                                                       */
/**   POP3 Generic functions                                              */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/**************************************************************************/
/*                                                                        */
/*  APPLICATION INTERFACE DEFINITION                       RELEASE        */
/*                                                                        */
/*    nx_pop3.h                                           PORTABLE C      */
/*                                                            5.1         */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Janet Christiansen, Express Logic, Inc.                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This file defines the NetX Post Office Protocol Version 3 (POP3)    */
/*    component, including all data types and external references.        */
/*    It is assumed that tx_api.h, tx_port.h, nx_api.h, and nx_port.h,    */
/*    have already been included.                                         */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  09-21-2007     Janet Christiansen       Initial Version 5.0           */
/*  04-22-2010     Janet Christiansen        Modified comment(s),         */
/*                                          resulting in version 5.1      */
/*                                                                        */
/**************************************************************************/


#ifndef NX_POP3_H
#define NX_POP3_H

#include "tx_api.h"
#include "nx_api.h"
#include "nx_ip.h"


/* Determine if a C++ compiler is being used.  If so, ensure that standard
   C is used to process the API information.  */

#ifdef   __cplusplus

/* Yes, C++ compiler is present.  Use standard C.  */ 
extern   "C" {

#endif


#define NX_POP3_ID    0x504F5033UL

#define NX_POP3_DEBUG_LEVEL_NONE     (0)
#define NX_POP3_DEBUG_LEVEL_LOG      (1)
#define NX_POP3_DEBUG_LEVEL_SEVERE   (2)
#define NX_POP3_DEBUG_LEVEL_MODERATE (3)
#define NX_POP3_DEBUG_LEVEL_ALL      (4)


/* Conversion between seconds and timer ticks. See tx_initialize_low_level.<asm> 
   for timer tick resolution before altering! */ 

#define NX_POP3_MILLISECONDS_PER_TICK       10
#define NX_POP3_TICKS_PER_SECOND            100
        

/* NX-POP3 API Error codes prefixed with MAX_NETX_ERROR_CONSTANT 
   to separate from NetX error codes.  */


#define NX_POP3_CLIENT_ERROR_CONSTANT         0xB0  

#define NX_POP3_PARAM_ERROR                  (NX_POP3_CLIENT_ERROR_CONSTANT | 0x1) /* Invalid non pointer parameter passed    */
#define NX_POP3_ILLEGAL_CLIENT_COMMAND       (NX_POP3_CLIENT_ERROR_CONSTANT | 0x2) /* Client command disallowed in current POP3 protocol state. */
#define NX_POP3_MISMATCHED_BLOCK_PACKET_SIZE (NX_POP3_CLIENT_ERROR_CONSTANT | 0x3) /* Client block pool not large enough to store packet payload. */
#define NX_POP3_APOP_NO_SERVER_PID           (NX_POP3_CLIENT_ERROR_CONSTANT | 0x4) /* Client authentication failed; server greeting missing process ID. */
#define NX_POP3_APOP_FAILED_MD5_DIGEST       (NX_POP3_CLIENT_ERROR_CONSTANT | 0x5) /* Client authentication failed; MD5 digest of server ID and password failed. */
#define NX_POP3_BAD_SERVER_LIST_REPLY        (NX_POP3_CLIENT_ERROR_CONSTANT | 0x6) /* Client LIST command received reply with no message data. */
#define NX_POP3_BAD_SERVER_RETR_REPLY        (NX_POP3_CLIENT_ERROR_CONSTANT | 0x7) /* Client RETR command received reply with no message data. */
#define NX_POP3_RSET_REPLY_ERR               (NX_POP3_CLIENT_ERROR_CONSTANT | 0x8) /* Client RSET command received ERR from server. */
#define NX_POP3_CANNOT_PARSE_REPLY           (NX_POP3_CLIENT_ERROR_CONSTANT | 0x9) /* Server reply appears to missing expected argument(s). */   
#define NX_POP3_SERVER_REJECTS_COMMAND       (NX_POP3_CLIENT_ERROR_CONSTANT | 0xA) /* Server responds to Client command with -ERR. */   
#define NX_POP3_FAILED_PACKET_EXTRACTION     (NX_POP3_CLIENT_ERROR_CONSTANT | 0xB) /* Mail data extraction from chained packets fails. */   
#define NX_POP3_MAIL_BUFFER_OVERRUN          (NX_POP3_CLIENT_ERROR_CONSTANT | 0xC) /* Mail data extraction has filled up mail buffer before extraction complete. */   
#define NX_POP3_PARAM_BUFFER_OVERRUN         (NX_POP3_CLIENT_ERROR_CONSTANT | 0xD) /* Client parameter (e.g. username/password) over runs memory. */   
#define NX_POP3_MAIL_SPOOL_ERROR             (NX_POP3_CLIENT_ERROR_CONSTANT | 0xE) /* Client mail spool callback is not successful. */   
#define NX_POP3_NULL_CURRENT_MAIL            (NX_POP3_CLIENT_ERROR_CONSTANT | 0xF) /* Client session current mail instance is null. */   


#define NX_POP3_SERVER_ERROR_CONSTANT             0xC0  

#define NX_POP3_SERVER_PARAM_ERROR           (NX_POP3_SERVER_ERROR_CONSTANT | 0x1) /* Invalid non pointer parameter passed    */
#define NX_POP3_ERROR_MAILDROP_NOT_FOUND     (NX_POP3_SERVER_ERROR_CONSTANT | 0x2) /* No maildrop found for current session Client. */
#define NX_POP3_MD5_DIGEST_AUTH_FAILED       (NX_POP3_SERVER_ERROR_CONSTANT | 0x3) /* Server rejects Client MD5 digest; authentication fails. */
#define NX_POP3_ERROR_BAD_CLIENT_MAILITEM    (NX_POP3_SERVER_ERROR_CONSTANT | 0x4) /* Server reads in a mailitem missing End Of Message tag, mail may be corrupted or truncated. */
#define NX_POP3_ERROR_NO_PASSWORD_FOUND      (NX_POP3_SERVER_ERROR_CONSTANT | 0x5) /* Client username either not found among Server maildrops or is missing a password. */
#define NX_POP3_ERROR_NO_EOM_FOUND           (NX_POP3_SERVER_ERROR_CONSTANT | 0x6) /* Mail message from Server maildrops is missing the end of message sequence. */
#define NX_POP3_ERROR_COMMAND_MISSING_PARAM  (NX_POP3_SERVER_ERROR_CONSTANT | 0x7) /* Command from Client requires a parameter, but none found in command buffer. */
#define NX_POP3_ERROR_BAD_PARAMETER          (NX_POP3_SERVER_ERROR_CONSTANT | 0x8) /* Non numeric parameter parsed from Client command where valid number expected. */
#define NX_POP3_ERROR_NO_SERVER_MEM_ALLOC    (NX_POP3_SERVER_ERROR_CONSTANT | 0x9) /* Server is not configured for dynamic memory allocation. */
#define NX_POP3_ERROR_BAD_MAIL_DATA_READ     (NX_POP3_SERVER_ERROR_CONSTANT | 0xA) /* Server callback for reading in a chunk of mail data returns invalid # bytes. */
#define NX_POP3_ERROR_COMMAND_MISSING_TERM   (NX_POP3_SERVER_ERROR_CONSTANT | 0xB) /* Client command missing command termination \r\n. */

#define NX_POP3_MAX_BINARY_MD5              16
#define NX_POP3_MAX_ASCII_MD5               33

/* Max size for Client command and Server reply text. */

#define NX_POP3_MAX_SERVER_REPLY            512
#define NX_POP3_MAX_CLIENT_COMMAND          150  

/* Server replies */
#define NX_POP3_POSITIVE_STATUS              "+OK"
#define NX_POP3_NEGATIVE_STATUS              "-ERR"

/* Enumeration of POP3 Server replies. */

#define NX_POP3_CODE_INVALID         0
#define NX_POP3_CODE_OK              1
#define NX_POP3_CODE_ERR             2


/* POP3 commands supported in the NetX POP3 Client API.  */

#define NX_POP3_COMMAND_GREETING            "GREETING"
#define NX_POP3_COMMAND_USER                "USER"
#define NX_POP3_COMMAND_APOP                "APOP"
#define NX_POP3_COMMAND_PASS                "PASS"
#define NX_POP3_COMMAND_STAT                "STAT"
#define NX_POP3_COMMAND_RETR                "RETR"
#define NX_POP3_COMMAND_DELE                "DELE"
#define NX_POP3_COMMAND_QUIT                "QUIT"
#define NX_POP3_COMMAND_LIST                "LIST"
#define NX_POP3_COMMAND_RSET                "RSET"
#define NX_POP3_COMMAND_NOOP                "NOOP"

/* POP3 Session states enumerated.  */      
                                             
typedef enum  NX_POP3_SESSION_STATE_ENUM
{

    NX_POP3_SESSION_STATE_NONE,           
    NX_POP3_SESSION_STATE_AUTHORIZATION, 
    NX_POP3_SESSION_STATE_TRANSACTION,
    NX_POP3_SESSION_STATE_UPDATE

} NX_POP3_SESSION_STATE;



/* Define buffer size for the process ID usually included in the server greeting. Used
   for authenticating the Client using APOP command. */

#define NX_POP3_SERVER_PROCESS_ID              50

#define NX_POP3_COMMAND_TERMINATION            "\r\n"
#define NX_POP3_END_OF_MESSAGE_TAG             ".\r\n"
#define NX_POP3_DOT                            "."
#define NX_POP3_END_OF_MESSAGE                 "\r\n.\r\n"

#ifdef  NX_POP3_ALTERNATE_SNPRINTF
int     _nx_pop3_snprintf(char *str, size_t size, const char *format, ...);
#else
#define _nx_pop3_snprintf snprintf
#endif /* ifdef  NX_POP3_ALTERNATE_SNPRINTF */

/* If a C++ compiler is being used....*/
#ifdef   __cplusplus
        }
#endif


#endif /* NX_POP3_H  */
