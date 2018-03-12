/**************************************************************************/ 
/*                                                                        */ 
/*            Copyright (c) 1996-2013 by Express Logic Inc.               */ 
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
/**   File Transfer Protocol (FTP)                                        */ 
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  APPLICATION INTERFACE DEFINITION                       RELEASE        */ 
/*                                                                        */ 
/*    nx_ftp.h                                            PORTABLE C      */ 
/*                                                          5.3           */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    William E. Lamie, Express Logic, Inc.                               */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the NetX File Transfer Protocol (FTP)             */ 
/*    component, including all data types and external references.        */ 
/*    It is assumed that nx_api.h and nx_port.h have already been         */ 
/*    included, along with fx_api.h and fx_port.h.                        */
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  03-01-2006     William E. Lamie         Initial Version 5.0           */ 
/*  04-01-2010     Janet Christiansen       Modified comment(s), and      */
/*                                            added new constants,        */ 
/*                                            resulting in version 5.1    */
/*  07-15-2011     Janet Christiansen       Modified comment(s), and      */
/*                                            added login failed error,   */ 
/*                                            resulting in version 5.2    */
/*  04-30-2013     Janet Christiansen       Modified comment(s),          */
/*                                            resulting in version 5.3    */
/*                                                                        */ 
/**************************************************************************/ 

#ifndef NX_FTP_H
#define NX_FTP_H

/* Determine if a C++ compiler is being used.  If so, ensure that standard
   C is used to process the API information.  */

#ifdef   __cplusplus

