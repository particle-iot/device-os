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
/*  Express Logic, Inc.                     info@expresslogic.com         */
/*  11423 West Bernardo Court               http://www.expresslogic.com   */
/*  San Diego, CA  92127                                                  */
/*                                                                        */
/**************************************************************************/

/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** NetX POP3 Server Component                                            */
/**                                                                       */
/**   Post Office Protocol Version 3 (POP3)                               */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/**************************************************************************/
/*                                                                        */
/*  APPLICATION INTERFACE DEFINITION                       RELEASE        */
/*                                                                        */
/*    nx_pop3_server.h                                    PORTABLE C      */
/*                                                           5.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Janet Christiansen, Express Logic, Inc.                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This file defines the NetX Post Office Protocol Version 3 (POP3)    */
/*    server component, including all data types and external references. */
/*    It is assumed that tx_api.h, tx_port.h, nx_api.h, and nx_port.h,    */
/*    have already been included.                                         */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  09-21-2007     Janet Christiansen       Initial Version 1.0           */ 
/*  04-22-2010     Janet Christiansen       Modified comment(s),          */
/*                                          resulting in version 5.1      */
/*                                                                        */
/**************************************************************************/

#ifndef NX_POP3_SERVER_H
#define NX_POP3_SERVER_H


#ifdef   __cplusplus

