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
/** NetX POP3 Client Component                                            */
/**                                                                       */
/**   Post Office Protocol Version 3 (POP3)                               */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/**************************************************************************/
/*                                                                        */
/*  APPLICATION INTERFACE DEFINITION                       RELEASE        */
/*                                                                        */
/*    nx_pop3_client.h                                    PORTABLE C      */
/*                                                           5.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Janet Christiansen, Express Logic, Inc.                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This file defines the NetX Post Office Protocol Version 3 (POP3)    */
/*    Client component, including all data types and external references. */
/*    It is assumed that tx_api.h, tx_port.h, nx_api.h, nx_port.h,        */
/*    fx_api.h and fx_port.h have already been included.                  */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  09-21-2007     Janet Christiansen       Initial Version 5.0           */ 
/*  04-22-2010     Janet Christiansen       Modified comment(s),          */
/*                                          resulting in version 5.1      */
/*                                                                        */
/**************************************************************************/

#ifndef NX_POP3_CLIENT_H
#define NX_POP3_CLIENT_H


/* Determine if a C++ compiler is being used.  If so, ensure that standard
   C is used to process the API information.  */

#ifdef   __cplusplus

/* Yes, C++ compiler is present.  Use standard C.  */ 
extern   "C" {

#endif


#include    "nx_pop3.h"
#include    "nx_md5.h"


/* Configure the Client to use Client byte and block pool memory
   for storing downloaded mail items from the POP3 Server.  */

/* #define NX_POP3_CLIENT_DYNAMIC_MEMORY_ALLOC  */

/*  POP Debug levels in decreased filtering order:  
    NONE:   no events reported;
    LOG:    report events as part of normal logging operation.
    SEVERE: report events requiring session or server to stop operation.
    MODERATE: report events possibly preventing mail transaction or aborting a client session
    ALL:   all events reported
*/

/* Set the event reporting/debug output level for the NetX POP3 Client */

#ifndef NX_POP3_CLIENT_DEBUG
#define NX_POP3_CLIENT_DEBUG                     NX_POP3_DEBUG_LEVEL_NONE
#endif


/* Set debugging level for NetX POP3 */
#if ( NX_POP3_CLIENT_DEBUG == NX_POP3_DEBUG_LEVEL_NONE )
#define NX_POP3_CLIENT_EVENT_LOG(debug_level, msg)
#else
#define NX_POP3_CLIENT_EVENT_LOG(debug_level, msg)                  \
{                                                                   \
    UINT level = (UINT)debug_level;                                 \
    if (level <= NX_POP3_DEBUG_LEVEL_ALL && NX_POP3_CLIENT_DEBUG == NX_POP3_DEBUG_LEVEL_ALL)                \
    {                                                               \
       printf msg ;                                                 \
    }                                                               \
    else if (level <= NX_POP3_DEBUG_LEVEL_MODERATE && NX_POP3_CLIENT_DEBUG == NX_POP3_DEBUG_LEVEL_MODERATE) \
    {                                                               \
       printf msg ;                                                 \
    }                                                               \
    else if (level == NX_POP3_DEBUG_LEVEL_SEVERE && NX_POP3_CLIENT_DEBUG == NX_POP3_DEBUG_LEVEL_SEVERE)     \
    {                                                               \
       printf msg ;                                                 \
    }                                                               \
    else if (level == NX_POP3_DEBUG_LEVEL_LOG)                                          \
    {                                                               \
       printf msg ;                                                 \
    }                                                               \
}
#endif /* NX_POP3_CLIENT_DEBUG */

/* Enumerated states of the protocol state machine. */

typedef enum NX_POP3_CLIENT_SESSION_STATE_ENUM
{
    NX_POP3_CLIENT_SESSION_STATE_GREETING,  /* 0 */
    NX_POP3_CLIENT_SESSION_STATE_APOP,
    NX_POP3_CLIENT_SESSION_STATE_USER,
    NX_POP3_CLIENT_SESSION_STATE_PASS,
    NX_POP3_CLIENT_SESSION_STATE_STAT,
    NX_POP3_CLIENT_SESSION_STATE_RETR,      /* 5 */
    NX_POP3_CLIENT_SESSION_STATE_DELE,
    NX_POP3_CLIENT_SESSION_STATE_QUIT, 
    NX_POP3_CLIENT_SESSION_STATE_LIST,
    NX_POP3_CLIENT_SESSION_STATE_RSET,      
    NX_POP3_CLIENT_SESSION_STATE_NOOP       /* 10 */
} NX_POP3_CLIENT_SESSION_STATE;


/* Set the number of client POP3 sessions to run concurrently. */

#ifndef NX_POP3_CLIENT_SESSION_COUNT            
#define NX_POP3_CLIENT_SESSION_COUNT             1 
#endif


/* Set Client static buffer size for Client sessions not using Client
   byte and block pool dynamic memory allocation.  This implies the Client knows
   what size mail to expect.  Mail messages larger than this will
   not get downloaded in full. */

#ifndef NX_POP3_CLIENT_MAIL_BUFFER_SIZE
#define NX_POP3_CLIENT_MAIL_BUFFER_SIZE                 3000
#endif


#ifndef NX_POP3_CLIENT_THREAD_STACK_SIZE
#define NX_POP3_CLIENT_THREAD_STACK_SIZE                (1024 * 4)
#endif

#ifndef NX_POP3_CLIENT_THREAD_PRIORITY
#define NX_POP3_CLIENT_THREAD_PRIORITY                  2
#endif

#ifndef NX_POP3_CLIENT_PREEMPTION_THRESHOLD
#define NX_POP3_CLIENT_PREEMPTION_THRESHOLD             2
#endif

#ifndef NX_POP3_CLIENT_THREAD_TIME_SLICE
#define NX_POP3_CLIENT_THREAD_TIME_SLICE                TX_NO_TIME_SLICE
#endif

#ifndef NX_POP3_CLIENT_SESSION_THREAD_STACK_SIZE
#define NX_POP3_CLIENT_SESSION_THREAD_STACK_SIZE        (1024 * 4)
#endif

#ifndef NX_POP3_CLIENT_SESSION_THREAD_TIME_SLICE
#define NX_POP3_CLIENT_SESSION_THREAD_TIME_SLICE        TX_NO_TIME_SLICE
#endif

#ifndef NX_POP3_CLIENT_SESSION_THREAD_PRIORITY
#define NX_POP3_CLIENT_SESSION_THREAD_PRIORITY          NX_POP3_CLIENT_THREAD_PRIORITY
#endif

#ifndef NX_POP3_CLIENT_SESSION_PREEMPTION_THRESHOLD    
#define NX_POP3_CLIENT_SESSION_PREEMPTION_THRESHOLD     NX_POP3_CLIENT_SESSION_THREAD_PRIORITY
#endif

#ifndef NX_POP3_CLIENT_BYTE_POOL_SIZE
#define NX_POP3_CLIENT_BYTE_POOL_SIZE                   1024 * 2 
#endif

#ifndef NX_POP3_CLIENT_BYTE_POOL_NAME
#define NX_POP3_CLIENT_BYTE_POOL_NAME                   "Client bytepool"
#endif

#ifndef NX_POP3_CLIENT_BYTE_POOL_MUTEX_NAME
#define NX_POP3_CLIENT_BYTE_POOL_MUTEX_NAME             "Client bytepool mutex" 
#endif

#ifndef NX_POP3_CLIENT_BYTE_POOL_MUTEX_WAIT
#define NX_POP3_CLIENT_BYTE_POOL_MUTEX_WAIT             (5 * NX_POP3_TICKS_PER_SECOND) 
#endif

/* Client block pool block size should be at least the size of the packet 
   payload to facilitate transfer of data from packet buffers to block memory. */

#ifndef NX_POP3_CLIENT_BLOCK_SIZE   
#define NX_POP3_CLIENT_BLOCK_SIZE                       NX_POP3_CLIENT_PACKET_SIZE
#endif


#ifndef NX_POP3_CLIENT_BLOCK_POOL_SIZE          
#define NX_POP3_CLIENT_BLOCK_POOL_SIZE                  (16 * NX_POP3_CLIENT_BLOCK_SIZE)
#endif


#ifndef NX_POP3_CLIENT_BLOCK_POOL_NAME
#define NX_POP3_CLIENT_BLOCK_POOL_NAME                  "Client blockpool"
#endif

#ifndef NX_POP3_CLIENT_BLOCK_POOL_MUTEX_NAME
#define NX_POP3_CLIENT_BLOCK_POOL_MUTEX_NAME            "Client blockpool mutex" 
#endif


#ifndef NX_POP3_CLIENT_BLOCK_POOL_MUTEX_WAIT
#define NX_POP3_CLIENT_BLOCK_POOL_MUTEX_WAIT            (5 * NX_POP3_TICKS_PER_SECOND) 
#endif


#ifndef NX_POP3_CLIENT_PACKET_SIZE
#define NX_POP3_CLIENT_PACKET_SIZE                      1500
#endif

#ifndef NX_POP3_CLIENT_PACKET_POOL_SIZE     
#define NX_POP3_CLIENT_PACKET_POOL_SIZE                 (10 * NX_POP3_CLIENT_PACKET_SIZE)     
#endif

#ifndef NX_POP3_CLIENT_PACKET_TIMEOUT
#define NX_POP3_CLIENT_PACKET_TIMEOUT                   (1 * NX_POP3_TICKS_PER_SECOND)    
#endif

#ifndef NX_POP3_TCP_SOCKET_SEND_WAIT     
#define NX_POP3_TCP_SOCKET_SEND_WAIT                    (2 * NX_POP3_TICKS_PER_SECOND)
#endif

/* Set timeout (seconds) for receiving Server reply. */

#ifndef NX_POP3_CLIENT_REPLY_TIMEOUT
#define NX_POP3_CLIENT_REPLY_TIMEOUT                    (10 * NX_POP3_TICKS_PER_SECOND)
#endif

#ifndef NX_POP3_CLIENT_CONNECTION_TIMEOUT
#define NX_POP3_CLIENT_CONNECTION_TIMEOUT               (30  * NX_POP3_TICKS_PER_SECOND)
#endif

#ifndef NX_POP3_CLIENT_DISCONNECT_TIMEOUT
#define NX_POP3_CLIENT_DISCONNECT_TIMEOUT               (2 * NX_POP3_TICKS_PER_SECOND)
#endif

#ifndef NX_POP3_CLIENT_TCP_SOCKET_NAME         
#define NX_POP3_CLIENT_TCP_SOCKET_NAME                  "POP3 Client socket"              
#endif

#ifndef NX_POP3_SERVER_PORT
#define NX_POP3_SERVER_PORT                             110    
#endif

#ifndef NX_POP3_CLIENT_IPADR        
#define NX_POP3_CLIENT_IPADR                            (IP_ADDRESS(192, 2, 2, 34))    
#endif

#ifndef NX_POP3_CLIENT_IP_THREAD_STACK_SIZE
#define NX_POP3_CLIENT_IP_THREAD_STACK_SIZE             1024 
#endif

#ifndef NX_POP3_CLIENT_IP_THREAD_PRIORITY
#define NX_POP3_CLIENT_IP_THREAD_PRIORITY               1 
#endif

#ifndef NX_POP3_CLIENT_ARP_CACHE_SIZE 
#define NX_POP3_CLIENT_ARP_CACHE_SIZE                   1040 
#endif

/* Set TCP window size (maximum number of bytes in socket receive queue). */

#ifndef NX_POP3_CLIENT_WINDOW_SIZE    
#define NX_POP3_CLIENT_WINDOW_SIZE                      1500
#endif

#ifndef NX_POP3_MAX_USERNAME
#define NX_POP3_MAX_USERNAME                            40
#endif

#ifndef NX_POP3_MAX_PASSWORD
#define NX_POP3_MAX_PASSWORD                            20
#endif

#ifndef NX_POP3_MAX_SHARED_SECRET
#define NX_POP3_MAX_SHARED_SECRET                            20
#endif


/* Set Client to ask Server to delete mail after successful download. 
   The RFC 1939 strongly recommends this to minimize server resources. */

#ifndef NX_POP3_CLIENT_DELETE_MAIL_ON_SERVER
#define NX_POP3_CLIENT_DELETE_MAIL_ON_SERVER            NX_TRUE
#endif


/* Internal defines for the session state machine. */
                                                       
#define   NX_POP3_CLIENT_SESSION_STATE_AWAITING_REPLY       -1     /* Session state depends on outcome of next response handler. */
#define   NX_POP3_CLIENT_SESSION_STATE_COMPLETED_NORMALLY   -2     /* No internal errors, session completed normally. */
#define   NX_POP3_CLIENT_SESSION_STATE_ERROR                -3     /* Internal errors e.g. TCP send or receive fails; session terminated abnormally. */



/* Define the NetX POP3 Message Segment structure. */

typedef struct NX_POP3_MESSAGE_SEGMENT_STRUCT
{
    CHAR                                  *message_ptr;            /* Pointer to message segment data. */
    ULONG                                  message_segment_length; /* Size of message segment. */
    struct NX_POP3_MESSAGE_SEGMENT_STRUCT *next_ptr;               /* Pointer to next message segment. */
                                                                     
} NX_POP3_MESSAGE_SEGMENT;



/* Define the NetX POP3 MAIL structure */

typedef struct NX_POP3_CLIENT_MAIL_STRUCT
{
    struct NX_POP3_CLIENT_SESSION_STRUCT    *session_ptr;                /* Session to which the mail is attached.  */
    struct NX_POP3_CLIENT_MAIL_STRUCT       *next_ptr;                   /* Pointer to the next mail in the client session. */
    NX_POP3_MESSAGE_SEGMENT                 *start_message_segment_ptr;  /* Pointer to first segment of mail message. */
    NX_POP3_MESSAGE_SEGMENT                 *current_message_segment_ptr;/* Pointer to current segment of mail message. */
    NX_POP3_MESSAGE_SEGMENT                 *end_message_segment_ptr;    /* Pointer to last segment of mail message. */

    /* NOT applicable to POP3 Clients using dynamic memory allocation (Client block pools and message structs). */
    CHAR                                    *mail_buffer_ptr;            /* Pointer to text of mail to send. */
    UINT                                    mail_buffer_length;          /* Size of mail buffer. */


} NX_POP3_CLIENT_MAIL;


/* Define the NetX POP3 SESSION structure  */

typedef struct NX_POP3_CLIENT_SESSION_STRUCT
{

    struct NX_POP3_CLIENT_STRUCT            *client_ptr;                  /* Client to which the session is attached. */    
    ULONG                                   session_id;                   /* Unique session identifier.  */
    UINT                                    server_ip_address;            /* Server IP address in Big Endian format.  */
    USHORT                                  server_port;                  /* Server port in Big Endian format.  */
    NX_TCP_SOCKET                           tcp_socket;                   /* NetX TCP client socket control block.  */
    TX_THREAD                               session_thread;               /* POP3 client session thread */
    UINT                                    available;                    /* Set session availability for a POP3 request. */
    UINT                                    delete_downloaded_mail;       /* Set Client to request server to delete Client mail after download. */
    struct NX_POP3_CLIENT_MAIL_STRUCT       *start_mail_ptr;              /* Start of mails in the current client session.  */
    struct NX_POP3_CLIENT_MAIL_STRUCT       *end_mail_ptr;                /* End of mails in the current client session.  */
    struct NX_POP3_CLIENT_MAIL_STRUCT       *current_mail_ptr;            /* Current mail being processed in the session.  */
    CHAR                                    server_process_ID[NX_POP3_SERVER_PROCESS_ID];
                                                                          /* Server process ID received by session from server greeting. */
    NX_PACKET                               *session_packet_ptr;          /* Pointer to packet received during server session */
    INT                                     cmd_state;                    /* Command state of the POP3 protocol */
    INT                                     rsp_state;                    /* Response state of the POP3 protocol */
    CHAR                                    reply_buffer[NX_POP3_MAX_SERVER_REPLY];      
                                                                          /* Text of server reply. */
    UINT                                    POP3_session_state;           /* State of the POP3 session: Authorization, Update or TRANSACTION. */
    UINT                                    maildrop_items;               /* Number of mail messages waiting in client (user) maildrop. */
    UINT                                    maildrop_index;               /* Message number of mail in maildrop to list, retrieve, or delete. */
    ULONG                                   total_message_data;           /* Message data in bytes sitting in client (user) maildrop. */
    ULONG                                   mailitem_message_data;        /* Message data of a single mail item in Client maildrop. */

} NX_POP3_CLIENT_SESSION;


/* Define the POP3 Client structure  */

typedef struct NX_POP3_CLIENT_STRUCT
{
    ULONG                           nx_pop3_client_id;                       /* POP3 ID for identify client service type to server  */
    CHAR                            client_name[NX_POP3_MAX_USERNAME];       /* Client name (also used in authentication) */
    CHAR                            client_password[NX_POP3_MAX_PASSWORD];   /* Client password for authentication */
    CHAR                            client_shared_secret[NX_POP3_MAX_SHARED_SECRET];   
                                                                             /* Client shared secret for APOP authentication */
    UINT                            enable_APOP_authentication;              /* Enable client for APOP authentication instead of USER/PASS. */
    UINT                            enable_user_pass_authentication;         /* Enable client to use USER/PASS (e.g. if APOP fails) */
    NX_MD5                          client_md5data;                          /* POP3 Client MD5 work area            */
    NX_POP3_CLIENT_SESSION          nx_pop3_client_session_list[NX_POP3_CLIENT_SESSION_COUNT]; 
                                                                             /* POP3 client session array            */
    NX_IP                          *ip_ptr;                                  /* Client IP instance  */
    NX_PACKET_POOL                 *packet_pool_ptr;                         /* packet pool for allocating packets for data transfer */
    ULONG                           reply_timeout;                           /* Time out (seconds) for receiving server reply. */
    ULONG                           tcp_window_size;                         /* TCP window size                                            */
    TX_BLOCK_POOL                  *blockpool_ptr;                           /* Pointer to client block pool. */
    TX_MUTEX                       *blockpool_mutex_ptr;                     /* Pointer to client block pool mutex. */
    ULONG                           blockpool_mutex_timeout;                 /* Timeout value for block pool mutex. */
    TX_BYTE_POOL                   *bytepool_ptr;                            /* Pointer to client byte pool. */
    TX_MUTEX                       *bytepool_mutex_ptr;                      /* Pointer to client byte pool mutex. */
    ULONG                           bytepool_mutex_timeout;                  /* Timeout value for byte pool mutex. */
    UINT                            (*nx_pop3_client_mail_spooler)(NX_POP3_CLIENT_MAIL *mail_ptr);  
                                                                             /* Pointer to mail spooler function. */

} NX_POP3_CLIENT;

/* Structure for holding command/response handlers.  */

typedef struct NX_POP3_CLIENT_SESSION_STATES_STRUCT
{
    UINT    (*cmd) (struct NX_POP3_CLIENT_SESSION_STRUCT *session_ptr);
    UINT    (*rsp) (struct NX_POP3_CLIENT_SESSION_STRUCT *session_ptr);
} NX_POP3_CLIENT_SESSION_STATES;



#ifndef NX_POP3_SOURCE_CODE     

#ifdef NX_DISABLE_ERROR_CHECKING

/* Services without error checking.  */


#define   nx_pop3_client_connect                    _nx_pop3_client_connect
#define   nx_pop3_client_create                     _nx_pop3_client_create
#define   nx_pop3_client_delete                     _nx_pop3_client_delete
#define   nx_pop3_cmd_apop                          _nxe_pop3_cmd_apop
#define   nx_pop3_cmd_dele                          _nx_pop3_cmd_dele
#define   nx_pop3_cmd_greeting                      _nx_pop3_cmd_greeting
#define   nx_pop3_cmd_list                          _nx_pop3_cmd_list
#define   nx_pop3_cmd_noop                          _nx_pop3_cmd_noop
#define   nx_pop3_cmd_pass                          _nx_pop3_cmd_pass
#define   nx_pop3_cmd_retr                          _nx_pop3_cmd_retr
#define   nx_pop3_cmd_quit                          _nx_pop3_cmd_quit
#define   nx_pop3_cmd_rset                          _nx_pop3_cmd_rset
#define   nx_pop3_cmd_stat                          _nx_pop3_cmd_stat
#define   nx_pop3_cmd_user                          _nx_pop3_cmd_user
#define   nx_pop3_mail_add                          _nx_pop3_mail_add
#define   nx_pop3_mail_create                       _nx_pop3_mail_create
#define   nx_pop3_mail_delete                       _nx_pop3_mail_delete
#define   nx_pop3_mail_spool                        _nx_pop3_mail_spool
#define   nx_pop3_rsp_apop                          _nx_pop3_rsp_apop
#define   nx_pop3_rsp_dele                          _nx_pop3_rsp_dele
#define   nx_pop3_rsp_greeting                      _nx_pop3_rsp_greeting
#define   nx_pop3_rsp_list                          _nx_pop3_rsp_list
#define   nx_pop3_rsp_noop                          _nx_pop3_rsp_noop   
#define   nx_pop3_rsp_pass                          _nx_pop3_rsp_pass
#define   nx_pop3_rsp_quit                          _nx_pop3_rsp_quit
#define   nx_pop3_rsp_retr                          _nx_pop3_rsp_retr
#define   nx_pop3_rsp_rset                          _nx_pop3_rsp_rset
#define   nx_pop3_rsp_stat                          _nx_pop3_rsp_stat
#define   nx_pop3_rsp_user                          _nx_pop3_rsp_user
#define   nx_pop3_session_delete                    _nx_pop3_session_delete
#define   nx_pop3_session_initialize                _nx_pop3_session_initialize
#define   nx_pop3_session_reinitialize              _nx_pop3_session_reinitialize
#define   nx_pop3_session_run                       _nx_pop3_session_run
#define   nx_pop3_session_run                       _nx_pop3_session_run
#define   nx_pop3_utility_print_client_mailitem     _nx_pop3_utility_print_client_mailitem
#define   nx_pop3_utility_print_client_reserves     _nx_pop3_utility_print_client_reserves

#else

/* Services with error checking.  */

#define   nx_pop3_client_connect                    _nxe_pop3_client_connect
#define   nx_pop3_client_create                     _nxe_pop3_client_create
#define   nx_pop3_client_delete                     _nxe_pop3_client_delete
#define   nx_pop3_cmd_apop                          _nxe_pop3_cmd_apop
#define   nx_pop3_cmd_dele                          _nxe_pop3_cmd_dele
#define   nx_pop3_cmd_greeting                      _nxe_pop3_cmd_greeting
#define   nx_pop3_cmd_list                          _nxe_pop3_cmd_list
#define   nx_pop3_cmd_noop                          _nxe_pop3_cmd_noop
#define   nx_pop3_cmd_pass                          _nxe_pop3_cmd_pass
#define   nx_pop3_cmd_retr                          _nxe_pop3_cmd_retr
#define   nx_pop3_cmd_quit                          _nxe_pop3_cmd_quit
#define   nx_pop3_cmd_rset                          _nxe_pop3_cmd_rset
#define   nx_pop3_cmd_stat                          _nxe_pop3_cmd_stat
#define   nx_pop3_cmd_user                          _nxe_pop3_cmd_user
#define   nx_pop3_mail_add                          _nxe_pop3_mail_add
#define   nx_pop3_mail_create                       _nxe_pop3_mail_create
#define   nx_pop3_mail_delete                       _nxe_pop3_mail_delete
#define   nx_pop3_mail_spool                        _nxe_pop3_mail_spool
#define   nx_pop3_rsp_apop                          _nxe_pop3_rsp_apop
#define   nx_pop3_rsp_dele                          _nxe_pop3_rsp_dele
#define   nx_pop3_rsp_greeting                      _nxe_pop3_rsp_greeting
#define   nx_pop3_rsp_list                          _nxe_pop3_rsp_list
#define   nx_pop3_rsp_noop                          _nxe_pop3_rsp_noop   
#define   nx_pop3_rsp_pass                          _nxe_pop3_rsp_pass
#define   nx_pop3_rsp_quit                          _nxe_pop3_rsp_quit
#define   nx_pop3_rsp_retr                          _nxe_pop3_rsp_retr
#define   nx_pop3_rsp_rset                          _nxe_pop3_rsp_rset
#define   nx_pop3_rsp_stat                          _nxe_pop3_rsp_stat
#define   nx_pop3_rsp_user                          _nxe_pop3_rsp_user
#define   nx_pop3_session_delete                    _nxe_pop3_session_delete
#define   nx_pop3_session_initialize                _nxe_pop3_session_initialize
#define   nx_pop3_session_reinitialize              _nxe_pop3_session_reinitialize
#define   nx_pop3_session_run                       _nxe_pop3_session_run
#define   nx_pop3_session_run                       _nxe_pop3_session_run
#define   nx_pop3_utility_print_client_mailitem     _nxe_pop3_utility_print_client_mailitem
#define   nx_pop3_utility_print_client_reserves     _nxe_pop3_utility_print_client_reserves



#endif /* if NX_DISABLE_ERROR_CHECKING */



UINT    nx_pop3_client_connect(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    nx_pop3_client_create(NX_POP3_CLIENT *client_ptr, CHAR *client_name, CHAR *client_password, CHAR *client_shared_secret,   
                                UINT APOP_authentication, UINT enable_user_pass_authentication, NX_IP *ip_ptr, NX_PACKET_POOL *packet_pool_ptr, 
                                TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *bytepool_mutex_ptr, 
                                ULONG bytepool_mutex_timeout, TX_BLOCK_POOL *blockpool_ptr,
                                TX_MUTEX *blockpool_mutex_ptr, ULONG blockpool_mutex_timeout, 
                                ULONG reply_timeout, ULONG window_size,
                                UINT (*nx_pop3_client_mail_spooler)(NX_POP3_CLIENT_MAIL *mail_ptr)); 
UINT    nx_pop3_client_delete (NX_POP3_CLIENT *client_ptr);
UINT    nx_pop3_cmd_apop(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    nx_pop3_cmd_dele(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    nx_pop3_cmd_greeting(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    nx_pop3_cmd_list(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    nx_pop3_cmd_noop(NX_POP3_CLIENT_SESSION     *session_ptr);
UINT    nx_pop3_cmd_pass(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    nx_pop3_cmd_quit(NX_POP3_CLIENT_SESSION     *session_ptr);
UINT    nx_pop3_cmd_retr(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    nx_pop3_cmd_rset(NX_POP3_CLIENT_SESSION     *session_ptr);
UINT    nx_pop3_cmd_stat(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    nx_pop3_cmd_user(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    nx_pop3_mail_add(NX_POP3_CLIENT_SESSION *session_ptr, NX_POP3_CLIENT_MAIL *mail_ptr);
UINT    nx_pop3_mail_create(NX_POP3_CLIENT_SESSION *session_ptr, NX_POP3_CLIENT_MAIL **session_mail_ptr);
UINT    nx_pop3_mail_delete(NX_POP3_CLIENT_SESSION *session_ptr, NX_POP3_CLIENT_MAIL *mail_ptr);
UINT    nx_pop3_mail_spool(NX_POP3_CLIENT_SESSION *session_ptr, NX_POP3_CLIENT_MAIL *mail_ptr);
UINT    nx_pop3_rsp_apop(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    nx_pop3_rsp_dele(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    nx_pop3_rsp_greeting(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    nx_pop3_rsp_list(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    nx_pop3_rsp_noop(NX_POP3_CLIENT_SESSION     *session_ptr);
UINT    nx_pop3_rsp_pass(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    nx_pop3_rsp_quit(NX_POP3_CLIENT_SESSION     *session_ptr);
UINT    nx_pop3_rsp_retr(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    nx_pop3_rsp_rset(NX_POP3_CLIENT_SESSION     *session_ptr);
UINT    nx_pop3_rsp_stat(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    nx_pop3_rsp_user(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    nx_pop3_session_delete(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    nx_pop3_session_initialize(NX_POP3_CLIENT_SESSION *session_ptr, ULONG session_id, NX_POP3_CLIENT *client_ptr, UINT delete_downloaded_mail, ULONG ip_addr, USHORT port);
UINT    nx_pop3_session_reinitialize(NX_POP3_CLIENT_SESSION *session_ptr, UINT session_availability);
UINT    nx_pop3_session_run (NX_POP3_CLIENT_SESSION *session_ptr);
UINT    nx_pop3_utility_print_client_mailitem(NX_POP3_CLIENT_MAIL *mail_ptr);
UINT    nx_pop3_utility_print_client_reserves(NX_POP3_CLIENT *client_ptr);

#else   /* if NX_POP3_SOURCE_CODE */


/* Client and session specific functions.  */

UINT    _nx_pop3_client_connect(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nxe_pop3_client_connect(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nx_pop3_client_create(NX_POP3_CLIENT *client_ptr, CHAR *client_name, CHAR *client_password, CHAR *client_shared_secret,     
                                UINT APOP_authentication, UINT enable_user_pass_authentication, NX_IP *ip_ptr, NX_PACKET_POOL *packet_pool_ptr, 
                                TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *bytepool_mutex_ptr, 
                                ULONG bytepool_mutex_timeout, TX_BLOCK_POOL *blockpool_ptr,
                                TX_MUTEX *blockpool_mutex_ptr, ULONG blockpool_mutex_timeout, 
                                ULONG reply_timeout, ULONG window_size,
                                UINT (*nx_pop3_client_mail_spooler)(NX_POP3_CLIENT_MAIL *mail_ptr)); 
UINT    _nxe_pop3_client_create(NX_POP3_CLIENT *client_ptr, CHAR *client_name, CHAR *client_password, CHAR *client_shared_secret,     
                                UINT APOP_authentication, UINT enable_user_pass_authentication, NX_IP *ip_ptr, NX_PACKET_POOL *packet_pool_ptr, 
                                TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *bytepool_mutex_ptr, 
                                ULONG bytepool_mutex_timeout, TX_BLOCK_POOL *blockpool_ptr,
                                TX_MUTEX *blockpool_mutex_ptr, ULONG blockpool_mutex_timeout, 
                                ULONG reply_timeout, ULONG window_size,
                                UINT (*nx_pop3_client_mail_spooler)(NX_POP3_CLIENT_MAIL *mail_ptr)); 
UINT    _nx_pop3_client_delete (NX_POP3_CLIENT *client_ptr);
UINT    _nxe_pop3_client_delete (NX_POP3_CLIENT *client_ptr);
UINT    _nx_pop3_cmd_apop(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nxe_pop3_cmd_apop(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nx_pop3_cmd_dele(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nxe_pop3_cmd_dele(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nx_pop3_cmd_greeting(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nxe_pop3_cmd_greeting(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nx_pop3_cmd_list(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nxe_pop3_cmd_list(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nx_pop3_cmd_noop(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nxe_pop3_cmd_noop(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nx_pop3_cmd_pass(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nxe_pop3_cmd_pass(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nx_pop3_cmd_quit(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nxe_pop3_cmd_quit(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nx_pop3_cmd_retr(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nxe_pop3_cmd_retr(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nx_pop3_cmd_rset(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nxe_pop3_cmd_rset(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nx_pop3_cmd_stat(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nxe_pop3_cmd_stat(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nx_pop3_cmd_user(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nxe_pop3_cmd_user(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nx_pop3_mail_add(NX_POP3_CLIENT_SESSION *session_ptr, NX_POP3_CLIENT_MAIL *mail_ptr);
UINT    _nxe_pop3_mail_add(NX_POP3_CLIENT_SESSION *session_ptr, NX_POP3_CLIENT_MAIL *mail_ptr);
UINT    _nx_pop3_mail_create(NX_POP3_CLIENT_SESSION *session_ptr, NX_POP3_CLIENT_MAIL **session_mail_ptr);
UINT    _nxe_pop3_mail_create(NX_POP3_CLIENT_SESSION *session_ptr, NX_POP3_CLIENT_MAIL **session_mail_ptr);
UINT    _nx_pop3_mail_delete(NX_POP3_CLIENT_SESSION *session_ptr, NX_POP3_CLIENT_MAIL *mail_ptr);
UINT    _nxe_pop3_mail_delete(NX_POP3_CLIENT_SESSION *session_ptr, NX_POP3_CLIENT_MAIL *mail_ptr);
UINT    _nx_pop3_mail_spool(NX_POP3_CLIENT_SESSION *session_ptr, NX_POP3_CLIENT_MAIL *mail_ptr);
UINT    _nxe_pop3_mail_spool(NX_POP3_CLIENT_SESSION *session_ptr, NX_POP3_CLIENT_MAIL *mail_ptr);
UINT    _nx_pop3_rsp_apop(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nxe_pop3_rsp_apop(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nx_pop3_rsp_dele(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nxe_pop3_rsp_dele(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nx_pop3_rsp_greeting(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nxe_pop3_rsp_greeting(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nx_pop3_rsp_list(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nxe_pop3_rsp_list(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nx_pop3_rsp_noop(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nxe_pop3_rsp_noop(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nx_pop3_rsp_pass(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nxe_pop3_rsp_pass(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nx_pop3_rsp_quit(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nxe_pop3_rsp_quit(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nx_pop3_rsp_retr(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nxe_pop3_rsp_retr(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nx_pop3_rsp_rset(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nxe_pop3_rsp_rset(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nx_pop3_rsp_stat(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nxe_pop3_rsp_stat(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nx_pop3_rsp_user(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nxe_pop3_rsp_user(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nx_pop3_session_delete(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nxe_pop3_session_delete(NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nx_pop3_session_initialize(NX_POP3_CLIENT_SESSION *session_ptr, ULONG session_id, NX_POP3_CLIENT *client_ptr, UINT delete_downloaded_mail, ULONG ip_addr, USHORT port);
UINT    _nxe_pop3_session_initialize(NX_POP3_CLIENT_SESSION *session_ptr, ULONG session_id, NX_POP3_CLIENT *client_ptr, UINT delete_downloaded_mail, ULONG ip_addr, USHORT port);
UINT    _nx_pop3_session_reinitialize(NX_POP3_CLIENT_SESSION *session_ptr, UINT session_availability);
UINT    _nxe_pop3_session_reinitialize(NX_POP3_CLIENT_SESSION *session_ptr, UINT session_availability);
UINT    _nx_pop3_session_run (NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nxe_pop3_session_run (NX_POP3_CLIENT_SESSION *session_ptr);
UINT    _nx_pop3_utility_print_client_mailitem(NX_POP3_CLIENT_MAIL *mail_ptr);
UINT    _nxe_pop3_utility_print_client_mailitem(NX_POP3_CLIENT_MAIL *mail_ptr);
UINT    _nx_pop3_utility_print_client_reserves(NX_POP3_CLIENT *client_ptr);
UINT    _nxe_pop3_utility_print_client_reserves(NX_POP3_CLIENT *client_ptr);


/* NetX POP3 Client internal functions */

UINT    _nx_pop3_client_blockpool_memory_get(VOID **memory_ptr, TX_BLOCK_POOL *blockpool_ptr, TX_MUTEX *mutex_ptr, ULONG mutex_wait_option);
UINT    _nx_pop3_client_blockpool_memory_release(VOID *memory_ptr, TX_BLOCK_POOL *blockpool_ptr, TX_MUTEX *mutex_ptr, ULONG mutex_wait_option);
UINT    _nx_pop3_client_bytepool_memory_get(VOID **memory_ptr, ULONG memory_size, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *mutex_ptr, ULONG mutex_wait_option);
UINT    _nx_pop3_client_bytepool_memory_release(VOID *memory_ptr, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *mutex_ptr, ULONG mutex_wait_option);

UINT    _nx_pop3_message_segment_add(NX_POP3_CLIENT_MAIL *mail_ptr, NX_POP3_MESSAGE_SEGMENT *message_segment_ptr);
VOID    _nx_pop3_utility_command_name(CHAR *command_name, UINT length, UINT command_id);
UINT    _nx_pop3_utility_digest_authenticate(NX_POP3_CLIENT *client_ptr, CHAR *process_ID_ptr, CHAR *secret_word, CHAR *result);
VOID    _nx_pop3_utility_find_crlf(CHAR *buffer, UINT length, CHAR **CRLF, UINT reverse);
UINT    _nx_pop3_utility_get_server_reply(NX_POP3_CLIENT_SESSION *session_ptr, ULONG timeout,UINT *server_reply, UINT  expect_numeric_reply, NX_PACKET **packet_ptr);
VOID    _nx_pop3_utility_hex_ascii_convert(CHAR *source, UINT source_length, CHAR *destination);
VOID    _nx_pop3_utility_parse_response(CHAR *buffer, UINT argument_index, UINT buffer_length, CHAR *argument, UINT argument_length, UINT convert_to_uppercase, UINT include_crlf);
UINT    _nx_pop3_utility_send_to_server (NX_POP3_CLIENT_SESSION *session_ptr, CHAR *buffer_ptr, UINT buffer_length, ULONG timeout);
VOID    _nx_pop3_utility_session_state_name(CHAR *state_name, UINT length, UINT state_id);


#endif /* NX_POP3_SOURCE_CODE */


/* If a C++ compiler is being used....*/
#ifdef   __cplusplus
        }
#endif


#endif /* NX_POP3_CLIENT_H  */
