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
/**   Hypertext Transfer Protocol (HTTP)                                  */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/
/*                                                                        */
/*  APPLICATION INTERFACE DEFINITION                       RELEASE        */
/*                                                                        */
/*    nxd_http_client.h                                   PORTABLE C      */
/*                                                           5.2          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    William E. Lamie, Express Logic, Inc.                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This file defines the NetX Hypertext Transfer Protocol (HTTP)       */
/*    component, including all data types and external references.        */
/*    It is assumed that nx_api.h and nx_port.h have already been         */
/*    included, along with fx_api.h and fx_port.h.                        */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  11-21-2011     William E. Lamie         Initial Version 5.1           */
/*  01-31-2013     Janet Christiansen       Modified comment(s),          */
/*                                            resulting in version 5.2    */
/*                                                                        */
/**************************************************************************/

#ifndef NXD_HTTP_CLIENT_H
#define NXD_HTTP_CLIENT_H

/* Determine if a C++ compiler is being used.  If so, ensure that standard
   C is used to process the API information.  */

#ifdef   __cplusplus

/* Yes, C++ compiler is present.  Use standard C.  */
extern   "C" {

#endif


/* Define the HTTP ID.  */

#define NXD_HTTP_CLIENT_ID                  0x48545450UL


/* Define HTTP TCP socket create options.  */

#ifndef NX_HTTP_TYPE_OF_SERVICE
#define NX_HTTP_TYPE_OF_SERVICE             NX_IP_NORMAL
#endif

#ifndef NX_HTTP_FRAGMENT_OPTION
#define NX_HTTP_FRAGMENT_OPTION             NX_DONT_FRAGMENT
#endif

#ifndef NX_HTTP_TIME_TO_LIVE
#define NX_HTTP_TIME_TO_LIVE                0x80
#endif

#ifndef NX_HTTP_MAX_RESOURCE
#define NX_HTTP_MAX_RESOURCE                40
#endif

#ifndef NX_HTTP_MAX_NAME
#define NX_HTTP_MAX_NAME                    20
#endif

#ifndef NX_HTTP_MAX_PASSWORD
#define NX_HTTP_MAX_PASSWORD                20
#endif

#ifndef NX_HTTP_CLIENT_TIMEOUT
#define NX_HTTP_CLIENT_TIMEOUT              1000
#endif

#ifndef NX_PHYSICAL_TRAILER
#define NX_PHYSICAL_TRAILER                 4
#endif

#ifndef NX_HTTP_CLIENT_MIN_PACKET_SIZE
#define NX_HTTP_CLIENT_MIN_PACKET_SIZE      600
#endif

#define NX_HTTP_MAX_STRING                  NX_HTTP_MAX_NAME + NX_HTTP_MAX_PASSWORD + 20
#define NX_HTTP_MAX_BINARY_MD5              16
#define NX_HTTP_MAX_ASCII_MD5               33


/* Define HTTP Client states.  */

#define NX_HTTP_CLIENT_STATE_READY          1
#define NX_HTTP_CLIENT_STATE_GET            2
#define NX_HTTP_CLIENT_STATE_PUT            3


/* Define return code constants.  */

#define NX_HTTP_ERROR                       0xE0        /* HTTP internal error                                  */
#define NX_HTTP_TIMEOUT                     0xE1        /* HTTP timeout occurred                                */
#define NX_HTTP_FAILED                      0xE2        /* HTTP error                                           */
#define NX_HTTP_DONT_AUTHENTICATE           0xE3        /* HTTP authentication not needed                       */
#define NX_HTTP_BASIC_AUTHENTICATE          0xE4        /* HTTP basic authentication requested                  */
#ifdef  NX_HTTP_DIGEST_ENABLE
#define NX_HTTP_DIGEST_AUTHENTICATE         0xE5        /* HTTP digest authentication requested                 */
#endif
#define NX_HTTP_NOT_FOUND                   0xE6        /* HTTP request not found                               */
#define NX_HTTP_DATA_END                    0xE7        /* HTTP end of content area                             */
#define NX_HTTP_CALLBACK_COMPLETED          0xE8        /* HTTP user callback completed the processing          */
#define NX_HTTP_POOL_ERROR                  0xE9        /* HTTP supplied pool payload is too small              */
#define NX_HTTP_NOT_READY                   0xEA        /* HTTP client not ready for operation                  */
#define NX_HTTP_AUTHENTICATION_ERROR        0xEB        /* HTTP client authentication failed                    */
#define NX_HTTP_GET_DONE                    0xEC        /* HTTP client get is complete                          */
#define NX_HTTP_BAD_PACKET_LENGTH           0xED        /* Invalid packet received - length incorrect           */
#define NX_HTTP_REQUEST_UNSUCCESSFUL_CODE   0xEE        /* Received an error code instead of 2xx from server    */
#define NX_HTTP_INCOMPLETE_PUT_ERROR        0xEF        /* Server responds before PUT is complete               */
#define NX_HTTP_PASSWORD_TOO_LONG           0xF0        /* Password exceeded expected length                    */
#define NX_HTTP_USERNAME_TOO_LONG           0xF1        /* Username exceeded expected length                    */
#define NX_HTTP_NO_QUERY_PARSED             0xF2        /* Server unable to find query in client request        */

/* Define the HTTP Server TCP port number */

#define NX_HTTP_SERVER_PORT                 80          /* Port for HTTP server                                 */


#ifdef  NX_HTTP_DIGEST_ENABLE

/* Include the MD5 digest header file.  */

#include "nx_md5.h"

#endif


/* Define the HTTP Client data structure.  */

typedef struct NX_HTTP_CLIENT_STRUCT
{
    ULONG           nx_http_client_id;                              /* HTTP Server ID                       */
    CHAR           *nx_http_client_name;                            /* Name of this HTTP Client             */
    UINT            nx_http_client_state;                           /* Current state of HTTP Client         */
    NX_IP          *nx_http_client_ip_ptr;                          /* Pointer to associated IP structure   */
    NX_PACKET_POOL *nx_http_client_packet_pool_ptr;                 /* Pointer to HTTP Client packet pool   */
    ULONG           nx_http_client_total_transfer_bytes;            /* Total number of bytes to transfer    */
    ULONG           nx_http_client_actual_bytes_transferred;        /* Number of bytes actually transferred */
    NX_PACKET      *nx_http_client_first_packet;                    /* Pointer to first packet with data    */
    NX_TCP_SOCKET   nx_http_client_socket;                          /* HTTP Client TCP socket               */
#ifdef  NX_HTTP_DIGEST_ENABLE
    NX_MD5          nx_http_client_md5data;                         /* HTTP Client MD5 work area            */
#endif
} NX_HTTP_CLIENT;



#ifndef NX_HTTP_SOURCE_CODE

/* Application caller is present, perform API mapping.  */

/* Determine if error checking is desired.  If so, map API functions
   to the appropriate error checking front-ends.  Otherwise, map API
   functions to the core functions that actually perform the work.
   Note: error checking is enabled by default.  */

#ifdef NX_DISABLE_ERROR_CHECKING

/* Services without error checking.  */

#define nx_http_client_create                       _nx_http_client_create
#define nx_http_client_delete                       _nx_http_client_delete
#define nx_http_client_get_start                    _nx_http_client_get_start
#define nx_http_client_get_packet                   _nx_http_client_get_packet
#define nx_http_client_put_start                    _nx_http_client_put_start
#define nx_http_client_put_packet                   _nx_http_client_put_packet
#define nxd_http_client_get_start                   _nxd_http_client_get_start
#define nxd_http_client_put_start                   _nxd_http_client_put_start

#else

/* Services with error checking.  */

#define nx_http_client_create                       _nxe_http_client_create
#define nx_http_client_delete                       _nxe_http_client_delete
#define nx_http_client_get_start                    _nxe_http_client_get_start
#define nx_http_client_get_packet                   _nxe_http_client_get_packet
#define nx_http_client_put_start                    _nxe_http_client_put_start
#define nx_http_client_put_packet                   _nxe_http_client_put_packet
#define nxd_http_client_put_start                   _nxde_http_client_put_start
#define nxd_http_client_get_start                   _nxde_http_client_get_start

#endif  /* NX_DISABLE_ERROR_CHECKING */

/* Define the prototypes accessible to the application software.  */

UINT        nx_http_client_create(NX_HTTP_CLIENT *client_ptr, CHAR *client_name, NX_IP *ip_ptr, NX_PACKET_POOL *pool_ptr, ULONG window_size);
UINT        nx_http_client_delete(NX_HTTP_CLIENT *client_ptr);
UINT        nx_http_client_get_start(NX_HTTP_CLIENT *client_ptr, ULONG ip_address, CHAR *resource, CHAR *input_ptr, UINT input_size, CHAR *username, CHAR *password, ULONG wait_option);
UINT        nx_http_client_get_packet(NX_HTTP_CLIENT *client_ptr, NX_PACKET **packet_ptr, ULONG wait_option);
UINT        nx_http_client_put_start(NX_HTTP_CLIENT *client_ptr, ULONG ip_address, CHAR *resource, CHAR *username, CHAR *password, ULONG total_bytes, ULONG wait_option);
UINT        nx_http_client_put_packet(NX_HTTP_CLIENT *client_ptr, NX_PACKET *packet_ptr, ULONG wait_option);
UINT        nxd_http_client_get_start(NX_HTTP_CLIENT *client_ptr, NXD_ADDRESS *ip_address, CHAR *resource, CHAR *input_ptr, UINT input_size, CHAR *username, CHAR *password, ULONG wait_option);
UINT        nxd_http_client_put_start(NX_HTTP_CLIENT *client_ptr, NXD_ADDRESS *ip_address, CHAR *resource, CHAR *username, CHAR *password, ULONG total_bytes, ULONG wait_option);

#else

/* HTTP source code is being compiled, do not perform any API mapping.  */

UINT        _nxe_http_client_create(NX_HTTP_CLIENT *client_ptr, CHAR *client_name, NX_IP *ip_ptr, NX_PACKET_POOL *pool_ptr, ULONG window_size);
UINT        _nx_http_client_create(NX_HTTP_CLIENT *client_ptr, CHAR *client_name, NX_IP *ip_ptr, NX_PACKET_POOL *pool_ptr, ULONG window_size);
UINT        _nxe_http_client_delete(NX_HTTP_CLIENT *client_ptr);
UINT        _nx_http_client_delete(NX_HTTP_CLIENT *client_ptr);
UINT        _nxe_http_client_get_start(NX_HTTP_CLIENT *client_ptr, ULONG ip_address, CHAR *resource, CHAR *input_ptr, UINT input_size, CHAR *username, CHAR *password, ULONG wait_option);
UINT        _nx_http_client_get_start(NX_HTTP_CLIENT *client_ptr, ULONG ip_address, CHAR *resource, CHAR *input_ptr, UINT input_size, CHAR *username, CHAR *password, ULONG wait_option);
UINT        _nxe_http_client_get_packet(NX_HTTP_CLIENT *client_ptr, NX_PACKET **packet_ptr, ULONG wait_option);
UINT        _nx_http_client_get_packet(NX_HTTP_CLIENT *client_ptr, NX_PACKET **packet_ptr, ULONG wait_option);
UINT        _nxe_http_client_put_start(NX_HTTP_CLIENT *client_ptr, ULONG ip_address, CHAR *resource, CHAR *username, CHAR *password, ULONG total_bytes, ULONG wait_option);
UINT        _nx_http_client_put_start(NX_HTTP_CLIENT *client_ptr, ULONG ip_address, CHAR *resource, CHAR *username, CHAR *password, ULONG total_bytes, ULONG wait_option);
UINT        _nxe_http_client_put_packet(NX_HTTP_CLIENT *client_ptr, NX_PACKET *packet_ptr, ULONG wait_option);
UINT        _nx_http_client_put_packet(NX_HTTP_CLIENT *client_ptr, NX_PACKET *packet_ptr, ULONG wait_option);
UINT        _nxde_http_client_get_start(NX_HTTP_CLIENT *client_ptr, NXD_ADDRESS *ip_address, CHAR *resource, CHAR *input_ptr, UINT input_size, CHAR *username, CHAR *password, ULONG wait_option);
UINT        _nxd_http_client_get_start(NX_HTTP_CLIENT *client_ptr, NXD_ADDRESS *ip_address, CHAR *resource, CHAR *input_ptr, UINT input_size, CHAR *username, CHAR *password, ULONG wait_option);
UINT        _nxde_http_client_put_start(NX_HTTP_CLIENT *client_ptr, NXD_ADDRESS *ip_address, CHAR *resource, CHAR *username, CHAR *password, ULONG total_bytes, ULONG wait_option);
UINT        _nxd_http_client_put_start(NX_HTTP_CLIENT *client_ptr, NXD_ADDRESS *ip_address, CHAR *resource, CHAR *username, CHAR *password, ULONG total_bytes, ULONG wait_option);

/* Define internal HTTP functions.  */

UINT        _nx_http_client_type_get(CHAR *name, CHAR *http_type_string);
UINT        _nx_http_client_content_length_get(NX_PACKET *packet_ptr);
UINT        _nx_http_client_calculate_content_offset(NX_PACKET *packet_ptr);
UINT        _nx_http_client_number_convert(UINT number, CHAR *string);
VOID        _nx_http_client_base64_encode(CHAR *name, CHAR *base64name);


#endif  /* NX_HTTP_SOURCE_CODE */

/* Determine if a C++ compiler is being used.  If so, complete the standard
   C conditional started above.  */
#ifdef   __cplusplus
        }
#endif

#endif  /* NXD_HTTP_CLIENT_H */