/* Yes, C++ compiler is present.  Use standard C.  */ 
extern   "C" {

#endif


#include "nx_api.h"
#include "nx_pop3.h"


/* Start of configurable options.  */

/* Set the event reporting/debug output for the NetX POP3 server.  */

#ifndef NX_POP3_SERVER_DEBUG
#define NX_POP3_SERVER_DEBUG                         NX_POP3_DEBUG_LEVEL_NONE
#endif

/* Scheme for filtering messages during program execution. 

   printf() itself may need to be defined for the specific 
   processor that is running the application and communication
   available (e.g. serial port).  */

#if ( NX_POP3_SERVER_DEBUG == NX_POP3_DEBUG_LEVEL_NONE )
#define NX_POP3_SERVER_EVENT_LOG(debug_level, msg)
#else
#define NX_POP3_SERVER_EVENT_LOG(debug_level, msg)                  \
{                                                                   \
    UINT level = (UINT)debug_level;                                 \
    if (level <= NX_POP3_DEBUG_LEVEL_ALL && NX_POP3_SERVER_DEBUG == NX_POP3_DEBUG_LEVEL_ALL)                \
    {                                                               \
       printf msg ;                                                 \
    }                                                               \
    else if (level <= NX_POP3_DEBUG_LEVEL_MODERATE && NX_POP3_SERVER_DEBUG == NX_POP3_DEBUG_LEVEL_MODERATE) \
    {                                                               \
       printf msg ;                                                 \
    }                                                               \
    else if (level == NX_POP3_DEBUG_LEVEL_SEVERE && NX_POP3_SERVER_DEBUG == NX_POP3_DEBUG_LEVEL_SEVERE)     \
    {                                                               \
       printf msg ;                                                 \
    }                                                               \
}
#endif /* NX_POP3_SERVER_DEBUG */
  
/* Enable print server packet and memory pool reserves feature.  */
/*
#define NX_POP3_PRINT_SERVER_RESERVES              
*/

/* Set Net TCP print summary mutex timeout in ticks.  */

#ifndef NX_POP3_SERVER_PRINT_TIMEOUT            
#define NX_POP3_SERVER_PRINT_TIMEOUT                 (1 * NX_POP3_TICKS_PER_SECOND)  
#endif


/* Configure POP3 Server thread and stack parameters.  */


/* Set the POP3 server thread stack size.  */

#ifndef NX_POP3_SERVER_THREAD_STACK_SIZE        
#define NX_POP3_SERVER_THREAD_STACK_SIZE              4096
#endif

/* Set POP3 server thread priority.  */

#ifndef NX_POP3_SERVER_THREAD_PRIORITY 
#define NX_POP3_SERVER_THREAD_PRIORITY                2
#endif


/* Set POP3 server thread time slice (how long it runs before threads of equal priority may run).  */

#ifndef NX_POP3_SERVER_THREAD_TIME_SLICE
#define NX_POP3_SERVER_THREAD_TIME_SLICE              TX_NO_TIME_SLICE
#endif


/* Set POP3 server preemption threshold.  */

#ifndef NX_POP3_SERVER_PREEMPTION_THRESHOLD
#define NX_POP3_SERVER_PREEMPTION_THRESHOLD           NX_POP3_SERVER_THREAD_PRIORITY  
#endif


/* Set the server session stack size.  */

#ifndef NX_POP3_SERVER_SESSION_THREAD_STACK_SIZE        
#define NX_POP3_SERVER_SESSION_THREAD_STACK_SIZE      4096 
#endif

/* Set POP3 server session thread priority.  */

#ifndef NX_POP3_SERVER_SESSION_THREAD_PRIORITY 
#define NX_POP3_SERVER_SESSION_THREAD_PRIORITY        NX_POP3_SERVER_THREAD_PRIORITY
#endif


/* Set POP3 server session preemption threshold.  */

#ifndef NX_POP3_SERVER_SESSION_PREEMPTION_THRESHOLD
#define NX_POP3_SERVER_SESSION_PREEMPTION_THRESHOLD   NX_POP3_SERVER_SESSION_THREAD_PRIORITY  
#endif

/* Set POP3 server session thread time slice.  */

#ifndef NX_POP3_SERVER_SESSION_THREAD_TIME_SLICE
#define NX_POP3_SERVER_SESSION_THREAD_TIME_SLICE      TX_NO_TIME_SLICE
#endif


/* Configure POP3 server memory resources.  */


/* Set POP3 server byte pool name.  */

#ifndef NX_POP3_SERVER_BYTE_POOL_NAME
#define NX_POP3_SERVER_BYTE_POOL_NAME                 "POP3 Server bytepool"
#endif

/* Set POP3 server byte pool size.  */

#ifndef NX_POP3_SERVER_BYTE_POOL_SIZE
#define NX_POP3_SERVER_BYTE_POOL_SIZE                 (1024 * 4)
#endif


/* Set POP3 server byte pool mutex name.  */

#ifndef NX_POP3_SERVER_BYTE_POOL_MUTEX_NAME
#define NX_POP3_SERVER_BYTE_POOL_MUTEX_NAME            "POP3 Server bytepool mutex" 
#endif


/* Set POP3 server byte pool mutex timeout.  */

#ifndef NX_POP3_SERVER_BYTE_POOL_MUTEX_WAIT
#define NX_POP3_SERVER_BYTE_POOL_MUTEX_WAIT            (2 * NX_POP3_TICKS_PER_SECOND)
#endif


/* Configure the POP3 server network parameters.  */

/* Set port for POP3 server to listen on.  */

#ifndef NX_POP3_SERVER_SESSION_PORT                       
#define NX_POP3_SERVER_SESSION_PORT                    110     
#endif


/* Set server socket queue size (number of connection requests that can be queued.  */

#ifndef NX_POP3_SERVER_SOCKET_QUEUE_SIZE
#define NX_POP3_SERVER_SOCKET_QUEUE_SIZE               5
#endif

/* Set TCP receive window size (maximum number of bytes in socket receive queue).  */

#ifndef NX_POP3_SERVER_WINDOW_SIZE   
#define NX_POP3_SERVER_WINDOW_SIZE                     NX_POP3_SERVER_PACKET_SIZE
#endif

/* Set size of NetX POP3 server packet. Best if close to (less than) the device MTU.  */

#ifndef NX_POP3_SERVER_PACKET_SIZE
#define NX_POP3_SERVER_PACKET_SIZE                     1500  
#endif


/* Set size of NetX POP3 server packet pool.  */

#ifndef NX_POP3_SERVER_PACKET_POOL_SIZE    
#define NX_POP3_SERVER_PACKET_POOL_SIZE                (20 * NX_POP3_SERVER_PACKET_SIZE)
#endif


/* Set timeout on NetX packet allocation.  */

#ifndef NX_POP3_SERVER_PACKET_TIMEOUT
#define NX_POP3_SERVER_PACKET_TIMEOUT                  (1 * NX_POP3_TICKS_PER_SECOND)
#endif


/* Set timeout for server TCP socket send completion.  */

#ifndef NX_POP3_SERVER_TCP_SOCKET_SEND_WAIT    
#define NX_POP3_SERVER_TCP_SOCKET_SEND_WAIT            (3  * NX_POP3_TICKS_PER_SECOND)    
#endif


/* Set NetX IP helper thread stack size.  */

#ifndef NX_POP3_SERVER_IP_THREAD_STACK_SIZE   
#define NX_POP3_SERVER_IP_THREAD_STACK_SIZE            2048
#endif

/* Set the server IP thread priority */

#ifndef NX_POP3_SERVER_IP_THREAD_PRIORITY
#define NX_POP3_SERVER_IP_THREAD_PRIORITY              2
#endif

/* Set ARP cache size of Server IP instance.  */

#ifndef NX_POP3_SERVER_ARP_CACHE_SIZE
#define NX_POP3_SERVER_ARP_CACHE_SIZE                  1040
#endif


/* Set NetX POP3 server timeout wait between client commands.  */

#ifndef NX_POP3_SERVER_TCP_RECEIVE_TIMEOUT              
#define NX_POP3_SERVER_TCP_RECEIVE_TIMEOUT             (5 * NX_POP3_TICKS_PER_SECOND)  
#endif

/* Set Net TCP connection timeout in ticks.  */

#ifndef NX_POP3_SERVER_CONNECTION_TIMEOUT               
#define NX_POP3_SERVER_CONNECTION_TIMEOUT              NX_WAIT_FOREVER 
#endif


/* Set Net TCP disconnect timeout in ticks.  */

#ifndef NX_POP3_SERVER_DISCONNECT_TIMEOUT            
#define NX_POP3_SERVER_DISCONNECT_TIMEOUT              (10 * NX_POP3_TICKS_PER_SECOND)  
#endif


/* Configure the POP3 server session parameters */

/* Set the number of POP3 server sessions handling concurrent Client sessions.  */

#ifndef  NX_POP3_MAX_SERVER_SESSIONS  
#define  NX_POP3_MAX_SERVER_SESSIONS                    1 
#endif

/* Set the max number of Client maildrops on the POP3 Server.  */

#ifndef NX_POP3_SERVER_MAX_MAILDROP_COUNT                 
#define NX_POP3_SERVER_MAX_MAILDROP_COUNT               3
#endif


/* Define the max size of the POP3 server reply. The server
   reply buffer are typically one line replies. This configuration
   option does not apply to potentially larger server messages,
   such as a line by line LIST-ing of client maildrop items and
   downloading mail message data.  */

#ifndef NX_POP3_SERVER_MAX_REPLY
#define NX_POP3_SERVER_MAX_REPLY                     200
#endif

#ifndef NX_POP3_MAX_CLIENT_USERNAME
#define NX_POP3_MAX_CLIENT_USERNAME                  40
#endif

#ifndef NX_POP3_MAX_CLIENT_PASSWORD
#define NX_POP3_MAX_CLIENT_PASSWORD                  20
#endif

#ifndef NX_POP3_MAX_CLIENT_SECRET
#define NX_POP3_MAX_CLIENT_SECRET                    20
#endif

/* Define the size of the buffer to contain the APOP string the server
   sends to the Client (contains the process ID, local clock 
   time and domain name) and other characters (spaces, angle brackets).  */

#ifndef NX_POP3_MAX_SERVER_APOP_STRING
#define NX_POP3_MAX_SERVER_APOP_STRING               100
#endif

/* Server clock time string has the general format of YYYYMMDDHHMM
    + 3 characters for milliseconds.  */

#ifndef NX_POP3_MAX_CLOCK_TIME
#define NX_POP3_MAX_CLOCK_TIME                       20
#endif

/* POP3 Servers typically use a process ID in the current POP3 session
   in the combined Process ID/Clock time string in the greeting.  */

#ifndef NX_POP3_MAX_PROCESS_ID
#define NX_POP3_MAX_PROCESS_ID                       10
#endif


/* Set server domain name as identifier in text replies to POP3 clients. 
   The default NetX POP3 server domain name is fictitious*/

#ifndef NX_POP3_SERVER_DOMAIN
#define NX_POP3_SERVER_DOMAIN                       "server.com" 
#endif


/* In lieu of a callback function for local server time, set a dummy time
   for the Server greeting message to the Client.  */

#ifndef NX_POP3_SERVER_DEFAULT_TIME
#define NX_POP3_SERVER_DEFAULT_TIME                  "200706150600000"
#endif

/* In lieu of a callback function for local server process ID, set a dummy ID
   for the Server greeting message to the Client.  */

#ifndef NX_POP3_SERVER_DEFAULT_PROCESS_ID
#define NX_POP3_SERVER_DEFAULT_PROCESS_ID            "1234"
#endif


/* The following configuration options do not effect the operation
   of the POP3 Server.  The Server should not expect the POP3 Client 
   to parse or read additional text in the Server replies other than
   the command arguments.  */


#define NX_POP3_SERVER_GREETING                       "POP3 Server ready"

#define NX_POP3_SERVER_SAYS_BYE                       "Bye bye"

#define NX_POP3_PASSWORD_REQUIRED                     "Password required."

#define NX_POP3_LOGIN_OK                              "Login OK."

#define NX_POP3_LOGIN_NOGOOD                          "Login failed."

#define NX_POP3_APOP_AUTHENTICATION_FAILED            "APOP Authentication failed."

#define NX_POP3_ERROR_INVALID_USERNAME                "Invalid username in command."

#define NX_POP3_ERROR_INVALID_PASSWORD                "Invalid password in command."

#define NX_POP3_ERROR_INVALID_APOP_DIGEST             "Invalid APOP digest in command."

#define NX_POP3_ERROR_COMMAND_SYNTAX                  "Command syntax error."

#define NX_POP3_ERROR_INVALID_PARAMETER               "Invalid parameter in Client command."

#define NX_POP3_ERROR_ILLEGAL_COMMAND                 "Command is illegal in the current POP3 state."

#define NX_POP3_ERROR_UNKNOWN_COMMAND                 "Unknown command received from Client."



/* Three digit server reply codes as bit fields so the server can prepare a bitmask
   of expected commands from the client.  */

#define     NX_POP3_COMMAND_GREET_CODE          0x0000 
#define     NX_POP3_COMMAND_USER_CODE           0x0001 
#define     NX_POP3_COMMAND_APOP_CODE           0x0002 
#define     NX_POP3_COMMAND_PASS_CODE           0x0004 
#define     NX_POP3_COMMAND_STAT_CODE           0x0008 
#define     NX_POP3_COMMAND_LIST_CODE           0x0010 
#define     NX_POP3_COMMAND_RETR_CODE           0x0020 
#define     NX_POP3_COMMAND_DELE_CODE           0x0040 
#define     NX_POP3_COMMAND_RSET_CODE           0x0080
#define     NX_POP3_COMMAND_NOOP_CODE           0x0100
#define     NX_POP3_COMMAND_QUIT_CODE           0x0200 
#define     NX_POP3_COMMAND_UNKNOWN_CODE        0xFFFF


/* Define the NetX POP3 Server Mail structure */

typedef struct NX_POP3_SERVER_MAIL_STRUCT
{

    struct NX_POP3_SERVER_MAIL_STRUCT       *previous_ptr;               /* Pointer to the previous mail in the mail item's session */
    struct NX_POP3_SERVER_MAIL_STRUCT       *next_ptr;                   /* Pointer to the next mail in the mail item's session */
    UINT                                    marked_for_deletion;         /* Status if mail ready for deletion */
    ULONG                                   message_length;              /* Total size of mail item message (including message data saved in 'message segments').  */

} NX_POP3_SERVER_MAIL;

/* Define the NetX POP3 Server Maildrop (contain each Client's mail) structure */

typedef struct NX_POP3_SERVER_MAILDROP_STRUCT
{

    CHAR                                    *client_username;           /* Owner/Addressee of Server maildrop */
    CHAR                                    *client_password;           /* Password of owner of Server maildrop */
    CHAR                                    *shared_secret;             /* Shared secret between owner of maildrop and Server */
    struct NX_POP3_SERVER_MAIL_STRUCT       *start_mail_ptr;            /* Pointer to first mail item in Client maildrop */
    struct NX_POP3_SERVER_MAIL_STRUCT       *end_mail_ptr;              /* Pointer to last mail item in Client maildrop */
    UINT                                    total_bytes;                /* Total amount of mail message data in Client maildrop */
    UINT                                    total_mail_items;           /* Total number of mail items in Client maildrop */

} NX_POP3_SERVER_MAILDROP;

/* Define the POP3 Server Session structure.  */

typedef struct NX_POP3_SERVER_SESSION_STRUCT
{
    UINT                                    session_id;                     /* Unique Session ID  */
    ULONG                                   nx_pop3_server_session_id;      /* POP3 ID identifying session thread as ready to run */
    struct NX_POP3_SERVER_STRUCT            *server_ptr;                    /* Pointer to the POP3 server.  */
    struct NX_POP3_SERVER_SESSION_STRUCT    *next_ptr;                      /* Pointer to server's next session */
    NX_POP3_SERVER_MAILDROP                 client_maildrop;                /* Control block for maildrop for current session Client */
    UINT                                    available;                      /* Session available for connecting to Client in a multi session POP3 Server */
    NX_TCP_SOCKET                           session_socket;                 /* Session TCP socket */
    TX_THREAD                               session_thread;                 /* POP3 server session thread */
    UINT                                    connection_pending;             /* Connection pending flag  */
    UINT                                    session_mid_transaction;        /* Status of session currently handling mail transaction (vs being idle).  */
    UINT                                    session_closed;                 /* Status for session being closed to any more client connections */
    ULONG                                   session_connection_attempts;    /* Count of client POP3 requests detected.  */
    ULONG                                   session_connection_failures;    /* Count of client connection accept failures.  */
    ULONG                                   session_disconnection_requests; /* Count of client disconnections without properly quitting the session.  */
    UINT                                    cmd_expected_state;             /* Expected command set for server state */
    NX_PACKET                               *session_packet_ptr;            /* Pointer to packet received during server session */
    ULONG                                   session_mail_transaction_attempts;  /* Number of mail transactions attempted in a session */
    ULONG                                   session_valid_mail_transactions;    /* Number of valid mail transactions in a session */
    UINT                                    session_state;                      /* Protocol state of the POP3 session.  */
    CHAR                                    greeting_APOP_string[NX_POP3_MAX_SERVER_APOP_STRING]; /* Max size for the Server APOP string to send to the Client.  */
    CHAR                                    client_username[NX_POP3_MAX_CLIENT_USERNAME];         /* Max size for Client user name parsed from USER command */
    CHAR                                    client_password[NX_POP3_MAX_CLIENT_PASSWORD];         /* Max size for Client password parsed from PASS command */
    CHAR                                    client_shared_secret[NX_POP3_MAX_CLIENT_SECRET];      /* Max size for Client shared secret parsed from PASS command */
    CHAR                                    client_process_ID[NX_POP3_MAX_PROCESS_ID];            /* Max size for process ID in Server greeting text.  */
    CHAR                                    client_APOP_digest[NX_POP3_MAX_ASCII_MD5];            /* Max size for Client digest parsed from APOP command */


} NX_POP3_SERVER_SESSION;

/* Define the POP3 Server structure.  */

typedef struct NX_POP3_SERVER_STRUCT
{
    ULONG                                   nx_pop3_server_id;               /* NetX POP3 Server thread ID  */
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
    TX_THREAD                               server_thread;                   /* POP3 server thread */
    UINT                                    server_closed;                   /* Status if server closed to any further client connections */
    NX_POP3_SERVER_SESSION                  nx_pop3_server_session_list[NX_POP3_MAX_SERVER_SESSIONS]; 
                                                                             /* Max number of active POP3 sessions servicing Client requests.  */
    NX_POP3_SERVER_MAILDROP                 client_maildrops[NX_POP3_SERVER_MAX_MAILDROP_COUNT];
                                                                             /* List for creating up to the max number of Client maildrops (header data only) */
    UINT                                    client_maildrop_count;           /* Number of actual Client maildrops sitting in Server mail cache.  */
    TX_BYTE_POOL                            *bytepool_ptr;                   /* Pointer to server byte pool */
    TX_MUTEX                                *bytepool_mutex_ptr;             /* Pointer to server byte pool mutex */
    UINT                                    bytepool_mutex_timeout;          /* Timeout value for byte pool mutex */
    UINT                                    (*nx_pop3_server_authentication_check)(NX_POP3_SERVER_SESSION *session_ptr, CHAR *name, CHAR *password,UINT *result);
                                                                             /* Pointer to sender authentication check function */
    UINT                                    (*nx_pop3_server_get_clock_time)(CHAR *clock_time);  
                                                                             /* Pointer to get local clock time callback */
    UINT                                    (*nx_pop3_server_get_process_ID)(CHAR *process_ID);  
                                                                             /* Pointer to get local process ID callback */
    UINT                                    (*nx_pop3_server_get_client_maildrop)(NX_POP3_SERVER_SESSION *session_ptr, CHAR *client_username, UINT *maildrop_found);
                                                                             /* Pointer to get session's Client maildrop callback */
    UINT                                    (*nx_pop3_server_get_client_mailitem_data)(NX_POP3_SERVER_SESSION *session_ptr, UINT maildrop_index, ULONG *mailitem_bytes, UINT *maildrop_found);
                                                                             /* Pointer to get session's Client mailitems callback */
    UINT                                    (*nx_pop3_server_create_client_maildrop_list)(struct NX_POP3_SERVER_STRUCT *server_ptr);
                                                                             /* Pointer to Load Client maildrops callback */
    UINT                                    (*nx_pop3_server_get_mail_message_buffer)(NX_POP3_SERVER_SESSION *session_ptr, UINT mailitem_index, CHAR **buffer_ptr, UINT *bytes_extracted, UINT *bytes_remaining);
                                                                             /* Pointer to return some or all of mail item's message data callback  */
    UINT                                    (*nx_pop3_server_delete_mail_on_file)(NX_POP3_SERVER_SESSION *session_ptr);
                                                                             /* Pointer to deleting specified mail item on Server hard drive callback  */
} NX_POP3_SERVER;



#ifndef     NX_POP3_SOURCE_CODE     


/* Define the system API mappings based on the error checking 
   selected by the user.   */

/* Determine if error checking is desired.  If so, map API functions
   to the appropriate error checking front-ends.  Otherwise, map API
   functions to the core functions that actually perform the work.
   Note: error checking is enabled by default.  */


#ifdef NX_POP3_DISABLE_ERROR_CHECKING

/* Services without error checking.  */

#define nx_pop3_server_create                   _nx_pop3_server_create
#define nx_pop3_server_delete                   _nx_pop3_server_delete
#define nx_pop3_server_get_time                 _nx_pop3_server_get_time
#define nx_pop3_server_get_PID                  _nx_pop3_server_get_PID
#define nx_pop3_server_get_auth_check           _nx_pop3_server_get_auth_check
#define nx_pop3_server_start                    _nx_pop3_server_start
#define nx_pop3_server_stop                     _nx_pop3_server_stop
#define nx_pop3_server_session_create           _nx_pop3_server_session_create
#define nx_pop3_server_session_delete           _nx_pop3_server_session_delete
#define nx_pop3_server_session_reinitialize     _nx_pop3_server_session_reinitialize
#define nx_pop3_server_session_run              _nx_pop3_server_session_run
#define nx_pop3_reply_to_greeting               _nx_pop3_reply_to_greeting
#define nx_pop3_reply_to_user                   _nx_pop3_reply_to_user
#define nx_pop3_reply_to_apop                   _nx_pop3_reply_to_apop
#define nx_pop3_reply_to_pass                   _nx_pop3_reply_to_pass
#define nx_pop3_reply_to_stat                   _nx_pop3_reply_to_stat
#define nx_pop3_reply_to_retr                   _nx_pop3_reply_to_retr
#define nx_pop3_reply_to_dele                   _nx_pop3_reply_to_dele
#define nx_pop3_reply_to_quit                   _nx_pop3_reply_to_quit
#define nx_pop3_reply_to_list                   _nx_pop3_reply_to_list
#define nx_pop3_reply_to_rset                   _nx_pop3_reply_to_rset
#define nx_pop3_reply_to_noop                   _nx_pop3_reply_to_noop
#define nx_pop3_server_print_server_reserves    _nx_pop3_server_print_server_reserves

#else

/* Services with error checking.  */

#define nx_pop3_server_create                   _nxe_pop3_server_create
#define nx_pop3_server_delete                   _nxe_pop3_server_delete
#define nx_pop3_server_get_time                 _nxe_pop3_server_get_time
#define nx_pop3_server_get_PID                  _nxe_pop3_server_get_PID
#define nx_pop3_server_get_auth_check           _nxe_pop3_server_get_auth_check
#define nx_pop3_server_start                    _nxe_pop3_server_start
#define nx_pop3_server_stop                     _nxe_pop3_server_stop
#define nx_pop3_server_session_create           _nxe_pop3_server_session_create
#define nx_pop3_server_session_delete           _nxe_pop3_server_session_delete
#define nx_pop3_server_session_reinitialize     _nxe_pop3_server_session_reinitialize
#define nx_pop3_server_session_run              _nxe_pop3_server_session_run
#define nx_pop3_reply_to_greeting               _nxe_pop3_reply_to_greeting
#define nx_pop3_reply_to_user                   _nxe_pop3_reply_to_user
#define nx_pop3_reply_to_apop                   _nxe_pop3_reply_to_apop
#define nx_pop3_reply_to_pass                   _nxe_pop3_reply_to_pass
#define nx_pop3_reply_to_stat                   _nxe_pop3_reply_to_stat
#define nx_pop3_reply_to_retr                   _nxe_pop3_reply_to_retr
#define nx_pop3_reply_to_dele                   _nxe_pop3_reply_to_dele
#define nx_pop3_reply_to_list                   _nxe_pop3_reply_to_list
#define nx_pop3_reply_to_rset                   _nxe_pop3_reply_to_rset
#define nx_pop3_reply_to_quit                   _nxe_pop3_reply_to_quit
#define nx_pop3_reply_to_noop                   _nxe_pop3_reply_to_noop
#define nx_pop3_server_print_server_reserves    _nxe_pop3_server_print_server_reserves

#endif    /* NX_POP3_DISABLE_ERROR_CHECKING */

UINT   nx_pop3_server_create(NX_POP3_SERVER *server_ptr, NX_IP *ip_ptr, VOID *stack_ptr, 
                             ULONG stack_size, UINT server_priority, UINT server_preempt_threshold, 
                             UINT server_time_slice, UINT auto_start, NX_PACKET_POOL *packet_pool_ptr, 
                             TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *bytepool_mutex_ptr, UINT bytepool_mutex_timeout,
                             UINT (*get_clock_time)(CHAR *clock_time),
                             UINT (*get_process_ID)(CHAR *process_ID),
                             UINT (*authentication_check)(NX_POP3_SERVER_SESSION *session_ptr, CHAR *username, CHAR *password, UINT *result),
                             UINT (*get_client_maildrop)(NX_POP3_SERVER_SESSION *session_ptr, CHAR *username, UINT *maildrop_found),
                             UINT (*get_client_mailitem_data)(NX_POP3_SERVER_SESSION *session_ptr, UINT maildrop_index, ULONG *mailitem_bytes, UINT *maildrop_found),
                             UINT (*load_client_maildrop)(NX_POP3_SERVER  *server_ptr),
                             UINT (*get_mail_message_buffer)(NX_POP3_SERVER_SESSION *session_ptr, UINT mailitem_index, CHAR **buffer_ptr, UINT *bytes_extracted, UINT *bytes_remaining),
                             UINT (*delete_mail_on_file)(NX_POP3_SERVER_SESSION *session_ptr));
UINT    nx_pop3_server_delete(NX_POP3_SERVER *server_ptr);
UINT    nx_pop3_server_get_time(NX_POP3_SERVER_SESSION *session_ptr, CHAR *clock_time);
UINT    nx_pop3_server_get_PID(NX_POP3_SERVER_SESSION *session_ptr, CHAR *process_ID);
UINT    nx_pop3_server_get_auth_check(NX_POP3_SERVER_SESSION *session_ptr, CHAR *username_ptr, CHAR *password_ptr, UINT *authenticated);
UINT    nx_pop3_server_session_create(NX_POP3_SERVER *server_ptr, NX_POP3_SERVER_SESSION *session_ptr, UINT session_id, 
                                      VOID *session_stack_ptr, ULONG session_stack_size, UINT session_priority, 
                                      UINT session_preempt_threshold, ULONG session_time_slice, UINT session_auto_start); 
UINT    nx_pop3_server_session_delete(NX_POP3_SERVER_SESSION *session_ptr);
UINT    nx_pop3_server_session_reinitialize(NX_POP3_SERVER_SESSION *session_ptr);
UINT    nx_pop3_server_session_run(NX_POP3_SERVER_SESSION  *session_ptr);
UINT    nx_pop3_server_start(NX_POP3_SERVER *server_ptr);
UINT    nx_pop3_server_stop(NX_POP3_SERVER *server_ptr);
UINT    nx_pop3_reply_to_greeting(NX_POP3_SERVER_SESSION *session_ptr);
UINT    nx_pop3_reply_to_user(NX_POP3_SERVER_SESSION *session_ptr);
UINT    nx_pop3_reply_to_apop(NX_POP3_SERVER_SESSION *session_ptr);
UINT    nx_pop3_reply_to_pass(NX_POP3_SERVER_SESSION *session_ptr);
UINT    nx_pop3_reply_to_stat(NX_POP3_SERVER_SESSION *session_ptr);
UINT    nx_pop3_reply_to_retr(NX_POP3_SERVER_SESSION *session_ptr);
UINT    nx_pop3_reply_to_dele(NX_POP3_SERVER_SESSION *session_ptr);
UINT    nx_pop3_reply_to_list(NX_POP3_SERVER_SESSION *session_ptr);
UINT    nx_pop3_reply_to_rset(NX_POP3_SERVER_SESSION *session_ptr);
UINT    nx_pop3_reply_to_quit(NX_POP3_SERVER_SESSION *session_ptr);
UINT    nx_pop3_reply_to_noop(NX_POP3_SERVER_SESSION *session_ptr);
UINT    nx_pop3_server_print_server_reserves(NX_POP3_SERVER *server_ptr);


#else     /* NX_POP3_SOURCE_CODE */

/* POP3 source code is being compiled, do not perform any API mapping.  */

UINT    _nx_pop3_server_create(NX_POP3_SERVER *server_ptr, NX_IP *ip_ptr, VOID *stack_ptr, 
                             ULONG stack_size, UINT server_priority, UINT server_preempt_threshold, 
                             UINT server_time_slice, UINT auto_start, NX_PACKET_POOL *packet_pool_ptr, 
                             TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *bytepool_mutex_ptr, UINT bytepool_mutex_timeout,
                             UINT (*get_clock_time)(CHAR *clock_time),
                             UINT (*get_process_ID)(CHAR *process_ID),
                             UINT (*authentication_check)(NX_POP3_SERVER_SESSION *session_ptr, CHAR *username, CHAR *password, UINT *result),
                             UINT (*get_client_maildrop)(NX_POP3_SERVER_SESSION *session_ptr, CHAR *username, UINT *maildrop_found),
                             UINT (*get_client_mailitem_data)(NX_POP3_SERVER_SESSION *session_ptr, UINT maildrop_index, ULONG *mailitem_bytes, UINT *maildrop_found),
                             UINT (*load_client_maildrop)(NX_POP3_SERVER  *server_ptr),
                             UINT (*get_mail_message_buffer)(NX_POP3_SERVER_SESSION *session_ptr, UINT mailitem_index, CHAR **buffer_ptr, UINT *bytes_extracted, UINT *bytes_remaining),
                             UINT (*delete_mail_on_file)(NX_POP3_SERVER_SESSION *session_ptr));
UINT    _nxe_pop3_server_create(NX_POP3_SERVER *server_ptr, NX_IP *ip_ptr, VOID *stack_ptr, 
                             ULONG stack_size, UINT server_priority, UINT server_preempt_threshold, 
                             UINT server_time_slice, UINT auto_start, NX_PACKET_POOL *packet_pool_ptr, 
                             TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *bytepool_mutex_ptr, UINT bytepool_mutex_timeout,
                             UINT (*get_clock_time)(CHAR *clock_time),
                             UINT (*get_process_ID)(CHAR *process_ID),
                             UINT (*authentication_check)(NX_POP3_SERVER_SESSION *session_ptr, CHAR *username, CHAR *password, UINT *result),
                             UINT (*get_client_maildrop)(NX_POP3_SERVER_SESSION *session_ptr, CHAR *username, UINT *maildrop_found),
                             UINT (*get_client_mailitem_data)(NX_POP3_SERVER_SESSION *session_ptr, UINT maildrop_index, ULONG *mailitem_bytes, UINT *maildrop_found),
                             UINT (*load_client_maildrop)(NX_POP3_SERVER  *server_ptr),
                             UINT (*get_mail_message_buffer)(NX_POP3_SERVER_SESSION *session_ptr, UINT mailitem_index, CHAR **buffer_ptr, UINT *bytes_extracted, UINT *bytes_remaining),
                             UINT (*delete_mail_on_file)(NX_POP3_SERVER_SESSION *session_ptr));
UINT    _nx_pop3_server_delete(NX_POP3_SERVER *server_ptr);
UINT    _nxe_pop3_server_delete(NX_POP3_SERVER *server_ptr); 
UINT    _nxe_pop3_server_session_reinitialize(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nx_pop3_server_session_reinitialize(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nx_pop3_server_get_time(NX_POP3_SERVER_SESSION *session_ptr, CHAR *clock_time);
UINT    _nxe_pop3_server_get_time(NX_POP3_SERVER_SESSION *session_ptr, CHAR *clock_time);
UINT    _nxe_pop3_server_get_PID(NX_POP3_SERVER_SESSION *session_ptr, CHAR *process_ID);
UINT    _nx_pop3_server_get_PID(NX_POP3_SERVER_SESSION *session_ptr, CHAR *process_ID);
UINT    _nxe_pop3_server_get_auth_check(NX_POP3_SERVER_SESSION *session_ptr, CHAR *username_ptr, CHAR *password_ptr, UINT *authenticated);
UINT    _nx_pop3_server_get_auth_check(NX_POP3_SERVER_SESSION *session_ptr, CHAR *username_ptr, CHAR *password_ptr, UINT *authenticated);
UINT    _nx_pop3_server_start(NX_POP3_SERVER *server_ptr);
UINT    _nxe_pop3_server_start(NX_POP3_SERVER *server_ptr);
UINT    _nx_pop3_server_stop(NX_POP3_SERVER *server_ptr);
UINT    _nxe_pop3_server_stop(NX_POP3_SERVER *server_ptr);
UINT    _nx_pop3_server_session_create(NX_POP3_SERVER *server_ptr, NX_POP3_SERVER_SESSION *session_ptr, UINT session_id, 
                                      VOID *session_stack_ptr, ULONG session_stack_size, UINT session_priority, 
                                      UINT session_preempt_threshold, ULONG session_time_slice, UINT session_auto_start); 
UINT    _nxe_pop3_server_session_create(NX_POP3_SERVER *server_ptr, NX_POP3_SERVER_SESSION *session_ptr, UINT session_id, 
                                      VOID *session_stack_ptr, ULONG session_stack_size, UINT session_priority, 
                                      UINT session_preempt_threshold, ULONG session_time_slice, UINT session_auto_start); 
UINT    _nxe_pop3_server_session_delete(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nx_pop3_server_session_delete(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nxe_pop3_server_session_run(NX_POP3_SERVER_SESSION  *session_ptr);
UINT    _nx_pop3_server_session_run(NX_POP3_SERVER_SESSION  *session_ptr);
UINT    _nx_pop3_reply_to_greeting(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nxe_pop3_reply_to_greeting(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nx_pop3_reply_to_user(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nxe_pop3_reply_to_apop(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nx_pop3_reply_to_apop(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nxe_pop3_reply_to_pass(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nx_pop3_reply_to_pass(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nxe_pop3_reply_to_stat(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nx_pop3_reply_to_stat(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nxe_pop3_reply_to_retr(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nx_pop3_reply_to_retr(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nxe_pop3_reply_to_dele(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nx_pop3_reply_to_dele(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nxe_pop3_reply_to_list(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nx_pop3_reply_to_list(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nxe_pop3_reply_to_rset(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nx_pop3_reply_to_rset(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nxe_pop3_reply_to_quit(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nx_pop3_reply_to_quit(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nxe_pop3_reply_to_noop(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nx_pop3_reply_to_noop(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nxe_pop3_server_print_server_reserves(NX_POP3_SERVER *server_ptr);
UINT    _nx_pop3_server_print_server_reserves(NX_POP3_SERVER *server_ptr);

/* Define internal POP3 Server functions.  */

UINT    _nx_pop3_server_bytepool_memory_get(VOID **memory_ptr, ULONG memory_size, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);
UINT    _nx_pop3_server_bytepool_memory_release(VOID *memory_ptr, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);
UINT    _nx_pop3_server_create_APOP_string(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nx_pop3_server_create_client_mail_list(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nx_pop3_server_delete_client_mail_list(NX_POP3_SERVER_SESSION *session_ptr);
UINT    _nx_pop3_server_digest_authenticate(NX_POP3_SERVER_SESSION *session_ptr, CHAR *received_response, UINT *authenticated);
VOID    _nx_pop3_server_find_crlf(CHAR *buffer, UINT length, CHAR **CRLF, UINT reverse);
UINT    _nx_pop3_server_get_client_command(CHAR *buffer_ptr, UINT *protocol_code, UINT length);
VOID    _nx_pop3_server_hex_ascii_convert(CHAR *source, UINT source_length, CHAR *destination);
UINT    _nx_pop3_server_login_authenticate(NX_POP3_SERVER_SESSION *session_ptr, UINT command_code, UINT *result);
UINT    _nx_pop3_server_mail_delete(NX_POP3_SERVER_SESSION *session_ptr, NX_POP3_SERVER_MAIL *mail_ptr);
UINT    _nx_pop3_server_parse_maildrop_index(NX_POP3_SERVER_SESSION *session_ptr, UINT *maildrop_index, UINT maildrop_index_required);
VOID    _nx_pop3_server_parse_response(CHAR *buffer, UINT argument_index, UINT buffer_length, CHAR *argument, UINT argument_length, UINT convert_to_uppercase, UINT crlf_are_word_breaks);
UINT    _nx_pop3_server_send_to_client(NX_POP3_SERVER_SESSION *session_ptr, CHAR *buffer_ptr, UINT buffer_length, ULONG timeout);
UINT    _nx_pop3_server_session_maildrop_add_mail(NX_POP3_SERVER_SESSION *session_ptr, NX_POP3_SERVER_MAIL *mail_ptr);
VOID    _nx_pop3_server_session_thread_entry(ULONG info);
VOID    _nx_pop3_server_thread_entry(ULONG info);
VOID    _nx_pop3_session_connection_present(NX_TCP_SOCKET *socket_ptr, UINT port);
VOID    _nx_pop3_session_disconnect_present(NX_TCP_SOCKET *socket_ptr);


#endif

/* If a C++ compiler is being used....*/
#ifdef   __cplusplus
        }
#endif

#endif /* __NX_POP3_SERVER_H__  */

