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
/*  Express Logic, Inc.                     info@expresslogic.com         */
/*  11423 West Bernardo Court               http://www.expresslogic.com   */
/*  San Diego, CA  92127                                                  */
/*                                                                        */
/**************************************************************************/

/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** NetX SMTP Server Component                                            */
/**                                                                       */
/**   Simple Mail Transfer Protocol (SMTP)                                */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/**************************************************************************/
/*                                                                        */
/*  APPLICATION INTERFACE DEFINITION                       RELEASE        */
/*                                                                        */
/*    nx_smtp_server.h                                    PORTABLE C      */
/*                                                           5.2          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Janet Christiansen, Express Logic, Inc.                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This file defines the NetX Simple Mail Transfer Protocol (SMTP)     */
/*    server component, including all data types and external references. */
/*    It is assumed that tx_api.h, tx_port.h, nx_api.h, and nx_port.h,    */
/*    have already been included.                                         */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  09-24-2007     Janet Christiansen         Initial version 5.0         */
/*  04-01-2010     Janet Christiansen         Modified comment(s),        */ 
/*                                              resulting in version 5.1  */
/*  07-15-2011     Janet Christiansen         Modified comment(s),        */ 
/*                                              resulting in version 5.2  */
/*                                                                        */ 
/**************************************************************************/

#ifndef NX_SMTP_SERVER_H
#define NX_SMTP_SERVER_H


#ifdef   __cplusplus

