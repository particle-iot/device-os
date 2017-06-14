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
/** NetX Component                                                        */
/**                                                                       */
/**   TELNET Protocol (TELNET)                                            */ 
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  APPLICATION INTERFACE DEFINITION                       RELEASE        */ 
/*                                                                        */ 
/*    nxd_telnet.h                                         PORTABLE C     */ 
/*                                                           5.2          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    Janet Christiansen, Express Logic, Inc.                             */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the NetX Duo TELNET Protocol (TELNET) component,  */ 
/*    including all data types and external references.                   */ 
/*    It is assumed that nx_api.h and nx_port.h have already been         */ 
/*    included.                                                           */
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  10-15-2010     Janet Christiansen       Initial Version 5.0           */ 
/*  01-27-2012     Janet Christiansen       Modified comment(s),          */
/*                                            resulting in version 5.1    */
/*  03-07-2012     Janet Christiansen       Modified comment(s), and      */
/*                                            added support for open      */
/*                                            connection count feature,   */
/*                                            resulting in version 5.2    */
/*                                                                        */ 
/**************************************************************************/ 

#ifndef NXD_TELNET_SERVER_H
#define NXD_TELNET_SERVER_H

/* Determine if a C++ compiler is being used.  If so, ensure that standard
   C is used to process the API information.  */

#ifdef   __cplusplus

