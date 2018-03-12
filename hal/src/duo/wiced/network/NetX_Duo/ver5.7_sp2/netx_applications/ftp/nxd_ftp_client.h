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
/** NetX Duo Component                                                    */
/**                                                                       */
/**   File Transfer Protocol (FTP)                                        */ 
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  APPLICATION INTERFACE DEFINITION                       RELEASE        */ 
/*                                                                        */ 
/*    nxd_ftp_client.h                                    PORTABLE C      */ 
/*                                                           5.2          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    William E. Lamie, Express Logic, Inc.                               */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the NetX Duo File Transfer Protocol (FTP over     */ 
/*    IPv6) component, including all data types and external references.  */ 
/*    It is assumed that nx_api.h and nx_port.h have already been         */ 
/*    included, along with fx_api.h and fx_port.h.                        */
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  06-01-2010     Janet Christiansen       Initial version 5.0           */ 
/*  01-16-2012     Janet Christiansen       Modified comment(s) and       */
/*                                            added internal packet       */
/*                                            allocate function,          */
/*                                            resulting in version 5.1    */
/*  01-31-2013     Janet Christiansen       Modified comment(s)           */
/*                                            added client data socket    */
/*                                            port to increment data port */
/*                                            binding on successive       */
/*                                            connections,                */
/*                                            resulting in version 5.2    */
/*                                                                        */ 
/**************************************************************************/ 

#ifndef NXD_FTP_CLIENT_H
#define NXD_FTP_CLIENT_H

/* Determine if a C++ compiler is being used.  If so, ensure that standard
   C is used to process the API information.  */

#ifdef   __cplusplus

/* Yes, C++ compiler is present.  Use standard C.  */
extern   "C" {

#endif

#ifndef      NX_FTP_NO_FILEX    
#include    "fx_api.h" 
#else
#include    "filex_stub.h"
#endif


/* Define the FTP Client ID.  */


#define NXD_FTP_CLIENT_ID                           0x46545200UL


/* Define the maximum number of clients the FTP Server can accommodate.  */

#ifndef NX_FTP_MAX_CLIENTS
#define NX_FTP_MAX_CLIENTS                  4
#endif


/* Define FTP TCP socket create options.  Normally any TCP port will do but this
   gives the application a means to specify a source port.  */

#ifndef NX_FTP_CLIENT_SOURCE_PORT
#define NX_FTP_CLIENT_SOURCE_PORT           NX_ANY_PORT
#endif

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

#ifndef NX_FTP_USERNAME_SIZE
#define NX_FTP_USERNAME_SIZE                20
#endif

#ifndef NX_FTP_PASSWORD_SIZE
#define NX_FTP_PASSWORD_SIZE                20
#endif

#ifndef NX_FTP_TIMEOUT_PERIOD
#define NX_FTP_TIMEOUT_PERIOD               60          /* Number of seconds to check                          */
#endif



/* Define open types.  */

#define NX_FTP_OPEN_FOR_READ                0x01        /* FTP file open for reading                           */
#define NX_FTP_OPEN_FOR_WRITE               0x02        /* FTP file open for writing                           */ 



/* Define return code constants.  */

#define NX_FTP_ERROR                        0xD0        /* Generic FTP internal error - deprecated             */ 
#define NX_FTP_TIMEOUT                      0xD1        /* FTP timeout occurred                                */ 
#define NX_FTP_FAILED                       0xD2        /* FTP error                                           */ 
#define NX_FTP_NOT_CONNECTED                0xD3        /* FTP not connected error                             */ 
#define NX_FTP_NOT_DISCONNECTED             0xD4        /* FTP not disconnected error                          */ 
#define NX_FTP_NOT_OPEN                     0xD5        /* FTP not opened error                                */ 
#define NX_FTP_NOT_CLOSED                   0xD6        /* FTP not closed error                                */ 
#define NX_FTP_END_OF_FILE                  0xD7        /* FTP end of file status                              */ 
#define NX_FTP_END_OF_LISTING               0xD8        /* FTP end of directory listing status                 */ 
#define NX_FTP_EXPECTED_1XX_CODE            0xD9        /* Expected a 1xx response from server                 */
#define NX_FTP_EXPECTED_2XX_CODE            0xDA        /* Expected a 2xx response from server                 */
#define NX_FTP_EXPECTED_22X_CODE            0xDB        /* Expected a 22x response from server                 */
#define NX_FTP_EXPECTED_23X_CODE            0xDC        /* Expected a 23x response from server                 */
#define NX_FTP_EXPECTED_3XX_CODE            0xDD        /* Expected a 3xx response from server                 */
#define NX_FTP_EXPECTED_33X_CODE            0xDE        /* Expected a 33x response from server                 */
#define NX_FTP_INVALID_NUMBER               0xDF        /* Extraced an invalid number from server response     */
#define NX_FTP_INVALID_ADDRESS              0x1D0       /* Invalid IP address parsed from FTP command          */
#define NX_FTP_INVALID_COMMAND              0x1D1       /* Invalid FTP command (bad syntax, unknown command)   */

/* Define FTP connection states.  */

#define NX_FTP_STATE_NOT_CONNECTED          1           /* FTP not connected                                   */ 
#define NX_FTP_STATE_CONNECTED              2           /* FTP connected                                       */ 
#define NX_FTP_STATE_OPEN                   3           /* FTP file open for reading                           */ 
#define NX_FTP_STATE_WRITE_OPEN             4           /* FTP file open for writing                           */ 


/* Define the FTP Server TCP port numbers.  */

#define NX_FTP_SERVER_CONTROL_PORT          21          /* Control Port for FTP server                         */
#define NX_FTP_SERVER_DATA_PORT             20          /* Data Port for FTP server                            */ 

/* Define the size for buffer to store an IPv6 address represented in ASCII. */
#define NX_FTP_IPV6_ADDRESS_BUFSIZE         60

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
#define NX_FTP_EPRT                         19



/* Define the basic FTP Client data structure.  */

typedef struct NX_FTP_CLIENT_STRUCT 
{
    ULONG           nx_ftp_client_id;                               /* FTP Client ID                       */
    CHAR           *nx_ftp_client_name;                             /* Name of this FTP client             */
    NX_IP          *nx_ftp_client_ip_ptr;                           /* Pointer to associated IP structure  */ 
    NX_PACKET_POOL *nx_ftp_client_packet_pool_ptr;                  /* Pointer to FTP client packet pool   */ 
    ULONG           nx_ftp_client_server_ip;                        /* Server's IP address                 */ 
    UINT            nx_ftp_client_state;                            /* State of FTP client                 */ 
    NX_TCP_SOCKET   nx_ftp_client_control_socket;                   /* Client FTP control socket           */
    NX_TCP_SOCKET   nx_ftp_client_data_socket;                      /* Client FTP data transfer socket     */ 
    UINT            nx_ftp_client_data_port;
} NX_FTP_CLIENT;


#ifndef NX_FTP_SOURCE_CODE

/* Application caller is present, perform API mapping.  */

/* Determine if error checking is desired.  If so, map API functions 
   to the appropriate error checking front-ends.  Otherwise, map API
   functions to the core functions that actually perform the work. 
   Note: error checking is enabled by default.  */

#ifdef NX_DISABLE_ERROR_CHECKING

/* Services without error checking.  */

#define nx_ftp_client_connect                       _nx_ftp_client_connect
#define nxd_ftp_client_connect                      _nxd_ftp_client_connect
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

#else

/* Services with error checking.  */

#define nx_ftp_client_connect                       _nxe_ftp_client_connect
#define nxd_ftp_client_connect                      _nxde_ftp_client_connect
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

#endif

/* Define the prototypes accessible to the application software.  */

UINT        nx_ftp_client_connect(NX_FTP_CLIENT *ftp_client_ptr, ULONG server_ip, CHAR *username, CHAR *password, ULONG wait_option);
UINT        nxd_ftp_client_connect(NX_FTP_CLIENT *ftp_client_ptr, NXD_ADDRESS *server_ipduo, CHAR *username, CHAR *password, ULONG wait_option);
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


#else

/* FTP source code is being compiled, do not perform any API mapping.  */

UINT        _nxe_ftp_client_connect(NX_FTP_CLIENT *ftp_client_ptr, ULONG server_ip, CHAR *username, CHAR *password, ULONG wait_option);
UINT        _nx_ftp_client_connect(NX_FTP_CLIENT *ftp_client_ptr, ULONG server_ip, CHAR *username, CHAR *password, ULONG wait_option);
UINT        _nxde_ftp_client_connect(NX_FTP_CLIENT *ftp_client_ptr, NXD_ADDRESS *server_ipduo, CHAR *username, CHAR *password, ULONG wait_option);
UINT        _nxd_ftp_client_connect(NX_FTP_CLIENT *ftp_client_ptr, NXD_ADDRESS *server_ipduo, CHAR *username, CHAR *password, ULONG wait_option);
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

#endif   /* NX_FTP_SOURCE_CODE */

/* Internal functions. */
UINT        _nx_ftp_utility_convert_IPv6_to_ascii(NX_TCP_SOCKET *tcp_socket_ptr, CHAR *buffer, UINT buffer_length);
UINT        _nx_ftp_utility_convert_number_ascii(ULONG number, CHAR *numstring);
UINT        _nx_ftp_utility_convert_portnumber_ascii(UINT number, CHAR *numstring);
UINT        _nx_ftp_utility_parse_port_number(CHAR *buffer_ptr, UINT buffer_length, UINT *portnumber);
UINT        _nx_ftp_client_packet_allocate(NX_FTP_CLIENT *ftp_client_ptr, NX_PACKET **packet_ptr, ULONG wait_option);


/* Determine if a C++ compiler is being used.  If so, complete the standard
   C conditional started above.  */
#ifdef   __cplusplus
        }
#endif

#endif  /* NXD_FTP_CLIENT_H */
