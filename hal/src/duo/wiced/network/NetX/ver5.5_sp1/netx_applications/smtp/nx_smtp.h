/**************************************************************************/
/*                                                                        */
/*            Copyright (c) 1996-2011 by Express Logic Inc.               */
/*                                                                        */
/*  This software is copyrighted by and is the sole property of Express   */
/*  Logic, Inc.  All rights, title, ownership, or other interests         */
/*  in the software remain the property of Express Logic, Inc. This       */
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
/**   Simple Mail Transfer Protocol (SMTP)                                */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/**************************************************************************/
/*                                                                        */
/*  APPLICATION INTERFACE DEFINITION                       RELEASE        */
/*                                                                        */
/*    nx_smtp.h                                           PORTABLE C      */
/*                                                           5.2          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Janet Christiansen, Express Logic, Inc.                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This file contains the NetX Simple Mail Transfer Protocol (SMTP)    */
/*    components, including data types and external references, common    */
/*    to both SMTP Server and SMTP Client API.                            */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  09-24-2007     Janet Christiansen         Initial version 5.0         */
/*  04-01-2010     Janet Christiansen         Modified comment(s),        */ 
/*                                              resulting in version 5.1  */
/*  07-11-2011     Janet Christiansen         Modified comment(s),        */
/*                                              resulting in version 5.2  */
/*                                                                        */
/**************************************************************************/

#ifndef NX_SMTP_H
#define NX_SMTP_H

#include "tx_api.h"
#include "nx_api.h"
#include "nx_ip.h"


/* Determine if a C++ compiler is being used.  If so, ensure that standard
   C is used to process the API information.  */

#ifdef   __cplusplus