/* Yes, C++ compiler is present.  Use standard C.  */
extern   "C" {

#endif

/* Define the Server TELNET ID.  */

#define NX_TELNET_SERVER_ID                        0x54454C4EUL
                                                   

/* Define the maximum number of clients the TELNET Server can accommodate.  */

#ifndef NX_TELNET_MAX_CLIENTS
#define NX_TELNET_MAX_CLIENTS               4
#endif


/* Define TELNET TCP socket create options.  */

#ifndef NX_TELNET_TOS
#define NX_TELNET_TOS                       NX_IP_NORMAL
#endif

#ifndef NX_TELNET_FRAGMENT_OPTION
#define NX_TELNET_FRAGMENT_OPTION           NX_DONT_FRAGMENT
#endif  

#ifndef NX_TELNET_SERVER_WINDOW_SIZE
#define NX_TELNET_SERVER_WINDOW_SIZE        2048
#endif

#ifndef NX_TELNET_TIME_TO_LIVE
#define NX_TELNET_TIME_TO_LIVE              0x80
#endif

#ifndef NX_TELNET_SERVER_TIMEOUT       
#define NX_TELNET_SERVER_TIMEOUT            1000
#endif

#ifndef NX_TELNET_SERVER_PRIORITY
#define NX_TELNET_SERVER_PRIORITY           16
#endif

#ifndef NX_TELNET_MAX_OPTION_SIZE
#define NX_TELNET_MAX_OPTION_SIZE           20
#endif

#ifndef NX_TELNET_ACTIVITY_TIMEOUT       
#define NX_TELNET_ACTIVITY_TIMEOUT          600         /* Seconds allowed with no activity                     */ 
#endif

#ifndef NX_TELNET_TIMEOUT_PERIOD
#define NX_TELNET_TIMEOUT_PERIOD            60          /* Number of seconds to check                           */
#endif


/* Define TELNET commands that are optionally included in the TELNET data.  The application is responsible for
   recognizing and responding to the commands in accordance with the specification.  The TELNET option command
   requires three bytes, as follows:

        IAC, COMMAND, OPTION ID

*/

/* Define byte indicating TELNET command follows.  */

#define NX_TELNET_IAC                       255         /* TELNET Command byte - two consecutive -> 255 data    */

/* Define TELNET Negotiation Commands - Immediately follows IAC.  */

#define NX_TELNET_WILL                      251         /* TELNET WILL - Sender wants to enable the option      */ 
#define NX_TELNET_WONT                      252         /* TELNET WONT - Sender wants to disable the option     */
#define NX_TELNET_DO                        253         /* TELNET DO -   Sender wants receiver to enable option */ 
#define NX_TELNET_DONT                      254         /* TELNET DONT - Sender wants receiver to disable option*/ 



/* Define Server thread events.  */

#define NX_TELNET_SERVER_CONNECT            0x01        /* TELNET connection is present                         */
#define NX_TELNET_SERVER_DISCONNECT         0x02        /* TELNET disconnection is present                      */ 
#define NX_TELNET_SERVER_DATA               0x04        /* TELNET receive data is present                       */ 
#define NX_TELNET_SERVER_ACTIVITY_TIMEOUT   0x08        /* TELNET activity timeout check                        */ 
#define NX_TELNET_ANY_EVENT                 0x0F        /* Any TELNET event                                     */


/* Define return code constants.  */

#define NX_TELNET_ERROR                     0xF0        /* TELNET internal error                                */ 
#define NX_TELNET_TIMEOUT                   0xF1        /* TELNET timeout occurred                              */ 
#define NX_TELNET_FAILED                    0xF2        /* TELNET error                                         */ 
#define NX_TELNET_NOT_CONNECTED             0xF3        /* TELNET not connected error                           */ 
#define NX_TELNET_NOT_DISCONNECTED          0xF4        /* TELNET not disconnected error                        */ 
#define NX_TELNET_INVALID_PARAMETER         0xF5        /* Invalid non pointer input to Telnet function         */

/* Define the TELNET Server TCP port numbers.  */

#ifndef NX_TELNET_SERVER_PORT
#define NX_TELNET_SERVER_PORT               23          /* Default Port for TELNET server                       */
#endif


/* Define the per client request structure for the TELNET Server data structure.  */

typedef struct NX_TELNET_CLIENT_REQUEST_STRUCT
{
    UINT            nx_telnet_client_request_connection;                /* Logical connection number            */
    ULONG           nx_telnet_client_request_activity_timeout;          /* Timeout for client activity          */ 
    ULONG           nx_telnet_client_request_total_bytes;               /* Total bytes read or written          */ 
    CHAR            nx_telnet_client_request_option[NX_TELNET_MAX_OPTION_SIZE];
    NX_TCP_SOCKET   nx_telnet_client_request_socket;                    /* Client request socket                */ 
} NX_TELNET_CLIENT_REQUEST;


/* Define the TELNET Server data structure.  */

typedef struct NX_TELNET_SERVER_STRUCT 
{
    ULONG           nx_telnet_server_id;                               /* TELNET Server ID                      */
    CHAR           *nx_telnet_server_name;                             /* Name of this TELNET server            */
    NX_IP          *nx_telnet_server_ip_ptr;                           /* Pointer to associated IP structure    */ 
    ULONG           nx_telnet_server_connection_requests;              /* Number of connection requests         */ 
    ULONG           nx_telnet_server_disconnection_requests;           /* Number of disconnection requests      */ 
    ULONG           nx_telnet_server_total_bytes_sent;                 /* Number of total bytes sent            */ 
    ULONG           nx_telnet_server_total_bytes_received;             /* Number of total bytes received        */ 
    ULONG           nx_telnet_server_relisten_errors;                  /* Number of relisten errors             */ 
    ULONG           nx_telnet_server_activity_timeouts;                /* Number of activity timeouts           */ 
    ULONG           nx_telnet_server_open_connections;                 /* Number of currently open connections  */
    NX_TELNET_CLIENT_REQUEST                                           /* TELNET client request array           */ 
                    nx_telnet_server_client_list[NX_TELNET_MAX_CLIENTS]; 
    TX_EVENT_FLAGS_GROUP
                    nx_telnet_server_event_flags;                      /* TELNET server thread events           */ 
    TX_TIMER        nx_telnet_server_timer;                            /* TELNET server activity timeout timer  */ 
    TX_THREAD       nx_telnet_server_thread;                           /* TELNET server thread                  */ 
    void            (*nx_telnet_new_connection)(struct NX_TELNET_SERVER_STRUCT *telnet_server_ptr, UINT logical_connection); 
    void            (*nx_telnet_receive_data)(struct NX_TELNET_SERVER_STRUCT *telnet_server_ptr, UINT logical_connection, NX_PACKET *packet_ptr);
    void            (*nx_telnet_connection_end)(struct NX_TELNET_SERVER_STRUCT *telnet_server_ptr, UINT logical_connection);
} NX_TELNET_SERVER;


#ifndef NX_TELNET_SOURCE_CODE

/* Application caller is present, perform API mapping.  */

/* Determine if error checking is desired.  If so, map API functions 
   to the appropriate error checking front-ends.  Otherwise, map API
   functions to the core functions that actually perform the work. 
   Note: error checking is enabled by default.  */

#ifdef NX_DISABLE_ERROR_CHECKING

/* Services without error checking.  */

#define nx_telnet_server_create                     _nx_telnet_server_create
#define nx_telnet_server_delete                     _nx_telnet_server_delete
#define nx_telnet_server_disconnect                 _nx_telnet_server_disconnect
#define nx_telnet_server_packet_send                _nx_telnet_server_packet_send
#define nx_telnet_server_start                      _nx_telnet_server_start
#define nx_telnet_server_stop                       _nx_telnet_server_stop
#define nx_telnet_server_get_open_connection_count  _nx_telnet_server_get_open_connection_count

#else

/* Services with error checking.  */

#define nx_telnet_server_create                     _nxe_telnet_server_create
#define nx_telnet_server_delete                     _nxe_telnet_server_delete
#define nx_telnet_server_disconnect                 _nxe_telnet_server_disconnect
#define nx_telnet_server_packet_send                _nxe_telnet_server_packet_send
#define nx_telnet_server_start                      _nxe_telnet_server_start
#define nx_telnet_server_stop                       _nxe_telnet_server_stop
#define nx_telnet_server_get_open_connection_count  _nxe_telnet_server_get_open_connection_count

#endif

/* Define the prototypes accessible to the application software.  */

UINT    nx_telnet_server_create(NX_TELNET_SERVER *server_ptr, CHAR *server_name, NX_IP *ip_ptr, VOID *stack_ptr, ULONG stack_size, 
            void (*new_connection)(struct NX_TELNET_SERVER_STRUCT *telnet_server_ptr, UINT logical_connection), 
            void (*receive_data)(struct NX_TELNET_SERVER_STRUCT *telnet_server_ptr, UINT logical_connection, NX_PACKET *packet_ptr),
            void (*connection_end)(struct NX_TELNET_SERVER_STRUCT *telnet_server_ptr, UINT logical_connection));
UINT    nx_telnet_server_delete(NX_TELNET_SERVER *server_ptr);
UINT    nx_telnet_server_disconnect(NX_TELNET_SERVER *server_ptr, UINT logical_connection);
UINT    nx_telnet_server_packet_send(NX_TELNET_SERVER *server_ptr, UINT logical_connection, NX_PACKET *packet_ptr, ULONG wait_option);
UINT    nx_telnet_server_start(NX_TELNET_SERVER *server_ptr);
UINT    nx_telnet_server_stop(NX_TELNET_SERVER *server_ptr);
UINT    nx_telnet_server_get_open_connection_count(NX_TELNET_SERVER *server_ptr, UINT *current_connections);


#else

/* TELNET source code is being compiled, do not perform any API mapping.  */

UINT    _nxe_telnet_server_create(NX_TELNET_SERVER *server_ptr, CHAR *server_name, NX_IP *ip_ptr, VOID *stack_ptr, ULONG stack_size, 
            void (*new_connection)(struct NX_TELNET_SERVER_STRUCT *telnet_server_ptr, UINT logical_connection), 
            void (*receive_data)(struct NX_TELNET_SERVER_STRUCT *telnet_server_ptr, UINT logical_connection, NX_PACKET *packet_ptr),
            void (*connection_end)(struct NX_TELNET_SERVER_STRUCT *telnet_server_ptr, UINT logical_connection));
UINT    _nx_telnet_server_create(NX_TELNET_SERVER *server_ptr, CHAR *server_name, NX_IP *ip_ptr, VOID *stack_ptr, ULONG stack_size, 
            void (*new_connection)(struct NX_TELNET_SERVER_STRUCT *telnet_server_ptr, UINT logical_connection), 
            void (*receive_data)(struct NX_TELNET_SERVER_STRUCT *telnet_server_ptr, UINT logical_connection, NX_PACKET *packet_ptr),
            void (*connection_end)(struct NX_TELNET_SERVER_STRUCT *telnet_server_ptr, UINT logical_connection));
UINT    _nxe_telnet_server_delete(NX_TELNET_SERVER *server_ptr);
UINT    _nx_telnet_server_delete(NX_TELNET_SERVER *server_ptr);
UINT    _nxe_telnet_server_disconnect(NX_TELNET_SERVER *server_ptr, UINT logical_connection);
UINT    _nx_telnet_server_disconnect(NX_TELNET_SERVER *server_ptr, UINT logical_connection);
UINT    _nxe_telnet_server_packet_send(NX_TELNET_SERVER *server_ptr, UINT logical_connection, NX_PACKET *packet_ptr, ULONG wait_option);
UINT    _nx_telnet_server_packet_send(NX_TELNET_SERVER *server_ptr, UINT logical_connection, NX_PACKET *packet_ptr, ULONG wait_option);
UINT    _nxe_telnet_server_start(NX_TELNET_SERVER *server_ptr);
UINT    _nx_telnet_server_start(NX_TELNET_SERVER *server_ptr);
UINT    _nxe_telnet_server_stop(NX_TELNET_SERVER *server_ptr);
UINT    _nx_telnet_server_stop(NX_TELNET_SERVER *server_ptr);
UINT    _nxe_telnet_server_get_open_connection_count(NX_TELNET_SERVER *server_ptr, UINT *current_connections);
UINT    _nx_telnet_server_get_open_connection_count(NX_TELNET_SERVER *server_ptr, UINT *current_connections);

/* Define internal TELNET functions.  */

VOID    _nx_telnet_server_thread_entry(ULONG telnet_server);
VOID    _nx_telnet_server_connect_process(NX_TELNET_SERVER *server_ptr);
VOID    _nx_telnet_server_connection_present(NX_TCP_SOCKET *socket_ptr, UINT port);
VOID    _nx_telnet_server_disconnect_present(NX_TCP_SOCKET *socket_ptr);
VOID    _nx_telnet_server_disconnect_process(NX_TELNET_SERVER *server_ptr);
VOID    _nx_telnet_server_data_present(NX_TCP_SOCKET *socket_ptr);
VOID    _nx_telnet_server_data_process(NX_TELNET_SERVER *server_ptr);
VOID    _nx_telnet_server_timeout(ULONG telnet_server_address);
VOID    _nx_telnet_server_timeout_processing(NX_TELNET_SERVER *server_ptr);

#endif

/* Determine if a C++ compiler is being used.  If so, complete the standard
   C conditional started above.  */
#ifdef   __cplusplus
        }
#endif

#endif  /* NXD_TELNET_SERVER_H */