/* Yes, C++ compiler is present.  Use standard C.  */
extern   "C" {

#endif


/* Define the FTP ID.  */

#define NX_FTP_ID                           0x46545200UL


/* Define the maximum number of clients the FTP Server can accommodate.  */

#ifndef NX_FTP_MAX_CLIENTS
#define NX_FTP_MAX_CLIENTS                  4
#endif


/* Define FTP TCP socket create options.  */

#ifndef NX_FTP_CONTROL_TOS
#define NX_FTP_CONTROL_TOS                  NX_IP_NORMAL
#endif

#ifndef NX_FTP_DATA_TOS
#define NX_FTP_DATA_TOS                     NX_IP_NORMAL
#endif

#ifndef NX_FTP_FRAGMENT_OPTION
#define NX_FTP_FRAGMENT_OPTION              NX_DONT_FRAGMENT
#endif  

#ifndef NX_FTP_CONTROL_WINDOW_SIZE
#define NX_FTP_CONTROL_WINDOW_SIZE          400
#endif

#ifndef NX_FTP_DATA_WINDOW_SIZE
#define NX_FTP_DATA_WINDOW_SIZE             2048  
#endif


#ifndef NX_FTP_TIME_TO_LIVE
#define NX_FTP_TIME_TO_LIVE                 0x80
#endif

#ifndef NX_FTP_SERVER_TIMEOUT       
#define NX_FTP_SERVER_TIMEOUT               100
#endif

#ifndef NX_FTP_SERVER_PRIORITY
#define NX_FTP_SERVER_PRIORITY              16
#endif

#ifndef NX_FTP_SERVER_TIME_SLICE
#define NX_FTP_SERVER_TIME_SLICE            2
#endif

#ifndef NX_FTP_USERNAME_SIZE
#define NX_FTP_USERNAME_SIZE                20
#endif

#ifndef NX_FTP_PASSWORD_SIZE
#define NX_FTP_PASSWORD_SIZE                20
#endif

#ifndef NX_FTP_ACTIVITY_TIMEOUT       
#define NX_FTP_ACTIVITY_TIMEOUT             240         /* Seconds allowed with no activity                    */ 
#endif

#ifndef NX_FTP_TIMEOUT_PERIOD
#define NX_FTP_TIMEOUT_PERIOD               60          /* Number of seconds to check                          */
#endif

#ifndef NX_PHYSICAL_TRAILER
#define NX_PHYSICAL_TRAILER                 4           /* Number of bytes to reserve at the end of buffer     */ 
#endif

/* Define the FTP data port retry parameters.  */

#ifndef NX_FTP_SERVER_RETRY_SECONDS
#define NX_FTP_SERVER_RETRY_SECONDS         2           /* 2 second initial timeout                            */ 
#endif

#ifndef NX_FTP_SERVER_TRANSMIT_QUEUE_DEPTH
#define NX_FTP_SERVER_TRANSMIT_QUEUE_DEPTH  20          /* Maximum of 20 queued transmit packets               */ 
#endif

#ifndef NX_FTP_SERVER_RETRY_MAX
#define NX_FTP_SERVER_RETRY_MAX             10          /* Maximum of 10 retries per packet                    */ 
#endif

#ifndef NX_FTP_SERVER_RETRY_SHIFT
#define NX_FTP_SERVER_RETRY_SHIFT           1           /* Every retry is twice as long                        */
#endif


/* Define open types.  */

#define NX_FTP_OPEN_FOR_READ                0x01        /* FTP file open for reading                           */
#define NX_FTP_OPEN_FOR_WRITE               0x02        /* FTP file open for writing                           */ 


/* Define Server thread events.  */

#define NX_FTP_SERVER_CONNECT               0x01        /* FTP connection is present                           */
#define NX_FTP_SERVER_DATA_DISCONNECT       0x02        /* FTP data disconnection is present                   */ 
#define NX_FTP_SERVER_COMMAND               0x04        /* FTP client command is present                       */ 
#define NX_FTP_SERVER_DATA                  0x08        /* FTP client data is present                          */ 
#define NX_FTP_SERVER_ACTIVITY_TIMEOUT      0x10        /* FTP activity timeout check                          */ 
#define NX_FTP_SERVER_CONTROL_DISCONNECT    0x20        /* FTP client disconnect of control socket             */ 
#define NX_FTP_ANY_EVENT                    0x3F        /* Any FTP event                                       */


/* Define return code constants.  */

#define NX_FTP_ERROR                        0xD0        /* FTP internal error                                  */ 
#define NX_FTP_TIMEOUT                      0xD1        /* FTP timeout occurred                                */ 
#define NX_FTP_FAILED                       0xD2        /* FTP error                                           */ 
#define NX_FTP_NOT_CONNECTED                0xD3        /* FTP not connected error                             */ 
#define NX_FTP_NOT_DISCONNECTED             0xD4        /* FTP not disconnected error                          */ 
#define NX_FTP_NOT_OPEN                     0xD5        /* FTP not opened error                                */ 
#define NX_FTP_NOT_CLOSED                   0xD6        /* FTP not closed error                                */ 
#define NX_FTP_END_OF_FILE                  0xD7        /* FTP end of file status                              */ 
#define NX_FTP_END_OF_LISTING               0xD8        /* FTP end of directory listing status                 */ 
#define NX_FTP_100_CODE_NOT_RECEIVED        0xD9        /* FTP client command did not receive 1xx status       */
#define NX_FTP_200_CODE_NOT_RECEIVED        0xDA        /* FTP client command did not receive 2xx status       */
#define NX_FTP_300_CODE_NOT_RECEIVED        0xDB        /* FTP client command did not receive 3xx status       */
#define NX_FTP_ERROR_530_LOGIN_FAILED       0xDC        /* FTP client login failed; username/password no good  */
#define NX_FTP_NO_ROUTE_FOUND               0xDD        /* FTP client unable to extract valid packet interface */

/* Define FTP connection states.  */

#define NX_FTP_STATE_NOT_CONNECTED          1           /* FTP not connected                                   */ 
#define NX_FTP_STATE_CONNECTED              2           /* FTP connected                                       */ 
#define NX_FTP_STATE_OPEN                   3           /* FTP file open for reading                           */ 
#define NX_FTP_STATE_WRITE_OPEN             4           /* FTP file open for writing                           */ 


/* Define the FTP Server TCP port numbers.  */

#define NX_FTP_SERVER_CONTROL_PORT          21          /* Control Port for FTP server                         */
#define NX_FTP_SERVER_DATA_PORT             20          /* Data Port for FTP server                            */ 


/* Define the FTP basic commands.  The ASCII command will be parsed and converted to the numerical 
   representation shown below.  */

#define NX_FTP_NOOP                         0
#define NX_FTP_USER                         1
#define NX_FTP_PASS                         2
#define NX_FTP_QUIT                         3
#define NX_FTP_RETR                         4
#define NX_FTP_STOR                         5
#define NX_FTP_RNFR                         6
#define NX_FTP_RNTO                         7
#define NX_FTP_DELE                         8
#define NX_FTP_RMD                          9
#define NX_FTP_MKD                          10
#define NX_FTP_NLST                         11
#define NX_FTP_PORT                         12
#define NX_FTP_CWD                          13
#define NX_FTP_PWD                          14
#define NX_FTP_TYPE                         15
#define NX_FTP_LIST                         16
#define NX_FTP_CDUP                         17
#define NX_FTP_INVALID                      18


/* Define the basic FTP Client data structure.  */

typedef struct NX_FTP_CLIENT_STRUCT 
{
    ULONG           nx_ftp_client_id;                               /* FTP Client ID                       */
    CHAR           *nx_ftp_client_name;                             /* Name of this FTP client             */
    NX_IP          *nx_ftp_client_ip_ptr;                           /* Pointer to associated IP structure  */ 
    NX_PACKET_POOL *nx_ftp_client_packet_pool_ptr;                  /* Pointer to FTP client packet pool   */ 
    ULONG           nx_ftp_client_server_ip;                        /* Server's IP address                 */ 
    UINT            nx_ftp_client_server_data_port;                 /* Server's data port number           */ 
    UINT            nx_ftp_client_state;                            /* State of FTP client                 */ 
    NX_TCP_SOCKET   nx_ftp_client_control_socket;                   /* Client FTP control socket           */
    NX_TCP_SOCKET   nx_ftp_client_data_socket;                      /* Client FTP data transfer socket     */ 
} NX_FTP_CLIENT;


/* Define the per client request structure for the FTP Server data structure.  */

typedef struct NX_FTP_CLIENT_REQUEST_STRUCT
{
    UINT            nx_ftp_client_request_data_port;                /* Client's data port                  */ 
    UINT            nx_ftp_client_request_open_type;                /* Open type of client request         */
    UINT            nx_ftp_client_request_authenticated;            /* Authenticated flag                  */ 
    CHAR            nx_ftp_client_request_read_only;                /* Read-only flag                      */
    CHAR            nx_ftp_client_ftp_transfer_mode;                /* FTP Data transfer mode              */ 
    ULONG           nx_ftp_client_request_activity_timeout;         /* Timeout for client activity         */ 
    ULONG           nx_ftp_client_request_total_bytes;              /* Total bytes read or written         */ 
    CHAR            nx_ftp_client_request_username[NX_FTP_USERNAME_SIZE];
    CHAR            nx_ftp_client_request_password[NX_FTP_PASSWORD_SIZE];
    NX_PACKET       *nx_ftp_client_request_packet;                  /* Previous request packet             */ 
    FX_FILE         nx_ftp_client_request_file;                     /* File control block                  */ 
    FX_LOCAL_PATH   nx_ftp_client_local_path;                       /* Local path control block            */ 
    NX_TCP_SOCKET   nx_ftp_client_request_control_socket;           /* Client control socket               */ 
    NX_TCP_SOCKET   nx_ftp_client_request_data_socket;              /* Client data socket                  */ 
} NX_FTP_CLIENT_REQUEST;


/* Define the FTP Server data structure.  */

typedef struct NX_FTP_SERVER_STRUCT 
{
    ULONG           nx_ftp_server_id;                               /* FTP Server ID                       */
    CHAR           *nx_ftp_server_name;                             /* Name of this FTP server             */
    NX_IP          *nx_ftp_server_ip_ptr;                           /* Pointer to associated IP structure  */ 
    NX_PACKET_POOL *nx_ftp_server_packet_pool_ptr;                  /* Pointer to FTP server packet pool   */ 
    FX_MEDIA       *nx_ftp_server_media_ptr;                        /* Pointer to media control block      */ 
    ULONG           nx_ftp_server_connection_requests;              /* Number of connection requests       */ 
    ULONG           nx_ftp_server_disconnection_requests;           /* Number of disconnection requests    */ 
    ULONG           nx_ftp_server_login_errors;                     /* Number of login errors              */ 
    ULONG           nx_ftp_server_authentication_errors;            /* Number of access w/o authentication */ 
    ULONG           nx_ftp_server_total_bytes_sent;                 /* Number of total bytes sent          */ 
    ULONG           nx_ftp_server_total_bytes_received;             /* Number of total bytes received      */ 
    ULONG           nx_ftp_server_unknown_commands;                 /* Number of unknown commands received */ 
    ULONG           nx_ftp_server_allocation_errors;                /* Number of allocation errors         */ 
    ULONG           nx_ftp_server_relisten_errors;                  /* Number of relisten errors           */ 
    ULONG           nx_ftp_server_activity_timeouts;                /* Number of activity timeouts         */ 
    NX_FTP_CLIENT_REQUEST                                           /* FTP client request array            */ 
                    nx_ftp_server_client_list[NX_FTP_MAX_CLIENTS]; 
    TX_EVENT_FLAGS_GROUP
                    nx_ftp_server_event_flags;                      /* FTP server thread events            */ 
    TX_TIMER        nx_ftp_server_timer;                            /* FTP server activity timeout timer   */ 
    TX_THREAD       nx_ftp_server_thread;                           /* FTP server thread                   */ 
    UINT            (*nx_ftp_login)(struct NX_FTP_SERVER_STRUCT *ftp_server_ptr, ULONG client_ip_address, UINT client_port, CHAR *name, CHAR *password, CHAR *extra_info); 
    UINT            (*nx_ftp_logout)(struct NX_FTP_SERVER_STRUCT *ftp_server_ptr, ULONG client_ip_address, UINT client_port, CHAR *name, CHAR *password, CHAR *extra_info);

    struct NX_FTP_SERVER_STRUCT
                    *nx_ftp_next_server_ptr,
                    *nx_ftp_previous_server_ptr;
} NX_FTP_SERVER;


#ifndef NX_FTP_SOURCE_CODE

/* Application caller is present, perform API mapping.  */

/* Determine if error checking is desired.  If so, map API functions 
   to the appropriate error checking front-ends.  Otherwise, map API
   functions to the core functions that actually perform the work. 
   Note: error checking is enabled by default.  */

#ifdef NX_DISABLE_ERROR_CHECKING

/* Services without error checking.  */

#define nx_ftp_client_connect                       _nx_ftp_client_connect
#define nx_ftp_client_create                        _nx_ftp_client_create
#define nx_ftp_client_delete                        _nx_ftp_client_delete
#define nx_ftp_client_directory_create              _nx_ftp_client_directory_create
#define nx_ftp_client_directory_default_set         _nx_ftp_client_directory_default_set
#define nx_ftp_client_directory_delete              _nx_ftp_client_directory_delete
#define nx_ftp_client_directory_listing_get         _nx_ftp_client_directory_listing_get
#define nx_ftp_client_directory_listing_continue    _nx_ftp_client_directory_listing_continue
#define nx_ftp_client_disconnect                    _nx_ftp_client_disconnect
#define nx_ftp_client_file_close                    _nx_ftp_client_file_close
#define nx_ftp_client_file_delete                   _nx_ftp_client_file_delete
#define nx_ftp_client_file_open                     _nx_ftp_client_file_open
#define nx_ftp_client_file_read                     _nx_ftp_client_file_read
#define nx_ftp_client_file_rename                   _nx_ftp_client_file_rename
#define nx_ftp_client_file_write                    _nx_ftp_client_file_write
#define nx_ftp_server_create                        _nx_ftp_server_create
#define nx_ftp_server_delete                        _nx_ftp_server_delete
#define nx_ftp_server_start                         _nx_ftp_server_start
#define nx_ftp_server_stop                          _nx_ftp_server_stop

#else

/* Services with error checking.  */

#define nx_ftp_client_connect                       _nxe_ftp_client_connect
#define nx_ftp_client_create                        _nxe_ftp_client_create
#define nx_ftp_client_delete                        _nxe_ftp_client_delete
#define nx_ftp_client_directory_create              _nxe_ftp_client_directory_create
#define nx_ftp_client_directory_default_set         _nxe_ftp_client_directory_default_set
#define nx_ftp_client_directory_delete              _nxe_ftp_client_directory_delete
#define nx_ftp_client_directory_listing_get         _nxe_ftp_client_directory_listing_get
#define nx_ftp_client_directory_listing_continue    _nxe_ftp_client_directory_listing_continue
#define nx_ftp_client_disconnect                    _nxe_ftp_client_disconnect
#define nx_ftp_client_file_close                    _nxe_ftp_client_file_close
#define nx_ftp_client_file_delete                   _nxe_ftp_client_file_delete
#define nx_ftp_client_file_open                     _nxe_ftp_client_file_open
#define nx_ftp_client_file_read                     _nxe_ftp_client_file_read
#define nx_ftp_client_file_rename                   _nxe_ftp_client_file_rename
#define nx_ftp_client_file_write                    _nxe_ftp_client_file_write
#define nx_ftp_server_create                        _nxe_ftp_server_create
#define nx_ftp_server_delete                        _nxe_ftp_server_delete
#define nx_ftp_server_start                         _nxe_ftp_server_start
#define nx_ftp_server_stop                          _nxe_ftp_server_stop

#endif

/* Define the prototypes accessible to the application software.  */

UINT        nx_ftp_client_connect(NX_FTP_CLIENT *ftp_client_ptr, ULONG server_ip, CHAR *username, CHAR *password, ULONG wait_option);
UINT        nx_ftp_client_create(NX_FTP_CLIENT *ftp_client_ptr, CHAR *ftp_client_name, NX_IP *ip_ptr, ULONG window_size, NX_PACKET_POOL *pool_ptr);
UINT        nx_ftp_client_delete(NX_FTP_CLIENT *ftp_client_ptr);
UINT        nx_ftp_client_directory_create(NX_FTP_CLIENT *ftp_client_ptr, CHAR *directory_name, ULONG wait_option);
UINT        nx_ftp_client_directory_default_set(NX_FTP_CLIENT *ftp_client_ptr, CHAR *directory_path, ULONG wait_option);
UINT        nx_ftp_client_directory_delete(NX_FTP_CLIENT *ftp_client_ptr, CHAR *directory_name, ULONG wait_option);
UINT        nx_ftp_client_directory_listing_get(NX_FTP_CLIENT *ftp_client_ptr, CHAR *directory_path, NX_PACKET **packet_ptr, ULONG wait_option);
UINT        nx_ftp_client_directory_listing_continue(NX_FTP_CLIENT *ftp_client_ptr, NX_PACKET **packet_ptr, ULONG wait_option);
UINT        nx_ftp_client_disconnect(NX_FTP_CLIENT *ftp_client_ptr, ULONG wait_option);
UINT        nx_ftp_client_file_close(NX_FTP_CLIENT *ftp_client_ptr, ULONG wait_option);
UINT        nx_ftp_client_file_delete(NX_FTP_CLIENT *ftp_client_ptr, CHAR *file_name, ULONG wait_option);
UINT        nx_ftp_client_file_open(NX_FTP_CLIENT *ftp_client_ptr, CHAR *file_name, UINT open_type, ULONG wait_option);
UINT        nx_ftp_client_file_read(NX_FTP_CLIENT *ftp_client_ptr, NX_PACKET **packet_ptr, ULONG wait_option);
UINT        nx_ftp_client_file_rename(NX_FTP_CLIENT *ftp_ptr, CHAR *filename, CHAR *new_filename, ULONG wait_option);
UINT        nx_ftp_client_file_write(NX_FTP_CLIENT *ftp_client_ptr, NX_PACKET *packet_ptr, ULONG wait_option);
UINT        nx_ftp_server_create(NX_FTP_SERVER *ftp_server_ptr, CHAR *ftp_server_name, NX_IP *ip_ptr, FX_MEDIA *media_ptr, VOID *stack_ptr, ULONG stack_size, NX_PACKET_POOL *pool_ptr,
                UINT (*ftp_login)(struct NX_FTP_SERVER_STRUCT *ftp_server_ptr, ULONG client_ip_address, UINT client_port, CHAR *name, CHAR *password, CHAR *extra_info),
                UINT (*ftp_logout)(struct NX_FTP_SERVER_STRUCT *ftp_server_ptr, ULONG client_ip_address, UINT client_port, CHAR *name, CHAR *password, CHAR *extra_info));
UINT        nx_ftp_server_delete(NX_FTP_SERVER *ftp_server_ptr);
UINT        nx_ftp_server_start(NX_FTP_SERVER *ftp_server_ptr);
UINT        nx_ftp_server_stop(NX_FTP_SERVER *ftp_server_ptr);

#else

/* FTP source code is being compiled, do not perform any API mapping.  */

UINT        _nxe_ftp_client_connect(NX_FTP_CLIENT *ftp_client_ptr, ULONG server_ip, CHAR *username, CHAR *password, ULONG wait_option);
UINT        _nx_ftp_client_connect(NX_FTP_CLIENT *ftp_client_ptr, ULONG server_ip, CHAR *username, CHAR *password, ULONG wait_option);
UINT        _nxe_ftp_client_create(NX_FTP_CLIENT *ftp_client_ptr, CHAR *ftp_client_name, NX_IP *ip_ptr, ULONG window_size, NX_PACKET_POOL *pool_ptr);
UINT        _nx_ftp_client_create(NX_FTP_CLIENT *ftp_client_ptr, CHAR *ftp_client_name, NX_IP *ip_ptr, ULONG window_size, NX_PACKET_POOL *pool_ptr);
VOID        _nx_ftp_client_data_disconnect(NX_TCP_SOCKET *data_socket_ptr);
UINT        _nxe_ftp_client_delete(NX_FTP_CLIENT *ftp_client_ptr);
UINT        _nx_ftp_client_delete(NX_FTP_CLIENT *ftp_client_ptr);
UINT        _nxe_ftp_client_directory_create(NX_FTP_CLIENT *ftp_client_ptr, CHAR *directory_name, ULONG wait_option);
UINT        _nx_ftp_client_directory_create(NX_FTP_CLIENT *ftp_client_ptr, CHAR *directory_name, ULONG wait_option);
UINT        _nxe_ftp_client_directory_default_set(NX_FTP_CLIENT *ftp_client_ptr, CHAR *directory_path, ULONG wait_option);
UINT        _nx_ftp_client_directory_default_set(NX_FTP_CLIENT *ftp_client_ptr, CHAR *directory_path, ULONG wait_option);
UINT        _nxe_ftp_client_directory_delete(NX_FTP_CLIENT *ftp_client_ptr, CHAR *directory_name, ULONG wait_option);
UINT        _nx_ftp_client_directory_delete(NX_FTP_CLIENT *ftp_client_ptr, CHAR *directory_name, ULONG wait_option);
UINT        _nxe_ftp_client_directory_listing_get(NX_FTP_CLIENT *ftp_client_ptr, CHAR *directory_path, NX_PACKET **packet_ptr, ULONG wait_option);
UINT        _nx_ftp_client_directory_listing_get(NX_FTP_CLIENT *ftp_client_ptr, CHAR *directory_path, NX_PACKET **packet_ptr, ULONG wait_option);
UINT        _nxe_ftp_client_directory_listing_continue(NX_FTP_CLIENT *ftp_client_ptr, NX_PACKET **packet_ptr, ULONG wait_option);
UINT        _nx_ftp_client_directory_listing_continue(NX_FTP_CLIENT *ftp_client_ptr, NX_PACKET **packet_ptr, ULONG wait_option);
UINT        _nxe_ftp_client_disconnect(NX_FTP_CLIENT *ftp_client_ptr, ULONG wait_option);
UINT        _nx_ftp_client_disconnect(NX_FTP_CLIENT *ftp_client_ptr, ULONG wait_option);
UINT        _nxe_ftp_client_file_close(NX_FTP_CLIENT *ftp_client_ptr, ULONG wait_option);
UINT        _nx_ftp_client_file_close(NX_FTP_CLIENT *ftp_client_ptr, ULONG wait_option);
UINT        _nxe_ftp_client_file_delete(NX_FTP_CLIENT *ftp_client_ptr, CHAR *file_name, ULONG wait_option);
UINT        _nx_ftp_client_file_delete(NX_FTP_CLIENT *ftp_client_ptr, CHAR *file_name, ULONG wait_option);
UINT        _nxe_ftp_client_file_open(NX_FTP_CLIENT *ftp_client_ptr, CHAR *file_name, UINT open_type, ULONG wait_option);
UINT        _nx_ftp_client_file_open(NX_FTP_CLIENT *ftp_client_ptr, CHAR *file_name, UINT open_type, ULONG wait_option);
UINT        _nxe_ftp_client_file_read(NX_FTP_CLIENT *ftp_client_ptr, NX_PACKET **packet_ptr, ULONG wait_option);
UINT        _nx_ftp_client_file_read(NX_FTP_CLIENT *ftp_client_ptr, NX_PACKET **packet_ptr, ULONG wait_option);
UINT        _nxe_ftp_client_file_rename(NX_FTP_CLIENT *ftp_client_ptr, CHAR *filename, CHAR *new_filename, ULONG wait_option);
UINT        _nx_ftp_client_file_rename(NX_FTP_CLIENT *ftp_ptr, CHAR *filename, CHAR *new_filename, ULONG wait_option);
UINT        _nxe_ftp_client_file_write(NX_FTP_CLIENT *ftp_client_ptr, NX_PACKET *packet_ptr, ULONG wait_option);
UINT        _nx_ftp_client_file_write(NX_FTP_CLIENT *ftp_client_ptr, NX_PACKET *packet_ptr, ULONG wait_option);
UINT        _nxe_ftp_server_create(NX_FTP_SERVER *ftp_server_ptr, CHAR *ftp_server_name, NX_IP *ip_ptr, FX_MEDIA *media_ptr, VOID *stack_ptr, ULONG stack_size, NX_PACKET_POOL *pool_ptr,
                UINT (*ftp_login)(struct NX_FTP_SERVER_STRUCT *ftp_server_ptr, ULONG client_ip_address, UINT client_port, CHAR *name, CHAR *password, CHAR *extra_info),
                UINT (*ftp_logout)(struct NX_FTP_SERVER_STRUCT *ftp_server_ptr, ULONG client_ip_address, UINT client_port, CHAR *name, CHAR *password, CHAR *extra_info));
UINT        _nx_ftp_server_create(NX_FTP_SERVER *ftp_server_ptr, CHAR *ftp_server_name, NX_IP *ip_ptr, FX_MEDIA *media_ptr, VOID *stack_ptr, ULONG stack_size, NX_PACKET_POOL *pool_ptr,
                UINT (*ftp_login)(struct NX_FTP_SERVER_STRUCT *ftp_server_ptr, ULONG client_ip_address, UINT client_port, CHAR *name, CHAR *password, CHAR *extra_info),
                UINT (*ftp_logout)(struct NX_FTP_SERVER_STRUCT *ftp_server_ptr, ULONG client_ip_address, UINT client_port, CHAR *name, CHAR *password, CHAR *extra_info));
UINT        _nxe_ftp_server_delete(NX_FTP_SERVER *ftp_server_ptr);
UINT        _nx_ftp_server_delete(NX_FTP_SERVER *ftp_server_ptr);
UINT        _nxe_ftp_server_start(NX_FTP_SERVER *ftp_server_ptr);
UINT        _nx_ftp_server_start(NX_FTP_SERVER *ftp_server_ptr);
UINT        _nxe_ftp_server_stop(NX_FTP_SERVER *ftp_server_ptr);
UINT        _nx_ftp_server_stop(NX_FTP_SERVER *ftp_server_ptr);
VOID        _nx_ftp_server_response(NX_TCP_SOCKET *socket, NX_PACKET *packet_ptr, const CHAR *reply_code, const CHAR *message);
VOID        _nx_ftp_server_directory_response(NX_TCP_SOCKET *socket, NX_PACKET *packet_ptr, const CHAR *reply_code, const CHAR *message, const CHAR *directory);
VOID        _nx_ftp_server_thread_entry(ULONG ftp_server_address);
VOID        _nx_ftp_server_command_process(NX_FTP_SERVER *ftp_server_ptr);
VOID        _nx_ftp_server_connect_process(NX_FTP_SERVER *ftp_server_ptr);
VOID        _nx_ftp_server_command_present(NX_TCP_SOCKET *control_socket_ptr);
VOID        _nx_ftp_server_connection_present(NX_TCP_SOCKET *control_socket_ptr, UINT port);
VOID        _nx_ftp_server_data_disconnect(NX_TCP_SOCKET *data_socket_ptr);
VOID        _nx_ftp_server_data_disconnect_process(NX_FTP_SERVER *ftp_server_ptr);
VOID        _nx_ftp_server_data_present(NX_TCP_SOCKET *data_socket_ptr);
VOID        _nx_ftp_server_data_process(NX_FTP_SERVER *ftp_server_ptr);
UINT        _nx_ftp_server_parse_command(NX_PACKET *packet_ptr);
VOID        _nx_ftp_server_timeout(ULONG ftp_server_address);
VOID        _nx_ftp_server_timeout_processing(NX_FTP_SERVER *ftp_server_ptr);
VOID        _nx_ftp_server_control_disconnect(NX_TCP_SOCKET *control_socket_ptr);
VOID        _nx_ftp_server_control_disonnect_processing(NX_FTP_SERVER *ftp_server_ptr);

#ifdef  NX_FTP_ALTERNATE_SNPRINTF
int     _nx_ftp_snprintf(char *str, size_t size, const char *format, ...);
#else
#define _nx_ftp_snprintf snprintf
#endif /* ifdef  NX_FTP_ALTERNATE_SNPRINTF */

#endif

/* Determine if a C++ compiler is being used.  If so, complete the standard
   C conditional started above.  */
#ifdef   __cplusplus
        }
#endif

#endif  