/* Yes, C++ compiler is present.  Use standard C.  */ 
extern   "C" {

#endif



/* ID for identifying as an SMTP client */

#define NX_SMTP_ID                              0x534D5450UL

/* Conversion between seconds and timer ticks. See tx_initialize_low_level.<asm> 
   for timer tick resolution before altering! */ 

#define NX_SMTP_MILLISECONDS_PER_TICK           10

#define NX_SMTP_TICKS_PER_SECOND                100

    /*  SMTP Debug levels in decreased filtering order:  

    NONE:       No events reported;
    SEVERE:     Report only events requiring session or server to stop operation.
    MODERATE:   Report events possibly preventing successful mail transaction
    ALL:        All events reported  */

#define NX_SMTP_DEBUG_LEVEL_NONE     (0)
#define NX_SMTP_DEBUG_LEVEL_SEVERE   (1)
#define NX_SMTP_DEBUG_LEVEL_MODERATE (2)
#define NX_SMTP_DEBUG_LEVEL_ALL      (3)



/* Internal error processing codes. */

#define NX_SMTP_ERROR_CONSTANT          0xB0

#define NX_SMTP_MEMORY_ERROR            (NX_SMTP_ERROR_CONSTANT | 0x1)       /* Memory  handling failure   */
#define NX_SMTP_PARAM_ERROR             (NX_SMTP_ERROR_CONSTANT | 0x2)       /* Invalid parameter received by API service */
#define NX_SMTP_MAIL_MESSAGE_ERROR      (NX_SMTP_ERROR_CONSTANT | 0x3)       /* Error processing mail message data.  */
#define NX_SMTP_SESSION_MAIL_ERROR      (NX_SMTP_ERROR_CONSTANT | 0x4)       /* Invalid list of session mail (e.g. link broken).  */
#define NX_SMTP_MAIL_RECIPIENT_ERROR    (NX_SMTP_ERROR_CONSTANT | 0x5)       /* Invalid list of mail recipients (e.g. link broken).  */


/* Defines for deciding priority of mail.  */

#define NX_SMTP_MAIL_PRIORITY_LOW               0x01
#define NX_SMTP_MAIL_PRIORITY_NORMAL            0x02
#define NX_SMTP_MAIL_PRIORITY_HIGH              0x04


/* Defines for type of mail recipient.  */

#define NX_SMTP_RECIPIENT_TO                    0x01
#define NX_SMTP_RECIPIENT_CC                    0x02
#define NX_SMTP_RECIPIENT_BCC                   0x04


/* Define size of SMTP reply status codes (RFC mandated). */

#define NX_SMTP_SERVER_REPLY_CODE_SIZE          3


/* Maximum client command line size (RFC mandated). */

#define NX_SMTP_CMD_SIZE_MAX                    512   


/* Maximum server reply line size (RFC mandated). */

#define NX_SMTP_SERVER_REPLY_LINE_SIZE_MAX      512   


/* Maximum length of mail message data per line (RFC mandated).  */ 

#define NX_SMTP_MESSAGE_LINE_SIZE_MAX           1000  


/* Maximum size of SMTP multiline response (RFC mandated).*/

#define NX_SMTP_MULTILINE_RESPONSE_MAX_SIZE     64 * 1024 


/* Basic SMTP commands supported by this NetX SMTP API.  */

#define NX_SMTP_COMMAND_EHLO                            "EHLO"
#define NX_SMTP_COMMAND_HELO                            "HELO"
#define NX_SMTP_COMMAND_MAIL                            "MAIL FROM"
#define NX_SMTP_COMMAND_RCPT                            "RCPT TO"
#define NX_SMTP_COMMAND_AUTH                            "AUTH"
#define NX_SMTP_COMMAND_NOOP                            "NOOP"
#define NX_SMTP_COMMAND_DATA                            "DATA"
#define NX_SMTP_COMMAND_RSET                            "RSET"
#define NX_SMTP_COMMAND_QUIT                            "QUIT"

/* List of common SMTP server reply codes */

#define     NX_SMTP_CODE_GREETING_OK                       220 
#define     NX_SMTP_CODE_ACKNOWLEDGE_QUIT                  221
#define     NX_SMTP_CODE_AUTHENTICATION_SUCCESSFUL         235
#define     NX_SMTP_CODE_OK_TO_CONTINUE                    250
#define     NX_SMTP_CODE_CANNOT_VERIFY_RECIPIENT           252
#define     NX_SMTP_CODE_AUTHENTICATION_TYPE_ACCEPTED      334
#define     NX_SMTP_CODE_SEND_MAIL_INPUT                   354
#define     NX_SMTP_CODE_SERVICE_NOT_AVAILABLE             421
#define     NX_SMTP_CODE_SERVICE_INTERNAL_SERVER_ERROR     451
#define     NX_SMTP_CODE_INSUFFICIENT_STORAGE              452
#define     NX_SMTP_CODE_AUTH_FAILED_INTERNAL_SERVER_ERROR 454
#define     NX_SMTP_CODE_COMMAND_SYNTAX_ERROR              500
#define     NX_SMTP_CODE_PARAMETER_SYNTAX_ERROR            501
#define     NX_SMTP_CODE_COMMAND_NOT_IMPLEMENTED           502
#define     NX_SMTP_CODE_BAD_SEQUENCE                      503
#define     NX_SMTP_CODE_PARAMETER_NOT_IMPLEMENTED         504
#define     NX_SMTP_CODE_AUTH_REQUIRED                     530
#define     NX_SMTP_CODE_AUTH_FAILED                       535
#define     NX_SMTP_CODE_REQUESTED_ACTION_NOT_TAKEN        550
#define     NX_SMTP_CODE_USER_NOT_LOCAL                    551 
#define     NX_SMTP_CODE_OVERSIZE_MAIL_DATA                552
#define     NX_SMTP_CODE_BAD_MAILBOX                       553
#define     NX_SMTP_CODE_TRANSACTION_FAILED                554
#define     NX_SMTP_CODE_BAD_SERVER_CODE_RECEIVED          555


/* Common components of SMTP command messages */

#define NX_SMTP_LINE_TERMINATOR                     "\r\n"
#define NX_SMTP_EOM                                 "\r\n.\r\n"   
#define NX_SMTP_MESSAGE_ID                          "Message-ID"


/* Defines for known authentication types.  */
                                        
#define NX_SMTP_AUTH_TYPE_CRAM_MD5                   "CRAM-MD5"
#define NX_SMTP_AUTH_TYPE_LOGIN                      "LOGIN"
#define NX_SMTP_AUTH_TYPE_PLAIN                      "PLAIN"


/* Define the character to cancel authentication process (RFC mandated). */

#define NX_SMTP_CANCEL_AUTHENTICATION                "*"


/* Enumeration of the state of authentication between server and client */

typedef enum  NX_SMTP_SESSION_AUTHENTICATION_STATE_ENUM
{
    NX_SMTP_NOT_AUTHENTICATED,
    NX_SMTP_AUTHENTICATION_IN_PROGRESS, 
    NX_SMTP_AUTHENTICATION_FAILED,
    NX_SMTP_AUTHENTICATION_SUCCEEDED

} NX_SMTP_SESSION_AUTHENTICATION_STATE;


/* Common server challenges to the client */

#define NX_SMTP_USERNAME_PROMPT                 "Username:"

#define NX_SMTP_PASSWORD_PROMPT                 "Password:"


/* Enumeration of common server challenges to the client */

typedef enum NX_SMTP_REPLY_TO_AUTH_PROMPT_ENUM
{
    NX_SMTP_REPLY_TO_UNKNOWN_PROMPT,
    NX_SMTP_REPLY_TO_USERNAME_PROMPT,
    NX_SMTP_REPLY_TO_PASSWORD_PROMPT

} NX_SMTP_REPLY_TO_AUTH_PROMPT;


/* Define size for single space delimited string of authentication types supported. */

#ifndef NX_SMTP_MAX_AUTH_LIST
#define NX_SMTP_MAX_AUTH_LIST                   60  
#endif


/* Define size for name of authentication type. */

#ifndef NX_SMTP_SESSION_AUTH_TYPE
#define NX_SMTP_SESSION_AUTH_TYPE               20
#endif


/* Define size for authentication data components. */

#ifndef NX_SMTP_MAX_USERNAME
#define NX_SMTP_MAX_USERNAME                    40
#endif

#ifndef NX_SMTP_MAX_PASSWORD
#define NX_SMTP_MAX_PASSWORD                    20
#endif


/* Define size for encryption of server challenge or prompt. */

#ifndef NX_SMTP_ENCRYPTION_MAX_STRING
#define NX_SMTP_ENCRYPTION_MAX_STRING           NX_SMTP_MAX_USERNAME + NX_SMTP_MAX_PASSWORD + 20
#endif

/* Define size for string containing time, day, and time zone. */

#ifndef NX_SMTP_DATE_AND_TIME_STAMP_SIZE
#define NX_SMTP_DATE_AND_TIME_STAMP_SIZE        50
#endif


/* Define size for domain name and local part of mailbox address.  Max sizes recommended by 
   the RFC for SMTP are 255 and 64 respectively. However the NetX SMTP server does not 
   support relaying or gateways it is unlikely there could be any 
   domain name or local part that approach these limits. */

#ifndef NX_SMTP_MAX_DOMAIN_NAME                 
#define NX_SMTP_MAX_DOMAIN_NAME                 75
#endif

#ifndef NX_SMTP_MAX_LOCAL_PART
#define NX_SMTP_MAX_LOCAL_PART                  32
#endif


/* Define size for server authentication challenges (prompts). RFC has no limits on this length.
   However, NetX SMTP Client and NetX SMTP Server are limited to 
   username, password in this API so we restrict this to a reasonable size. */

#ifndef NX_SMTP_MAX_SERVER_AUTH_PROMPT
#define NX_SMTP_MAX_SERVER_AUTH_PROMPT          20
#endif


/* Define the NetX SMTP Message Segment structure. This is common to the NetX SMTP Client
   and NetX SMTP Server MAIL structure. */

typedef struct NX_SMTP_MESSAGE_SEGMENT_STRUCT
{
    CHAR                                  *message_ptr;            /* Pointer to message segment data. */
    ULONG                                  message_segment_length; /* Size of message segment. */
    struct NX_SMTP_MESSAGE_SEGMENT_STRUCT *next_ptr;               /* Pointer to next message segment. */
                                                                     
} NX_SMTP_MESSAGE_SEGMENT;


/* If a C++ compiler is being used */
#ifdef   __cplusplus
        }
#endif

#endif /* NX_SMTP_H  */
