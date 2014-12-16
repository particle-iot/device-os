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
/**   Trivial File Transfer Protocol (TFTP)                               */ 
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  APPLICATION INTERFACE DEFINITION                       RELEASE        */ 
/*                                                                        */ 
/*    nx_tftp.h                                           PORTABLE C      */ 
/*                                                           5.3          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    William E. Lamie, Express Logic, Inc.                               */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the NetX Trivial File Transfer Protocol (TFTP)    */ 
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
/*                                            changed constants, resulting*/ 
/*                                            in version 5.1              */ 
/*  07-11-2011     Janet Christiansen       Modified comment(s),          */ 
/*                                            resulting in version 5.2    */ 
/*  04-30-2013     Janet Christiansen       Modified comment(s),          */
/*                                            resulting in version 5.3    */
/*                                                                        */ 
/**************************************************************************/ 

#ifndef NX_TFTP_H
#define NX_TFTP_H

/* Determine if a C++ compiler is being used.  If so, ensure that standard
   C is used to process the API information.  */

#ifdef   __cplusplus

/* Yes, C++ compiler is present.  Use standard C.  */
extern   "C" {

#endif

/* Define the TFTP ID.  */

#define NX_TFTP_ID                          0x54465450UL


#ifndef      NX_TFTP_NO_FILEX
#include    "fx_api.h"
#else
#include    "filex_stub.h"
#endif


/* Define TFTP maximum error string.  */

#ifndef NX_TFTP_ERROR_STRING_MAX
#define NX_TFTP_ERROR_STRING_MAX            64          /* Maximum error sting size                             */
#endif


/* Define the maximum number of clients the TFTP Server can accommodate.  */

#ifndef NX_TFTP_MAX_CLIENTS
#define NX_TFTP_MAX_CLIENTS                 10
#endif


/* Define TFTP UDP socket create options.  */

#ifndef NX_TFTP_TYPE_OF_SERVICE
#define NX_TFTP_TYPE_OF_SERVICE             NX_IP_NORMAL
#endif

#ifndef NX_TFTP_FRAGMENT_OPTION
#define NX_TFTP_FRAGMENT_OPTION             NX_DONT_FRAGMENT
#endif  

#ifndef NX_TFTP_TIME_TO_LIVE
#define NX_TFTP_TIME_TO_LIVE                0x80
#endif

#ifndef NX_PHYSICAL_TRAILER        
#define NX_PHYSICAL_TRAILER                 4
#endif

#ifndef NX_TFTP_SERVER_PRIORITY
#define NX_TFTP_SERVER_PRIORITY             16
#endif

#ifndef NX_TFTP_SERVER_TIME_SLICE
#define NX_TFTP_SERVER_TIME_SLICE           2
#endif

#define NX_TFTP_QUEUE_DEPTH                 5
        
#define NX_TFTP_FILE_TRANSFER_MAX           512         /* 512 byte maximum file transfer                       */


/* Derive the maximum TFTP packet size, accounting for potential physical driver needs at the end of the packet.*/

#define NX_TFPT_PACKET_SIZE                 (NX_UDP_PACKET+NX_TFTP_FILE_TRANSFER_MAX+NX_PHYSICAL_TRAILER) 


/* Define open types.  */

#define NX_TFTP_OPEN_FOR_READ               0x01        /* TFTP open for reading                                */
#define NX_TFTP_OPEN_FOR_WRITE              0x02        /* TFTP open for writing                                */ 


/* Define TFTP message codes.  */

#define NX_TFTP_CODE_READ                   0x01        /* TFTP read file request                               */ 
#define NX_TFTP_CODE_WRITE                  0x02        /* TFTP write file request                              */ 
#define NX_TFTP_CODE_DATA                   0x03        /* TFTP data packet                                     */ 
#define NX_TFTP_CODE_ACK                    0x04        /* TFTP command/data acknowledgement                    */ 
#define NX_TFTP_CODE_ERROR                  0x05        /* TFTP error message                                   */ 


/* Define TFTP error code constants.  */

#define NX_TFTP_ERROR_NOT_DEFINED           0x00        /* TFTP not defined error code, see error string        */
#define NX_TFTP_ERROR_FILE_NOT_FOUND        0x01        /* TFTP file not found error code                       */ 
#define NX_TFTP_ERROR_ACCESS_VIOLATION      0x02        /* TFTP file access violation error code                */ 
#define NX_TFTP_ERROR_DISK_FULL             0x03        /* TFTP disk full error code                            */ 
#define NX_TFTP_ERROR_ILLEGAL_OPERATION     0x04        /* TFTP illegal operation error code                    */ 
#define NX_TFTP_CODE_ERROR                  0x05        /* TFTP client request received error code from server  */ 
#define NX_TFTP_ERROR_FILE_EXISTS           0x06        /* TFTP file already exists error code                  */ 
#define NX_TFTP_ERROR_NO_SUCH_USER          0x07        /* TFTP no such user error code                         */ 
#define NX_INVALID_TFTP_SERVER_ADDRESS      0x08        /* Invalid TFTP server IP extraced from received packet */
#define NX_TFTP_NO_ACK_RECEIVED             0x09        /* Did not receive TFTP server ACK response             */
#define NX_TFTP_INVALID_BLOCK_NUMBER        0x0A        /* Invalid block number received from Server response   */


/* Define offsets into the TFTP message buffer.  */

#define NX_TFTP_CODE_OFFSET                 0           /* Offset to TFTP code in buffer                        */
#define NX_TFTP_FILENAME_OFFSET             2           /* Offset to TFTP filename in message                   */ 
#define NX_TFTP_BLOCK_NUMBER_OFFSET         2           /* Offset to TFTP block number in buffer                */ 
#define NX_TFTP_DATA_OFFSET                 4           /* Offset to TFTP data in buffer                        */ 
#define NX_TFTP_ERROR_CODE_OFFSET           2           /* Offset to TFTP error code                            */ 
#define NX_TFTP_ERROR_STRING_OFFSET         4           /* Offset to TFPT error string                          */ 


/* Define return code constants.  */

#define NX_TFTP_ERROR                       0xC0        /* TFTP internal error                                  */ 
#define NX_TFTP_TIMEOUT                     0xC1        /* TFTP timeout occurred                                */ 
#define NX_TFTP_FAILED                      0xC2        /* TFTP error                                           */ 
#define NX_TFTP_NOT_OPEN                    0xC3        /* TFTP not opened error                                */ 
#define NX_TFTP_NOT_CLOSED                  0xC4        /* TFTP not closed error                                */ 
#define NX_TFTP_END_OF_FILE                 0xC5        /* TFTP end of file error                               */ 
#define NX_TFTP_POOL_ERROR                  0xC6        /* TFTP packet pool size error - less than 560 bytes    */ 


/* Define TFTP connection states.  */

#define NX_TFTP_STATE_NOT_OPEN              0           /* TFTP connection not open                             */ 
#define NX_TFTP_STATE_OPEN                  1           /* TFTP connection open                                 */ 
#define NX_TFTP_STATE_WRITE_OPEN            2           /* TFTP connection open for writing                     */ 
#define NX_TFTP_STATE_END_OF_FILE           3           /* TFTP connection at end of file                       */ 
#define NX_TFTP_STATE_ERROR                 4           /* TFTP error condition                                 */ 
#define NX_TFTP_STATE_FINISHED              5           /* TFTP finished writing condition                      */ 


/* Define the TFTP Server UDP port number */

#define NX_TFTP_SERVER_PORT                 69          /* Port for TFTP server                                 */


/* Define the basic TFTP Client data structure.  */

typedef struct NX_TFTP_CLIENT_STRUCT 
{
    ULONG           nx_tftp_client_id;                              /* TFTP Client ID                       */
    CHAR           *nx_tftp_client_name;                            /* Name of this TFTP client             */
    NX_IP          *nx_tftp_client_ip_ptr;                          /* Pointer to associated IP structure   */ 
    NX_PACKET_POOL *nx_tftp_client_packet_pool_ptr;                 /* Pointer to TFTP client packet pool   */ 
    ULONG           nx_tftp_client_server_ip;                       /* Server's IP address                  */ 
    UINT            nx_tftp_client_server_port;                     /* Server's port number (69 originally) */ 
    UINT            nx_tftp_client_state;                           /* State of TFTP client                 */ 
    UINT            nx_tftp_client_block_number;                    /* Block number in file transfer        */ 
    UINT            nx_tftp_client_error_code;                      /* Error code received                  */ 
    CHAR            nx_tftp_client_error_string[NX_TFTP_ERROR_STRING_MAX];
    NX_UDP_SOCKET   nx_tftp_client_socket;                          /* TFPT Socket                          */

} NX_TFTP_CLIENT;


/* Define the per client request structure for the TFTP Server data structure.  */

typedef struct NX_TFTP_CLIENT_REQUEST_STRUCT
{
    UINT            nx_tftp_client_request_port;                    /* Port of client request               */
    ULONG           nx_tftp_client_request_ip_address;              /* IP address of client                 */ 
    UINT            nx_tftp_client_request_block_number;            /* Block number of file transfer        */ 
    UINT            nx_tftp_client_request_open_type;               /* Open type of client request          */
    ULONG           nx_tftp_client_request_remaining_bytes;         /* Remaining bytes to send              */ 
    UINT            nx_tftp_client_request_exact_fit;               /* Exact fit flag                       */ 
    ULONG           nx_tftp_client_request_last_activity_time;      /* Time of last activity                */ 
    FX_FILE         nx_tftp_client_request_file;                    /* File control block                   */ 
} NX_TFTP_CLIENT_REQUEST;


/* Define the TFTP Server data structure.  */

typedef struct NX_TFTP_SERVER_STRUCT 
{
    ULONG           nx_tftp_server_id;                              /* TFTP Server ID                       */
    CHAR           *nx_tftp_server_name;                            /* Name of this TFTP client             */
    NX_IP          *nx_tftp_server_ip_ptr;                          /* Pointer to associated IP structure   */ 
    NX_PACKET_POOL *nx_tftp_server_packet_pool_ptr;                 /* Pointer to TFTP server packet pool   */ 
    FX_MEDIA       *nx_tftp_server_media_ptr;                       /* Pointer to media control block       */ 
    ULONG           nx_tftp_server_open_for_write_requests;         /* Number of open for write requests    */ 
    ULONG           nx_tftp_server_open_for_read_requests;          /* Number of open for read requests     */ 
    ULONG           nx_tftp_server_acks_received;                   /* Number of ACKs received              */ 
    ULONG           nx_tftp_server_data_blocks_received;            /* Number of data blocks received       */ 
    ULONG           nx_tftp_server_errors_received;                 /* Number of errors received            */ 
    ULONG           nx_tftp_server_total_bytes_sent;                /* Number of total bytes sent           */ 
    ULONG           nx_tftp_server_total_bytes_received;            /* Number of total bytes received       */ 
    ULONG           nx_tftp_server_unknown_commands;                /* Number of unknown commands received  */ 
    ULONG           nx_tftp_server_allocation_errors;               /* Number of allocation errors          */ 
    ULONG           nx_tftp_server_clients_exceeded_errors;         /* Number of maximum clients errors     */ 
    ULONG           nx_tftp_server_unknown_clients_errors;          /* Number of unknown clients errors     */ 
    UINT            nx_tftp_server_error_code;                      /* Last error code received             */ 
    CHAR            nx_tftp_server_error_string[NX_TFTP_ERROR_STRING_MAX];
    NX_TFTP_CLIENT_REQUEST                                          /* TFTP client request array            */ 
                    nx_tftp_server_client_list[NX_TFTP_MAX_CLIENTS]; 
    NX_UDP_SOCKET   nx_tftp_server_socket;                          /* TFTP Server UDP socket               */ 
    TX_THREAD       nx_tftp_server_thread;                          /* TFTP server thread                   */ 

} NX_TFTP_SERVER;



#ifndef NX_TFTP_SOURCE_CODE

/* Application caller is present, perform API mapping.  */

/* Determine if error checking is desired.  If so, map API functions 
   to the appropriate error checking front-ends.  Otherwise, map API
   functions to the core functions that actually perform the work. 
   Note: error checking is enabled by default.  */

#ifdef NX_DISABLE_ERROR_CHECKING

/* Services without error checking.  */

#define nx_tftp_client_create           _nx_tftp_client_create
#define nx_tftp_client_delete           _nx_tftp_client_delete
#define nx_tftp_client_error_info_get   _nx_tftp_client_error_info_get
#define nx_tftp_client_file_close       _nx_tftp_client_file_close
#define nx_tftp_client_file_open        _nx_tftp_client_file_open
#define nx_tftp_client_file_read        _nx_tftp_client_file_read
#define nx_tftp_client_file_write       _nx_tftp_client_file_write
#define nx_tftp_client_packet_allocate  _nx_tftp_client_packet_allocate
#define nx_tftp_server_create           _nx_tftp_server_create
#define nx_tftp_server_delete           _nx_tftp_server_delete
#define nx_tftp_server_start            _nx_tftp_server_start
#define nx_tftp_server_stop             _nx_tftp_server_stop

#else

/* Services with error checking.  */

#define nx_tftp_client_create           _nxe_tftp_client_create
#define nx_tftp_client_delete           _nxe_tftp_client_delete
#define nx_tftp_client_error_info_get   _nxe_tftp_client_error_info_get
#define nx_tftp_client_file_close       _nxe_tftp_client_file_close
#define nx_tftp_client_file_open        _nxe_tftp_client_file_open
#define nx_tftp_client_file_read        _nxe_tftp_client_file_read
#define nx_tftp_client_file_write       _nxe_tftp_client_file_write
#define nx_tftp_client_packet_allocate  _nxe_tftp_client_packet_allocate
#define nx_tftp_server_create           _nxe_tftp_server_create
#define nx_tftp_server_delete           _nxe_tftp_server_delete
#define nx_tftp_server_start            _nxe_tftp_server_start
#define nx_tftp_server_stop             _nxe_tftp_server_stop

#endif

/* Define the prototypes accessible to the application software.  */

UINT        nx_tftp_client_create(NX_TFTP_CLIENT *tftp_client_ptr, CHAR *tftp_client_name, NX_IP *ip_ptr, NX_PACKET_POOL *pool_ptr);
UINT        nx_tftp_client_delete(NX_TFTP_CLIENT *tftp_client_ptr);
UINT        nx_tftp_client_error_info_get(NX_TFTP_CLIENT *tftp_client_ptr, UINT *error_code, CHAR **error_string);
UINT        nx_tftp_client_file_close(NX_TFTP_CLIENT *tftp_client_ptr);
UINT        nx_tftp_client_file_open(NX_TFTP_CLIENT *tftp_client_ptr, CHAR *file_name, ULONG server_ip_address, UINT open_type, ULONG wait_option);
UINT        nx_tftp_client_file_read(NX_TFTP_CLIENT *tftp_client_ptr, NX_PACKET **packet_ptr, ULONG wait_option);
UINT        nx_tftp_client_file_write(NX_TFTP_CLIENT *tftp_client_ptr, NX_PACKET *packet_ptr, ULONG wait_option);
UINT        nx_tftp_client_packet_allocate(NX_PACKET_POOL *pool_ptr, NX_PACKET **packet_ptr, ULONG wait_option);
UINT        nx_tftp_server_create(NX_TFTP_SERVER *tftp_server_ptr, CHAR *tftp_server_name, NX_IP *ip_ptr, FX_MEDIA *media_ptr, VOID *stack_ptr, ULONG stack_size, NX_PACKET_POOL *pool_ptr);
UINT        nx_tftp_server_delete(NX_TFTP_SERVER *tftp_server_ptr);
UINT        nx_tftp_server_start(NX_TFTP_SERVER *tftp_server_ptr);
UINT        nx_tftp_server_stop(NX_TFTP_SERVER *tftp_server_ptr);

#else

/* TFTP source code is being compiled, do not perform any API mapping.  */

UINT        _nxe_tftp_client_create(NX_TFTP_CLIENT *tftp_client_ptr, CHAR *tftp_client_name, NX_IP *ip_ptr, NX_PACKET_POOL *pool_ptr);
UINT        _nx_tftp_client_create(NX_TFTP_CLIENT *tftp_client_ptr, CHAR *tftp_client_name, NX_IP *ip_ptr, NX_PACKET_POOL *pool_ptr);
UINT        _nxe_tftp_client_delete(NX_TFTP_CLIENT *tftp_client_ptr);
UINT        _nx_tftp_client_delete(NX_TFTP_CLIENT *tftp_client_ptr);
UINT        _nxe_tftp_client_error_info_get(NX_TFTP_CLIENT *tftp_client_ptr, UINT *error_code, CHAR **error_string);
UINT        _nx_tftp_client_error_info_get(NX_TFTP_CLIENT *tftp_client_ptr, UINT *error_code, CHAR **error_string);
UINT        _nxe_tftp_client_file_close(NX_TFTP_CLIENT *tftp_client_ptr);
UINT        _nx_tftp_client_file_close(NX_TFTP_CLIENT *tftp_client_ptr);
UINT        _nxe_tftp_client_file_open(NX_TFTP_CLIENT *tftp_client_ptr, CHAR *file_name, ULONG server_ip_address, UINT open_type, ULONG wait_option);
UINT        _nx_tftp_client_file_open(NX_TFTP_CLIENT *tftp_client_ptr, CHAR *file_name, ULONG server_ip_address, UINT open_type, ULONG wait_option);
UINT        _nxe_tftp_client_file_read(NX_TFTP_CLIENT *tftp_client_ptr, NX_PACKET **packet_ptr, ULONG wait_option);
UINT        _nx_tftp_client_file_read(NX_TFTP_CLIENT *tftp_client_ptr, NX_PACKET **packet_ptr, ULONG wait_option);
UINT        _nxe_tftp_client_file_write(NX_TFTP_CLIENT *tftp_client_ptr, NX_PACKET *packet_ptr, ULONG wait_option);
UINT        _nx_tftp_client_file_write(NX_TFTP_CLIENT *tftp_client_ptr, NX_PACKET *packet_ptr, ULONG wait_option);
UINT        _nxe_tftp_client_packet_allocate(NX_PACKET_POOL *pool_ptr, NX_PACKET **packet_ptr, ULONG wait_option);
UINT        _nx_tftp_client_packet_allocate(NX_PACKET_POOL *pool_ptr, NX_PACKET **packet_ptr, ULONG wait_option);
UINT        _nxe_tftp_server_create(NX_TFTP_SERVER *tftp_server_ptr, CHAR *tftp_server_name, NX_IP *ip_ptr, FX_MEDIA *media_ptr, VOID *stack_ptr, ULONG stack_size, NX_PACKET_POOL *pool_ptr);
UINT        _nx_tftp_server_create(NX_TFTP_SERVER *tftp_server_ptr, CHAR *tftp_server_name, NX_IP *ip_ptr, FX_MEDIA *media_ptr, VOID *stack_ptr, ULONG stack_size, NX_PACKET_POOL *pool_ptr);
UINT        _nxe_tftp_server_delete(NX_TFTP_SERVER *tftp_server_ptr);
UINT        _nx_tftp_server_delete(NX_TFTP_SERVER *tftp_server_ptr);
UINT        _nxe_tftp_server_start(NX_TFTP_SERVER *tftp_server_ptr);
UINT        _nx_tftp_server_start(NX_TFTP_SERVER *tftp_server_ptr);
UINT        _nxe_tftp_server_stop(NX_TFTP_SERVER *tftp_server_ptr);
UINT        _nx_tftp_server_stop(NX_TFTP_SERVER *tftp_server_ptr);
VOID        _nx_tftp_server_thread_entry(ULONG tftp_server_address);
VOID        _nx_tftp_server_open_for_read_process(NX_TFTP_SERVER *server_ptr, NX_PACKET *packet_ptr);
VOID        _nx_tftp_server_open_for_write_process(NX_TFTP_SERVER *server_ptr, NX_PACKET *packet_ptr);
VOID        _nx_tftp_server_data_process(NX_TFTP_SERVER *server_ptr, NX_PACKET *packet_ptr);
VOID        _nx_tftp_server_ack_process(NX_TFTP_SERVER *server_ptr, NX_PACKET *packet_ptr);
VOID        _nx_tftp_server_error_process(NX_TFTP_SERVER *server_ptr, NX_PACKET *packet_ptr);
NX_TFTP_CLIENT_REQUEST 
            *_nx_tftp_server_find_client_request(NX_TFTP_SERVER *server_ptr, UINT port, ULONG ip_address);
VOID        _nx_tftp_server_send_error(NX_TFTP_SERVER *server_ptr, ULONG ip_address, UINT port, UINT error, const CHAR *error_message);

#endif

/* Determine if a C++ compiler is being used.  If so, complete the standard
   C conditional started above.  */
#ifdef   __cplusplus
        }
#endif

#endif  