/* Yes, C++ compiler is present.  Use standard C.  */ 
extern   "C" {

#endif


#include "nx_api.h"
#include "nx_smtp.h"


/* Start of configurable options */

/* Set the event reporting/debug output for the NetX SMTP server */

#ifndef NX_SMTP_SERVER_DEBUG
#define NX_SMTP_SERVER_DEBUG                         NX_SMTP_DEBUG_LEVEL_NONE
#endif


/* Scheme for filtering messages during program execution. 

   printf() itself may need to be defined for the specific 
   processor that is running the application and communication
   available (e.g. serial port).  */

#if ( NX_SMTP_SERVER_DEBUG == NX_SMTP_DEBUG_LEVEL_NONE )
#define NX_SMTP_SERVER_EVENT_LOG(debug_level, msg)
#else
#define NX_SMTP_SERVER_EVENT_LOG(debug_level, msg)                      \
{                                                                       \
    UINT level = (UINT)debug_level;                                     \
    if ((level <= NX_SMTP_DEBUG_LEVEL_ALL) && (NX_SMTP_SERVER_DEBUG == NX_SMTP_DEBUG_LEVEL_ALL))                \
    {                                                                   \
       printf msg ;                                                     \
    }                                                                   \
    else if ((level <= NX_SMTP_DEBUG_LEVEL_MODERATE) && (NX_SMTP_SERVER_DEBUG == NX_SMTP_DEBUG_LEVEL_MODERATE)) \
    {                                                                   \
       printf msg ;                                                     \
    }                                                                   \
    else if ((level == NX_SMTP_DEBUG_LEVEL_SEVERE) && (NX_SMTP_SERVER_DEBUG == NX_SMTP_DEBUG_LEVEL_SEVERE))     \
    {                                                                   \
       printf msg ;                                                     \
    }                                                                   \
}
#endif /* NX_SMTP_SERVER_DEBUG */

/* Enable print server session mail summary feature.  */
/* #define NX_SMTP_PRINT_SERVER_MAIL_DATA  */
  
/* Enable print server packet and memory pool reserves feature.  */
/* #define NX_SMTP_PRINT_SERVER_RESERVES  */


/* Set Net TCP print summary mutex timeout in ticks.  */

#ifndef NX_SMTP_SERVER_PRINT_TIMEOUT            
#define NX_SMTP_SERVER_PRINT_TIMEOUT                 (1 * NX_SMTP_TICKS_PER_SECOND)  
#endif


/* Configure SMTP Server thread and stack parameters.  */

/* Set SMTP server thread priority.  */

#ifndef NX_SMTP_SERVER_THREAD_PRIORITY 
#define NX_SMTP_SERVER_THREAD_PRIORITY                2
#endif


/* Set the SMTP server thread stack size.  */

#ifndef NX_SMTP_SERVER_THREAD_STACK_SIZE        
#define NX_SMTP_SERVER_THREAD_STACK_SIZE              4096
#endif


/* Set SMTP server thread time slice (how long it runs before threads of equal priority may run).  */

#ifndef NX_SMTP_SERVER_THREAD_TIME_SLICE
#define NX_SMTP_SERVER_THREAD_TIME_SLICE              TX_NO_TIME_SLICE
#endif


/* Set SMTP server preemption threshold.  */

#ifndef NX_SMTP_SERVER_PREEMPTION_THRESHOLD
#define NX_SMTP_SERVER_PREEMPTION_THRESHOLD           NX_SMTP_SERVER_THREAD_PRIORITY  
#endif


/* Set the server session stack size.  */

#ifndef NX_SMTP_SERVER_SESSION_THREAD_STACK_SIZE        
#define NX_SMTP_SERVER_SESSION_THREAD_STACK_SIZE      4096 
#endif

/* Set SMTP server session thread priority.  */

#ifndef NX_SMTP_SERVER_SESSION_THREAD_PRIORITY 
#define NX_SMTP_SERVER_SESSION_THREAD_PRIORITY        NX_SMTP_SERVER_THREAD_PRIORITY
#endif


/* Set SMTP server session preemption threshold.  */

#ifndef NX_SMTP_SERVER_SESSION_PREEMPTION_THRESHOLD
#define NX_SMTP_SERVER_SESSION_PREEMPTION_THRESHOLD   NX_SMTP_SERVER_SESSION_THREAD_PRIORITY  
#endif

/* Set SMTP server session thread time slice.  */

#ifndef NX_SMTP_SERVER_SESSION_THREAD_TIME_SLICE
#define NX_SMTP_SERVER_SESSION_THREAD_TIME_SLICE      TX_NO_TIME_SLICE
#endif


/* Configure SMTP server memory resources.  */

/* Set SMTP server byte pool size.  */

#ifndef NX_SMTP_SERVER_BYTE_POOL_SIZE
#define NX_SMTP_SERVER_BYTE_POOL_SIZE                 (1024 * 4)
#endif


/* Set SMTP server byte pool name.  */

#ifndef NX_SMTP_SERVER_BYTE_POOL_NAME
#define NX_SMTP_SERVER_BYTE_POOL_NAME                 "SMTP server bytepool"
#endif


/* Set SMTP server byte pool mutex name.  */

#ifndef NX_SMTP_SERVER_BYTE_POOL_MUTEX_NAME
#define NX_SMTP_SERVER_BYTE_POOL_MUTEX_NAME            "SMPT server bytepool mutex" 
#endif


/* Set SMTP server byte pool mutex timeout.  */

#ifndef NX_SMTP_SERVER_BYTE_POOL_MUTEX_WAIT
#define NX_SMTP_SERVER_BYTE_POOL_MUTEX_WAIT            (2 * NX_SMTP_TICKS_PER_SECOND)
#endif

/* Set SMTP server block pool size.  */

#ifndef NX_SMTP_SERVER_BLOCK_POOL_SIZE
#define NX_SMTP_SERVER_BLOCK_POOL_SIZE                 (10 * NX_SMTP_SERVER_BLOCK_SIZE)
#endif

/* Set server block pool block size.  */
#ifndef NX_SMTP_SERVER_BLOCK_SIZE
#define NX_SMTP_SERVER_BLOCK_SIZE                      NX_SMTP_SERVER_PACKET_SIZE
#endif

/* Set server block pool name.  */

#ifndef NX_SMTP_SERVER_BLOCK_POOL_NAME
#define NX_SMTP_SERVER_BLOCK_POOL_NAME                 "SMTP server blockpool"
#endif

/* Set server block pool mutex name.  */

#ifndef NX_SMTP_SERVER_BLOCK_POOL_MUTEX_NAME
#define NX_SMTP_SERVER_BLOCK_POOL_MUTEX_NAME           "SMTP server blockpool mutex"
#endif

/* Set server block pool mutex timeout.  */

#ifndef NX_SMTP_SERVER_BLOCK_POOL_MUTEX_WAIT
#define NX_SMTP_SERVER_BLOCK_POOL_MUTEX_WAIT           (5 * NX_SMTP_TICKS_PER_SECOND)
#endif

/* Configure the SMTP server network parameters.  */

/* Set port for SMTP server to listen on.  */

#ifndef NX_SMTP_SERVER_SESSION_PORT                       
#define NX_SMTP_SERVER_SESSION_PORT                    25     
#endif

/* Set server domain name as identifier in text replies to SMTP clients. 
   The default NetX SMTP server domain name is fictitious*/

#ifndef NX_SMTP_SERVER_DOMAIN
#define NX_SMTP_SERVER_DOMAIN                          "Server.com" 
#endif


/* Set server socket queue size (number of connection requests that can be queued.  */

#ifndef NX_SMTP_SERVER_SOCKET_QUEUE_SIZE
#define NX_SMTP_SERVER_SOCKET_QUEUE_SIZE               5
#endif

/* Set TCP receive window size (maximum number of bytes in socket receive queue).  */

#ifndef NX_SMTP_SERVER_WINDOW_SIZE   
#define NX_SMTP_SERVER_WINDOW_SIZE                     NX_SMTP_SERVER_PACKET_SIZE
#endif

/* Set timeout on NetX packet allocation.  */

#ifndef NX_SMTP_PACKET_TIMEOUT
#define NX_SMTP_PACKET_TIMEOUT                         (1 * NX_SMTP_TICKS_PER_SECOND)
#endif


/* Set size of NetX SMTP server packet. Best if close to the device MTU.  */

#ifndef NX_SMTP_SERVER_PACKET_SIZE
#define NX_SMTP_SERVER_PACKET_SIZE                     1500  
#endif


/* Set size of header data from network Frame, IP, TCP and NetX in bytes.  */

#ifndef NX_SMTP_SERVER_PACKET_HEADER_SIZE       
#define NX_SMTP_SERVER_PACKET_HEADER_SIZE              60
#endif    

/* Set size of NetX SMTP server packet pool.  */

#ifndef NX_SMTP_SERVER_PACKET_POOL_SIZE    
#define NX_SMTP_SERVER_PACKET_POOL_SIZE                (10 * NX_SMTP_SERVER_PACKET_SIZE)
#endif


/* Set NetX IP helper thread stack size.  */

#ifndef NX_SMTP_SERVER_IP_STACK_SIZE   
#define NX_SMTP_SERVER_IP_STACK_SIZE                   2048
#endif

/* Set the server IP thread priority */

#ifndef NX_SMTP_SERVER_IP_THREAD_PRIORITY
#define NX_SMTP_SERVER_IP_THREAD_PRIORITY              2
#endif

/* Set ARP cache size of Server IP instance.  */

#ifndef NX_SMTP_SERVER_ARP_CACHE_SIZE
#define NX_SMTP_SERVER_ARP_CACHE_SIZE                  1040
#endif

/* Set timeout for server TCP socket send completion.  */

#ifndef NX_SMTP_SERVER_TCP_SOCKET_SEND_WAIT    
#define NX_SMTP_SERVER_TCP_SOCKET_SEND_WAIT            (3  * NX_SMTP_TICKS_PER_SECOND)    
#endif


/* Set NetX SMTP server timeout wait between client commands.  */

#ifndef NX_SMTP_SERVER_TCP_RECEIVE_TIMEOUT              
#define NX_SMTP_SERVER_TCP_RECEIVE_TIMEOUT             (5 * NX_SMTP_TICKS_PER_SECOND)  
#endif

/* Set Net TCP connection timeout in ticks.  */

#ifndef NX_SMTP_SERVER_CONNECTION_TIMEOUT               
#define NX_SMTP_SERVER_CONNECTION_TIMEOUT              NX_WAIT_FOREVER 
#endif


/* Set Net TCP disconnect timeout in ticks.  */

#ifndef NX_SMTP_SERVER_DISCONNECT_TIMEOUT            
#define NX_SMTP_SERVER_DISCONNECT_TIMEOUT              (10 * NX_SMTP_TICKS_PER_SECOND)  
#endif


/* Configure the SMTP server session parameters */

/* Set the number of SMTP server sessions handling concurrent client sessions.  */

#ifndef  NX_SMTP_MAX_SERVER_SESSIONS  
#define  NX_SMTP_MAX_SERVER_SESSIONS                  1 
#endif

/* Set SMTP session to require client authentication before receiving mail.  */
 
#ifndef NX_SMTP_SERVER_AUTHENTICATION_REQUIRED
#define NX_SMTP_SERVER_AUTHENTICATION_REQUIRED        NX_FALSE
#endif

/* Set the list of authentication types supported by the server*/

#ifndef NX_SMTP_SERVER_AUTHENTICATION_LIST
#define NX_SMTP_SERVER_AUTHENTICATION_LIST            "LOGIN"
#endif


/* Set server limit on number of mail transactions allowed per client session. 
   Set to zero for no limit on mail transactions per session.  */

#ifndef NX_SMTP_SERVER_SESSION_MAIL_LIMIT  
#define NX_SMTP_SERVER_SESSION_MAIL_LIMIT             10
#endif


/* Set server limit on number of recipients allowed per client mail transaction.  */
/* RFC 2821 specifies a recipient buffer of at least 100 recipients.  */

#ifndef NX_SMTP_SERVER_RECIPIENT_MAIL_LIMIT  
#define NX_SMTP_SERVER_RECIPIENT_MAIL_LIMIT           100
#endif


/* SERVER text message macros for standard protocol messages to SMTP Clients.
   The application code can substitute its own wording but the meaning should not be 
   significantly changed.  */

#ifndef NX_SMTP_SERVER_OK                              
#define NX_SMTP_SERVER_OK                             "OK"
#endif


/* Set the SMTP server greeting message.  */

#ifndef NX_SMTP_SERVER_CONNECTION_MESSAGE              
#define NX_SMTP_SERVER_CONNECTION_MESSAGE              "Simple Mail Transfer Service Ready."
#endif


/* Set the SMTP server list of services when responding to client EHLO command.  */     

#ifndef NX_SMTP_SERVER_SERVICES_MESSAGE      
#define NX_SMTP_SERVER_SERVICES_MESSAGE                "250 AUTH LOGIN\r\n" 
#endif


/* Set the SMTP server text when responding to client QUIT command.  */

#ifndef NX_SMTP_SERVER_CLOSE_CONNECTION_MESSAGE
#define NX_SMTP_SERVER_CLOSE_CONNECTION_MESSAGE        "SMTP Service closing transmission channel."
#endif


/* Set the SMTP server text when responding to client HELO command.  */

#ifndef NX_SMTP_SERVER_HELO_PROTOCOL
#define NX_SMTP_SERVER_HELO_PROTOCOL                   "Received HELO command; disabling SMTP protocol extensions."
#endif

/* Set the SMTP server text message for the following errors.  */

#ifndef NX_SMTP_SERVER_COMMAND_NOT_IMPLEMENTED
#define NX_SMTP_SERVER_COMMAND_NOT_IMPLEMENTED         "Command not implemented."
#endif

#ifndef NX_SMTP_SERVER_ACKNOWLEDGE_QUIT
#define NX_SMTP_SERVER_ACKNOWLEDGE_QUIT                "Received QUIT command; Service closing transmission channel."
#endif

#ifndef NX_SMTP_SERVER_REQUESTED_ACTION_NOT_TAKEN
#define NX_SMTP_SERVER_REQUESTED_ACTION_NOT_TAKEN      "Requested action not taken. "
#endif

#ifndef NX_SMTP_SERVER_BAD_SENDERS_MAILBOX_NAME
#define NX_SMTP_SERVER_BAD_SENDERS_MAILBOX_NAME        "Bad or illegal sender mailbox."
#endif

#ifndef NX_SMTP_SERVER_BAD_RECIPIENT_MAILBOX_NAME
#define NX_SMTP_SERVER_BAD_RECIPIENT_MAILBOX_NAME      "Bad, ambiguous or illegal recipient mailbox."
#endif

#ifndef NX_SMTP_SERVER_START_MAIL_INPUT
#define NX_SMTP_SERVER_START_MAIL_INPUT                "Start mail input. End with <CR><LF>.<CR><LF> "
#endif

#ifndef NX_SMTP_SERVER_BAD_SEQUENCE
#define NX_SMTP_SERVER_BAD_SEQUENCE                    "Bad sequence of commands."
#endif

#ifndef NX_SMTP_SERVER_COMMAND_SYNTAX_ERROR
#define NX_SMTP_SERVER_COMMAND_SYNTAX_ERROR            "Unknown command or syntax error."
#endif

#ifndef NX_SMTP_SERVER_PARAMETER_SYNTAX_ERROR
#define NX_SMTP_SERVER_PARAMETER_SYNTAX_ERROR          "Bad or missing command line parameters."
#endif

#ifndef NX_SMTP_SERVER_PARAMETER_NOT_IMPLEMENTED
#define NX_SMTP_SERVER_PARAMETER_NOT_IMPLEMENTED       "Command parameter not implemented."
#endif

#ifndef NX_SMTP_SERVER_TRANSACTION_FAILED
#define NX_SMTP_SERVER_TRANSACTION_FAILED              "Transaction failed; No data received."
#endif
    
#ifndef NX_SMTP_SERVER_INTERNAL_SERVER_ERROR
#define NX_SMTP_SERVER_INTERNAL_SERVER_ERROR           "Internal server error; no SMTP service available."
#endif

#ifndef NX_SMTP_SERVER_INSUFFICIENT_STORAGE
#define NX_SMTP_SERVER_INSUFFICIENT_STORAGE            "Insufficient storage available on the server."
#endif

#ifndef NX_SMTP_SERVER_SESSION_EXCEEDED_STORAGE_LIMIT
#define NX_SMTP_SERVER_SESSION_EXCEEDED_STORAGE_LIMIT  "Session has exceeded its storage limit."
#endif

#ifndef NX_SMTP_SERVER_AUTHENTICATION_SUCCESSFUL
#define NX_SMTP_SERVER_AUTHENTICATION_SUCCESSFUL       "Authentication successful!"
#endif


#ifndef NX_SMTP_SERVER_AUTHENTICATION_UNSUCCESSFUL
#define NX_SMTP_SERVER_AUTHENTICATION_UNSUCCESSFUL     "Authentication unsuccessful."
#endif

#ifndef NX_SMTP_SERVER_AUTH_INTERNAL_SERVER_ERROR
#define NX_SMTP_SERVER_AUTH_INTERNAL_SERVER_ERROR      "Authentication failed due to internal server error."
#endif

#ifndef NX_SMTP_SERVER_AUTH_PREVIOUS_ATTEMPT
#define NX_SMTP_SERVER_AUTH_PREVIOUS_ATTEMPT           "Session is already authenticated."
#endif

#ifndef NX_SMTP_SERVER_AUTH_REQUIRED
#define NX_SMTP_SERVER_AUTH_REQUIRED                   "Server requires authentication to deliver mail."
#endif

#ifndef NX_SMTP_SERVER_AUTH_CANCELLED
#define NX_SMTP_SERVER_AUTH_CANCELLED                  "Authentication failed. Server received * symbol; cancelling authentication."
#endif

#ifndef NX_SMTP_SERVER_SERVICE_NOT_AVAILABLE
#define NX_SMTP_SERVER_SERVICE_NOT_AVAILABLE           "SMTP service not available; closing transmission channel."
#endif

#ifndef NX_SMTP_SERVER_INVALID_PACKET_DATA
#define NX_SMTP_SERVER_INVALID_PACKET_DATA             "Client mail data cannot be extracted. Invalid packet(s) received."
#endif

#ifndef NX_SMTP_SERVER_OVERSIZE_MAIL_MESSAGE
#define NX_SMTP_SERVER_OVERSIZE_MAIL_MESSAGE           "Too much mail data."
#endif

#ifndef NX_SMTP_SERVER_TOO_MANY_SESSION_MAILS
#define NX_SMTP_SERVER_TOO_MANY_SESSION_MAILS          "Session over the limit on mail items "
#endif

#ifndef NX_SMTP_SERVER_TOO_MANY_RECIPIENTS
#define NX_SMTP_SERVER_TOO_MANY_RECIPIENTS             "Too many recipients!"
#endif


/* SMTP server reply codes in string format. Do not modify these! */

#define     NX_SMTP_TEXT_CODE_GREETING_OK                       "220" 
#define     NX_SMTP_TEXT_CODE_ACKNOWLEDGE_QUIT                  "221"
#define     NX_SMTP_TEXT_CODE_AUTHENTICATION_SUCCESSFUL         "235"
#define     NX_SMTP_TEXT_CODE_OK_TO_CONTINUE                    "250"
#define     NX_SMTP_TEXT_CODE_USER_NOT_LOCAL_WILL_FORWARD       "251"
#define     NX_SMTP_TEXT_CODE_CANNOT_VERIFY_RECIPIENT           "252"
#define     NX_SMTP_TEXT_CODE_AUTHENTICATION_TYPE_ACCEPTED      "334"
#define     NX_SMTP_TEXT_CODE_SEND_MAIL_INPUT                   "354"
#define     NX_SMTP_TEXT_CODE_SERVICE_NOT_AVAILABLE             "421"
#define     NX_SMTP_TEXT_CODE_SERVICE_INTERNAL_SERVER_ERROR     "451"
#define     NX_SMTP_TEXT_CODE_INSUFFICIENT_STORAGE              "452"
#define     NX_SMTP_TEXT_CODE_AUTH_FAILED_INTERNAL_SERVER_ERROR "454"
#define     NX_SMTP_TEXT_CODE_COMMAND_SYNTAX_ERROR              "500"
#define     NX_SMTP_TEXT_CODE_PARAMETER_SYNTAX_ERROR            "501"
#define     NX_SMTP_TEXT_CODE_COMMAND_NOT_IMPLEMENTED           "502"
#define     NX_SMTP_TEXT_CODE_BAD_SEQUENCE                      "503"
#define     NX_SMTP_TEXT_CODE_PARAMETER_NOT_IMPLEMENTED         "504"
#define     NX_SMTP_TEXT_CODE_AUTH_REQUIRED                     "530"
#define     NX_SMTP_TEXT_CODE_AUTH_FAILED                       "535"
#define     NX_SMTP_TEXT_CODE_REQUESTED_ACTION_NOT_TAKEN        "550"
#define     NX_SMTP_TEXT_CODE_USER_NOT_LOCAL                    "551" 
#define     NX_SMTP_TEXT_CODE_OVERSIZE_MAIL_MESSAGE             "552"
#define     NX_SMTP_TEXT_CODE_BAD_MAILBOX                       "553"
#define     NX_SMTP_TEXT_CODE_TRANSACTION_FAILED                "554"


/* Three digit server reply codes as bit fields so the server can prepare a bitmask
   of expected commands from the client.  */

#define     NX_SMTP_COMMAND_GREET_CODE          0x0000 
#define     NX_SMTP_COMMAND_RSET_CODE           0x0001 
#define     NX_SMTP_COMMAND_EHLO_CODE           0x0002 
#define     NX_SMTP_COMMAND_HELO_CODE           0x0004 
#define     NX_SMTP_COMMAND_MAIL_CODE           0x0008 
#define     NX_SMTP_COMMAND_RCPT_CODE           0x0010 
#define     NX_SMTP_COMMAND_DATA_CODE           0x0020 
#define     NX_SMTP_COMMAND_AUTH_CODE           0x0040
#define     NX_SMTP_COMMAND_NOOP_CODE           0x0080
#define     NX_SMTP_COMMAND_QUIT_CODE           0x0100 
#define     NX_SMTP_COMMAND_UNKNOWN_CODE        0xFFFF



/* Define the NetX SMTP Server Recipient structure */

typedef struct NX_SMTP_SERVER_RECIPIENT_STRUCT
{
    CHAR                                    *recipient_mailbox;        /* Pointer to recipient mailbox address  */
    struct NX_SMTP_SERVER_RECIPIENT_STRUCT  *next_ptr;                 /* Pointer to next recipient in mail item recipient list */

} NX_SMTP_SERVER_RECIPIENT;


/* Define the NetX SMTP Server Mail structure */

typedef struct NX_SMTP_SERVER_MAIL_STRUCT
{
    CHAR                                    *reverse_path_mailbox_ptr;   /* Pointer to sender mailbox (fully qualified domain name) */
    UINT                                    priority;                    /* Mail priority level  */
    UINT                                    valid_recipients;            /* Total number of recipients in this mail */
    NX_SMTP_SERVER_RECIPIENT                *start_recipient_ptr;        /* Start of mail item's recipient list.  */
    NX_SMTP_SERVER_RECIPIENT                *current_recipient_ptr;      /* Current recipient in session mail transaction */
    NX_SMTP_SERVER_RECIPIENT                *end_recipient_ptr;          /* End of mail item's recipient list.  */    
    NX_SMTP_MESSAGE_SEGMENT                 *start_message_segment_ptr;  /* Pointer to first segment of mail message.  */
    NX_SMTP_MESSAGE_SEGMENT                 *current_message_segment_ptr;/* Pointer to current segment of mail message.  */
    NX_SMTP_MESSAGE_SEGMENT                 *end_message_segment_ptr;    /* Pointer to last segment of mail message.  */
    struct NX_SMTP_SERVER_MAIL_STRUCT       *previous_ptr;               /* Pointer to the previous mail in the mail item's session.  */
    struct NX_SMTP_SERVER_MAIL_STRUCT       *next_ptr;                   /* Pointer to the next mail in the mail item's session.  */
    struct NX_SMTP_SERVER_SESSION_STRUCT    *session_ptr;                /* Pointer to mail item's session.  */
    struct NX_SMTP_SERVER_STRUCT            *server_ptr;                 /* Pointer to mail item's server.  */
    ULONG                                   message_length;              /* Length of mail message.  */
    UINT                                    valid_mail_transaction;      /* Status if mail transaction successful.  */
    UINT                                    marked_for_deletion;         /* Status if mail ready for deletion */

} NX_SMTP_SERVER_MAIL;


/* Define the SMTP Server Session structure.  */

typedef struct NX_SMTP_SERVER_SESSION_STRUCT
{
    UINT                                    session_id;                     /* Unique Session ID  */
    ULONG                                   nx_smtp_server_session_id;      /* SMTP ID identifying session thread as ready to run */
    struct NX_SMTP_SERVER_STRUCT            *server_ptr;                    /* Pointer to the SMTP server.  */
    struct NX_SMTP_SERVER_SESSION_STRUCT    *next_ptr;                      /* Pointer to server's next session */
    NX_SMTP_SERVER_MAIL                     *start_mail_ptr;                /* Pointer to the start of session mail list.  */
    NX_SMTP_SERVER_MAIL                     *end_mail_ptr;                  /* Pointer to the end of session mail list.  */
    NX_SMTP_SERVER_MAIL                     *current_mail_ptr;              /* Pointer to the current session mail.  */
    NX_TCP_SOCKET                           session_socket;                 /* Session TCP socket */
    TX_THREAD                               session_thread;                 /* SMTP server session thread */
    UINT                                    connection_pending;             /* Connection pending flag  */
    UINT                                    session_mid_transaction;        /* Status of session currently handling mail transaction (vs being idle).  */
    UINT                                    session_closed;                 /* Status for session being closed to any more client connections */
    ULONG                                   session_connection_attempts;    /* Count of client SMTP requests detected.  */
    ULONG                                   session_connection_failures;    /* Count of client connection accept failures.  */
    ULONG                                   session_disconnection_requests; /* Count of client disconnections without properly quitting the session.  */
    UINT                                    cmd_expected_state;             /* Expected command set for server state */
    NX_PACKET                               *session_packet_ptr;            /* Pointer to packet received during server session */
    ULONG                                   session_mail_transaction_attempts;  /* Number of mail transactions attempted in a session */
    ULONG                                   session_valid_mail_transactions;/* Number of valid mail transactions in a session */
    UINT                                    authentication_state;           /* State of session authentication.  */

} NX_SMTP_SERVER_SESSION;

/* Define the SMTP Server structure.  */

typedef struct NX_SMTP_SERVER_STRUCT
{
    ULONG                                   nx_smtp_server_id;               /* NetX SMTP Server thread ID  */
    NX_IP                                   *server_ip_ptr;                  /* Pointer to associated NetX IP structure */
    NX_PACKET_POOL                          *server_packet_pool_ptr;         /* Pointer to NetX server packet pool */
    UINT                                    server_ip_address;               /* Server IP address */
    USHORT                                  server_port;                     /* Server listen port */
    UINT                                    session_connection_in_progress;  /* Status if server needs to set up next session for client request */
    ULONG                                   server_connection_attempts;      /* Number of attempted connections */
    ULONG                                   server_connection_failures;      /* Number of failed connections  */
    ULONG                                   packet_allocation_errors;        /* Number of packet allocation errors */
    ULONG                                   server_relisten_errors;          /* Number of failed relisten requests */
    ULONG                                   server_disconnection_requests;   /* Number of broken connections */
    TX_THREAD                               server_thread;                   /* SMTP server thread */
    UINT                                    server_closed;                   /* Status if server closed to any further client connections */
    TX_MUTEX                                print_summary_mutex;             /* Server print summary mutex */
    UINT                                    print_summary_mutex_ptr_timeout; /* Timeout value for print summary mutex */
    NX_SMTP_SERVER_SESSION                  nx_smtp_server_session_list[NX_SMTP_MAX_SERVER_SESSIONS]; 
                                                                             /* SMTP server session array  */
    TX_BLOCK_POOL                           *blockpool_ptr;                  /* Pointer to server block pool */
    TX_MUTEX                                *blockpool_mutex_ptr;            /* Pointer to server block pool mutex */
    UINT                                    blockpool_mutex_timeout;         /* Timeout value for block pool mutex */
    TX_BYTE_POOL                            *bytepool_ptr;                   /* Pointer to server byte pool */
    TX_MUTEX                                *bytepool_mutex_ptr;             /* Pointer to server byte pool mutex */
    UINT                                    bytepool_mutex_timeout;          /* Timeout value for byte pool mutex */
    UINT                                    authentication_required;         /* Flag for required session authentication */
    CHAR                                    *authentication_list;            /* Pointer to list of authentication types supported by server */
    UINT                                    (*server_authentication_check)(CHAR *name, CHAR *password,UINT *result);
                                                                             /* Pointer to sender authentication check function */
    UINT                                    (*nx_smtp_server_mail_spooler)(NX_SMTP_SERVER_MAIL *mail_ptr);  
                                                                             /* Pointer to mail spooler function */

} NX_SMTP_SERVER;



#ifndef     NX_SMTP_SOURCE_CODE     


/* Define the system API mappings based on the error checking 
   selected by the user.   */

/* Determine if error checking is desired.  If so, map API functions
   to the appropriate error checking front-ends.  Otherwise, map API
   functions to the core functions that actually perform the work.
   Note: error checking is enabled by default.  */


#ifdef NX_SMTP_DISABLE_ERROR_CHECKING

/* Services without error checking.  */

#define nx_smtp_server_create                   _nx_smtp_server_create
#define nx_smtp_server_delete                   _nx_smtp_server_delete
#define nx_smtp_server_start                    _nx_smtp_server_start
#define nx_smtp_server_stop                     _nx_smtp_server_stop
#define nx_smtp_server_session_create           _nx_smtp_server_session_create
#define nx_smtp_server_session_delete           _nx_smtp_server_session_delete
#define nx_smtp_server_session_run              _nx_smtp_server_session_run
#define nx_smtp_server_mail_create              _nx_smtp_server_mail_create 
#define nx_smtp_server_mail_add                 _nx_smtp_server_mail_add 
#define nx_smtp_server_mail_delete              _nx_smtp_server_mail_delete
#define nx_smtp_server_recipient_create         _nx_smtp_server_recipient_create
#define nx_smtp_server_recipient_add            _nx_smtp_server_recipient_add
#define nx_smtp_server_recipient_delete         _nx_smtp_server_recipient_delete
#define nx_smtp_server_message_segment_add      _nx_smtp_server_message_segment_add
#define nx_smtp_reply_to_greeting               _nx_smtp_reply_to_greeting
#define nx_smtp_reply_to_helo                   _nx_smtp_reply_to_helo
#define nx_smtp_reply_to_ehlo                   _nx_smtp_reply_to_ehlo
#define nx_smtp_reply_to_auth                   _nx_smtp_reply_to_auth
#define nx_smtp_reply_to_mail                   _nx_smtp_reply_to_mail
#define nx_smtp_reply_to_rcpt                   _nx_smtp_reply_to_rcpt
#define nx_smtp_reply_to_data                   _nx_smtp_reply_to_data
#define nx_smtp_reply_to_message                _nx_smtp_reply_to_message
#define nx_smtp_reply_to_rset                   _nx_smtp_reply_to_rset
#define nx_smtp_reply_to_quit                   _nx_smtp_reply_to_quit
#define nx_smtp_reply_to_noop                   _nx_smtp_reply_to_noop
#define nx_smtp_server_bytepool_memory_get      _nx_smtp_server_bytepool_memory_get
#define nx_smtp_server_bytepool_memory_release  _nx_smtp_server_bytepool_memory_release 
#define nx_smtp_server_blockpool_memory_get     _nx_smtp_server_blockpool_memory_get
#define nx_smtp_server_blockpool_memory_release _nx_smtp_server_blockpool_memory_release 
#define nx_smtp_utility_send_server_response    _nx_smtp_utility_send_server_response
#define nx_smtp_utility_parse_client_request    _nx_smtp_utility_parse_client_request
#define nx_smtp_utility_parse_mailbox_address   _nx_smtp_utility_parse_mailbox_address
#define nx_smtp_utility_login_authenticate      _nx_smtp_utility_login_authenticate
#define nx_smtp_utility_spool_session_mail      _nx_smtp_utility_spool_session_mail
#define nx_smtp_utility_clear_session_mail      _nx_smtp_utility_clear_session_mail
#define nx_smtp_utility_print_server_mail       _nx_smtp_utility_print_server_mail
#define nx_smtp_utility_print_server_reserves   _nx_smtp_utility_print_server_reserves
#define nx_smtp_utility_packet_data_extract     _nx_smtp_utility_packet_data_extract

#else

/* Services with error checking.  */

#define nx_smtp_server_create                   _nxe_smtp_server_create
#define nx_smtp_server_delete                   _nxe_smtp_server_delete
#define nx_smtp_server_start                    _nxe_smtp_server_start
#define nx_smtp_server_stop                     _nxe_smtp_server_stop
#define nx_smtp_server_session_create           _nxe_smtp_server_session_create
#define nx_smtp_server_session_delete           _nxe_smtp_server_session_delete
#define nx_smtp_server_session_run              _nxe_smtp_server_session_run
#define nx_smtp_server_mail_create              _nxe_smtp_server_mail_create
#define nx_smtp_server_mail_add                 _nxe_smtp_server_mail_add
#define nx_smtp_server_mail_delete              _nxe_smtp_server_mail_delete
#define nx_smtp_server_recipient_create         _nxe_smtp_server_recipient_create
#define nx_smtp_server_recipient_add            _nxe_smtp_server_recipient_add
#define nx_smtp_server_recipient_delete         _nxe_smtp_server_recipient_delete
#define nx_smtp_server_message_segment_add      _nxe_smtp_server_message_segment_add
#define nx_smtp_reply_to_greeting               _nxe_smtp_reply_to_greeting
#define nx_smtp_reply_to_helo                   _nxe_smtp_reply_to_helo
#define nx_smtp_reply_to_ehlo                   _nxe_smtp_reply_to_ehlo
#define nx_smtp_reply_to_auth                   _nxe_smtp_reply_to_auth
#define nx_smtp_reply_to_mail                   _nxe_smtp_reply_to_mail
#define nx_smtp_reply_to_rcpt                   _nxe_smtp_reply_to_rcpt
#define nx_smtp_reply_to_data                   _nxe_smtp_reply_to_data
#define nx_smtp_reply_to_message                _nxe_smtp_reply_to_message
#define nx_smtp_reply_to_rset                   _nxe_smtp_reply_to_rset
#define nx_smtp_reply_to_quit                   _nxe_smtp_reply_to_quit
#define nx_smtp_reply_to_noop                   _nxe_smtp_reply_to_noop
#define nx_smtp_server_bytepool_memory_get      _nxe_smtp_server_bytepool_memory_get
#define nx_smtp_server_bytepool_memory_release  _nxe_smtp_server_bytepool_memory_release 
#define nx_smtp_server_blockpool_memory_get     _nxe_smtp_server_blockpool_memory_get
#define nx_smtp_server_blockpool_memory_release _nxe_smtp_server_blockpool_memory_release 
#define nx_smtp_utility_send_server_response    _nxe_smtp_utility_send_server_response
#define nx_smtp_utility_parse_client_request    _nxe_smtp_utility_parse_client_request
#define nx_smtp_utility_login_authenticate      _nxe_smtp_utility_login_authenticate
#define nx_smtp_utility_parse_mailbox_address   _nxe_smtp_utility_parse_mailbox_address
#define nx_smtp_utility_spool_session_mail      _nxe_smtp_utility_spool_session_mail
#define nx_smtp_utility_clear_session_mail      _nxe_smtp_utility_clear_session_mail
#define nx_smtp_utility_print_server_mail       _nxe_smtp_utility_print_server_mail
#define nx_smtp_utility_print_server_reserves   _nxe_smtp_utility_print_server_reserves
#define nx_smtp_utility_packet_data_extract     _nxe_smtp_utility_packet_data_extract

#endif    /* NX_SMTP_DISABLE_ERROR_CHECKING */

UINT    nx_smtp_server_create(NX_SMTP_SERVER *server_ptr, CHAR *authentication_list, NX_IP *ip_ptr, VOID *stack_ptr, 
            ULONG stack_size, UINT server_priority, UINT server_preempt_threshold, UINT server_time_slice, UINT auto_start,
            NX_PACKET_POOL *packet_pool_ptr, UINT (*authentication_check)(CHAR *username, CHAR *password, UINT *result),
            UINT (*mail_spooler)(NX_SMTP_SERVER_MAIL *mail_ptr), TX_BLOCK_POOL *blockpool_ptr, TX_MUTEX *blockpool_mutex_ptr, 
            UINT blockpool_mutex_timeout, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *bytepool_mutex_ptr, UINT bytepool_mutex_timeout); 
UINT    nx_smtp_server_delete(NX_SMTP_SERVER *server_ptr);
UINT    nx_smtp_server_start(NX_SMTP_SERVER *server_ptr);
UINT    nx_smtp_server_stop(NX_SMTP_SERVER *server_ptr);
UINT    nx_smtp_server_session_create(NX_SMTP_SERVER *server_ptr, NX_SMTP_SERVER_SESSION *session_ptr, UINT session_id, 
                                      VOID *session_stack_ptr, ULONG session_stack_size, UINT session_priority, 
                                      UINT session_preempt_threshold, ULONG session_time_slice, UINT session_auto_start); 
UINT    nx_smtp_server_session_delete(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    nx_smtp_server_session_run(NX_SMTP_SERVER_SESSION  *session_ptr);
UINT    nx_smtp_server_mail_create(NX_SMTP_SERVER_SESSION *session_ptr, NX_SMTP_SERVER_MAIL **mail_ptr, CHAR *sender_mailbox, UINT priority);
UINT    nx_smtp_server_mail_add(NX_SMTP_SERVER_SESSION *session_ptr, NX_SMTP_SERVER_MAIL *mail_ptr);
UINT    nx_smtp_server_mail_delete(NX_SMTP_SERVER_SESSION *session_ptr, NX_SMTP_SERVER_MAIL *mail_ptr);
UINT    nx_smtp_server_recipient_create(NX_SMTP_SERVER_SESSION *session_ptr, NX_SMTP_SERVER_RECIPIENT **recipient_ptr, CHAR *recipient_mailbox);
UINT    nx_smtp_server_recipient_add(NX_SMTP_SERVER_MAIL *mail_ptr, NX_SMTP_SERVER_RECIPIENT *recipient_ptr);
UINT    nx_smtp_server_recipient_delete(NX_SMTP_SERVER_MAIL *mail_ptr, NX_SMTP_SERVER_RECIPIENT  *recipient_ptr);
UINT    nx_smtp_server_message_segment_add(NX_SMTP_SERVER_MAIL *mail_ptr, NX_SMTP_MESSAGE_SEGMENT *message_segment_ptr);
UINT    nx_smtp_reply_to_greeting(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    nx_smtp_reply_to_helo(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    nx_smtp_reply_to_ehlo(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    nx_smtp_reply_to_auth(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    nx_smtp_reply_to_mail(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    nx_smtp_reply_to_rcpt(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    nx_smtp_reply_to_data(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    nx_smtp_reply_to_message(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    nx_smtp_reply_to_rset(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    nx_smtp_reply_to_quit(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    nx_smtp_reply_to_noop(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    nx_smtp_server_bytepool_memory_get(VOID **memory_ptr, ULONG memory_size, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);
UINT    nx_smtp_server_bytepool_memory_release(VOID *memory_ptr, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);
UINT    nx_smtp_server_blockpool_memory_get(VOID **memory_ptr, TX_BLOCK_POOL *blockpool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);
UINT    nx_smtp_server_blockpool_memory_release(VOID *memory_ptr, TX_BLOCK_POOL *blockpool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);
UINT    nx_smtp_utility_send_server_response(NX_SMTP_SERVER_SESSION *session_ptr, CHAR *header, CHAR *info, CHAR *more_info, UINT more_lines_to_follow);
UINT    nx_smtp_utility_parse_client_request(CHAR *buffer_ptr, UINT *protocol_code, UINT length);
UINT    nx_smtp_utility_parse_mailbox_address(CHAR *start_buffer, CHAR *end_buffer, CHAR **mailbox_address, UINT addr_is_parameter);
UINT    nx_smtp_utility_login_authenticate(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    nx_smtp_utility_spool_session_mail(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    nx_smtp_utility_clear_session_mail(NX_SMTP_SERVER_SESSION *session_ptr);                                 
UINT    nx_smtp_utility_print_server_mail(NX_SMTP_SERVER_MAIL *mail_ptr);
UINT    nx_smtp_utility_print_server_reserves(NX_SMTP_SERVER *server_ptr);
UINT    nx_smtp_utility_packet_data_extract(NX_PACKET *packet_ptr, VOID *buffer_start, ULONG buffer_length, ULONG *bytes_copied);


#else     /* NX_SMTP_SOURCE_CODE */

/* SMTP source code is being compiled, do not perform any API mapping.  */

UINT    _nxe_smtp_server_create(NX_SMTP_SERVER *server_ptr, CHAR *authentication_list, NX_IP *ip_ptr, VOID *stack_ptr, 
            ULONG stack_size, UINT server_priority, UINT server_preempt_threshold, UINT server_time_slice, UINT auto_start,
            NX_PACKET_POOL *packet_pool_ptr, UINT (*authentication_check)(CHAR *username, CHAR *password, UINT *result),
            UINT (*mail_spooler)(NX_SMTP_SERVER_MAIL *mail_ptr), TX_BLOCK_POOL *blockpool_ptr, TX_MUTEX *blockpool_mutex_ptr, 
            UINT blockpool_mutex_timeout, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *bytepool_mutex_ptr, UINT bytepool_mutex_timeout);
UINT    _nx_smtp_server_create(NX_SMTP_SERVER *server_ptr, CHAR *authentication_list, NX_IP *ip_ptr, VOID *stack_ptr, 
            ULONG stack_size, UINT server_priority, UINT server_preempt_threshold, UINT server_time_slice, UINT auto_start,
            NX_PACKET_POOL *packet_pool_ptr, UINT (*authentication_check)(CHAR *username, CHAR *password, UINT *result),
            UINT (*mail_spooler)(NX_SMTP_SERVER_MAIL *mail_ptr), TX_BLOCK_POOL *blockpool_ptr, TX_MUTEX *blockpool_mutex_ptr, 
            UINT blockpool_mutex_timeout, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *bytepool_mutex_ptr, UINT bytepool_mutex_timeout); 
UINT    _nxe_smtp_server_delete(NX_SMTP_SERVER *server_ptr); 
UINT    _nx_smtp_server_delete(NX_SMTP_SERVER *server_ptr);
UINT    _nxe_smtp_server_start(NX_SMTP_SERVER *server_ptr);
UINT    _nx_smtp_server_start(NX_SMTP_SERVER *server_ptr);
UINT    _nxe_smtp_server_stop(NX_SMTP_SERVER *server_ptr);
UINT    _nx_smtp_server_stop(NX_SMTP_SERVER *server_ptr);
UINT    _nxe_smtp_server_session_create(NX_SMTP_SERVER *server_ptr, NX_SMTP_SERVER_SESSION *session_ptr, UINT session_id, 
                                      VOID *session_stack_ptr, ULONG session_stack_size, UINT session_priority, 
                                      UINT session_preempt_threshold, ULONG session_time_slice, UINT session_auto_start); 
UINT    _nx_smtp_server_session_create(NX_SMTP_SERVER *server_ptr, NX_SMTP_SERVER_SESSION *session_ptr, UINT session_id, 
                                      VOID *session_stack_ptr, ULONG session_stack_size, UINT session_priority, 
                                      UINT session_preempt_threshold, ULONG session_time_slice, UINT session_auto_start); 
UINT    _nxe_smtp_server_session_delete(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nx_smtp_server_session_delete(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nxe_smtp_server_session_run(NX_SMTP_SERVER_SESSION  *session_ptr);
UINT    _nx_smtp_server_session_run(NX_SMTP_SERVER_SESSION  *session_ptr);
UINT    _nxe_smtp_server_mail_create(NX_SMTP_SERVER_SESSION *session_ptr, NX_SMTP_SERVER_MAIL **mail_ptr, CHAR *sender_mailbox, UINT priority);
UINT    _nx_smtp_server_mail_create(NX_SMTP_SERVER_SESSION *session_ptr, NX_SMTP_SERVER_MAIL **mail_ptr, CHAR *sender_mailbox, UINT priority);
UINT    _nxe_smtp_server_mail_add(NX_SMTP_SERVER_SESSION *session_ptr, NX_SMTP_SERVER_MAIL *mail_ptr);
UINT    _nx_smtp_server_mail_add(NX_SMTP_SERVER_SESSION *session_ptr, NX_SMTP_SERVER_MAIL *mail_ptr);
UINT    _nxe_smtp_server_mail_delete(NX_SMTP_SERVER_SESSION *session_ptr, NX_SMTP_SERVER_MAIL *mail_ptr);
UINT    _nx_smtp_server_mail_delete(NX_SMTP_SERVER_SESSION *session_ptr, NX_SMTP_SERVER_MAIL *mail_ptr);
UINT    _nxe_smtp_server_recipient_create(NX_SMTP_SERVER_SESSION *session_ptr, NX_SMTP_SERVER_RECIPIENT **recipient_ptr, CHAR *recipient_mailbox);
UINT    _nx_smtp_server_recipient_create(NX_SMTP_SERVER_SESSION *session_ptr, NX_SMTP_SERVER_RECIPIENT **recipient_ptr, CHAR *recipient_mailbox);
UINT    _nxe_smtp_server_recipient_add(NX_SMTP_SERVER_MAIL *mail_ptr, NX_SMTP_SERVER_RECIPIENT *recipient_ptr);
UINT    _nx_smtp_server_recipient_add(NX_SMTP_SERVER_MAIL *mail_ptr, NX_SMTP_SERVER_RECIPIENT *recipient_ptr);
UINT    _nxe_smtp_server_recipient_delete(NX_SMTP_SERVER_MAIL *mail_ptr, NX_SMTP_SERVER_RECIPIENT  *recipient_ptr);
UINT    _nx_smtp_server_recipient_delete(NX_SMTP_SERVER_MAIL *mail_ptr, NX_SMTP_SERVER_RECIPIENT  *recipient_ptr);
UINT    _nxe_smtp_server_message_segment_add(NX_SMTP_SERVER_MAIL *mail_ptr, NX_SMTP_MESSAGE_SEGMENT *message_segment_ptr);
UINT    _nx_smtp_server_message_segment_add(NX_SMTP_SERVER_MAIL *mail_ptr, NX_SMTP_MESSAGE_SEGMENT *message_segment_ptr);
UINT    _nx_smtp_reply_to_greeting(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nxe_smtp_reply_to_greeting(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nx_smtp_reply_to_helo(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nxe_smtp_reply_to_ehlo(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nx_smtp_reply_to_ehlo(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nxe_smtp_reply_to_auth(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nx_smtp_reply_to_auth(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nxe_smtp_reply_to_mail(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nx_smtp_reply_to_mail(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nxe_smtp_reply_to_rcpt(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nx_smtp_reply_to_rcpt(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nxe_smtp_reply_to_data(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nx_smtp_reply_to_data(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nxe_smtp_reply_to_message(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nx_smtp_reply_to_message(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nxe_smtp_reply_to_rset(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nx_smtp_reply_to_rset(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nxe_smtp_reply_to_quit(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nx_smtp_reply_to_quit(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nxe_smtp_reply_to_noop(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nx_smtp_reply_to_noop(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nx_smtp_server_bytepool_memory_get(VOID **memory_ptr, ULONG memory_size, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);
UINT    _nxe_smtp_server_bytepool_memory_get(VOID **memory_ptr, ULONG memory_size, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);
UINT    _nx_smtp_server_bytepool_memory_release(VOID *memory_ptr, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);
UINT    _nxe_smtp_server_bytepool_memory_release(VOID *memory_ptr, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);
UINT    _nx_smtp_server_blockpool_memory_get(VOID **memory_ptr, TX_BLOCK_POOL *blockpool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);
UINT    _nxe_smtp_server_blockpool_memory_get(VOID **memory_ptr, TX_BLOCK_POOL *blockpool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);
UINT    _nx_smtp_server_blockpool_memory_release(VOID *memory_ptr, TX_BLOCK_POOL *blockpool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);
UINT    _nxe_smtp_server_blockpool_memory_release(VOID *memory_ptr, TX_BLOCK_POOL *blockpool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);
UINT    _nx_smtp_utility_send_server_response(NX_SMTP_SERVER_SESSION *session_ptr, const CHAR *header, const CHAR *info, const CHAR *more_info, UINT more_lines_to_follow);
UINT    _nxe_smtp_utility_send_server_response(NX_SMTP_SERVER_SESSION *session_ptr, CHAR *header, CHAR *info, CHAR *more_info, UINT more_lines_to_follow);
UINT    _nx_smtp_utility_parse_client_request(CHAR *buffer_ptr, UINT *protocol_code, UINT length);
UINT    _nxe_smtp_utility_parse_client_request(CHAR *buffer_ptr, UINT *protocol_code, UINT length);
UINT    _nx_smtp_utility_parse_mailbox_address(CHAR *start_buffer, CHAR *end_buffer, CHAR **mailbox_address, UINT addr_is_parameter);
UINT    _nxe_smtp_server_parse_mailbox_address(CHAR *start_buffer, CHAR *end_buffer, CHAR **mailbox_address, UINT addr_is_parameter);
UINT    _nx_smtp_utility_login_authenticate(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nxe_smtp_utility_login_authenticate(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nx_smtp_utility_spool_session_mail(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nxe_smtp_utility_spool_session_mail(NX_SMTP_SERVER_SESSION *session_ptr);
UINT    _nx_smtp_utility_clear_session_mail(NX_SMTP_SERVER_SESSION *session_ptr);                                 
UINT    _nxe_smtp_utility_clear_session_mail(NX_SMTP_SERVER_SESSION *session_ptr);                                 
UINT    _nxe_smtp_utility_print_server_mail(NX_SMTP_SERVER_MAIL *mail_ptr);
UINT    _nx_smtp_utility_print_server_mail(NX_SMTP_SERVER_MAIL *mail_ptr);
UINT    _nxe_smtp_utility_print_server_reserves(NX_SMTP_SERVER *server_ptr);
UINT    _nx_smtp_utility_print_server_reserves(NX_SMTP_SERVER *server_ptr);
UINT    _nx_smtp_utility_packet_data_extract(NX_PACKET *packet_ptr, VOID *buffer_start, ULONG buffer_length, ULONG *bytes_copied);
UINT    _nxe_smtp_utility_packet_data_extract(NX_PACKET *packet_ptr, VOID *buffer_start, ULONG buffer_length, ULONG *bytes_copied);

/* Define internal SMTP Server functions.  */

VOID    _nx_smtp_server_thread_entry(ULONG info);
VOID    _nx_smtp_session_connection_present(NX_TCP_SOCKET *socket_ptr, UINT port);
VOID    _nx_smtp_session_disconnect_present(NX_TCP_SOCKET *socket_ptr);
VOID    _nx_smtp_server_session_thread_entry(ULONG info);
VOID    _nx_smtp_server_parse_response(CHAR *buffer, UINT argument_index, UINT buffer_length, CHAR *arguement, UINT arguement_length, UINT convert_to_uppercase, UINT include_crlf);
VOID    _nx_smtp_server_find_crlf(CHAR *buffer, UINT length, CHAR **CRLF, UINT reverse);
VOID    _nx_smtp_server_base64_encode(const CHAR *name, CHAR *base64name);
VOID    _nx_smtp_server_base64_decode(const CHAR *base64name, CHAR *name);

#endif

/* If a C++ compiler is being used....*/
#ifdef   __cplusplus
        }
#endif

#endif /* NX_SMTP_SERVER_H */

