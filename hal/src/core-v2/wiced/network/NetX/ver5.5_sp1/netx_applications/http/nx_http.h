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
/**   Hypertext Transfer Protocol (HTTP)                                  */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/
/*                                                                        */
/*  APPLICATION INTERFACE DEFINITION                       RELEASE        */
/*                                                                        */
/*    nx_http.h                                           PORTABLE C      */
/*                                                           5.3          */
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
/*  03-01-2006     William E. Lamie         Initial Version 5.0           */
/*  04-01-2010     Janet Christiansen       Modified comment(s),          */
/*                                            resulting in version 5.1    */
/*  07-15-2011     Janet Christiansen       Modified comment(s),          */
/*                                            resulting in version 5.2    */
/*  04-30-2013     Janet Christiansen       Modified comment(s), and      */
/*                                            option from source code,    */
/*                                            added error codes,          */
/*                                            resulting in version 5.3    */
/*                                                                        */
/**************************************************************************/

#ifndef NX_HTTP_H
#define NX_HTTP_H

/* Determine if a C++ compiler is being used.  If so, ensure that standard
   C is used to process the API information.  */

#ifdef   __cplusplus

/* Yes, C++ compiler is present.  Use standard C.  */
extern   "C" {

#endif

/* Determine to use the Express Logic or other file system. */
#ifndef      NX_HTTP_NO_FILEX
#include    "fx_api.h"
#else
#include    "filex_stub.h"
#endif

/* Define the HTTP ID.  */

#define NX_HTTP_ID                          0x48545450UL
/* Enable Digest authentication. 
#define NX_HTTP_DIGEST_ENABLE
*/


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

#ifndef NX_HTTP_SERVER_PRIORITY
#define NX_HTTP_SERVER_PRIORITY             16
#endif

#ifndef NX_HTTP_SERVER_WINDOW_SIZE
#define NX_HTTP_SERVER_WINDOW_SIZE          2048
#endif

#ifndef NX_HTTP_SERVER_TIMEOUT
#define NX_HTTP_SERVER_TIMEOUT              1000
#endif

#ifndef NX_HTTP_SERVER_MAX_PENDING
#define NX_HTTP_SERVER_MAX_PENDING          5
#endif

#ifndef NX_HTTP_SERVER_THREAD_TIME_SLICE
#define NX_HTTP_SERVER_THREAD_TIME_SLICE    2
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

#ifndef NX_PHYSICAL_TRAILER
#define NX_PHYSICAL_TRAILER                 4
#endif

#ifndef NX_HTTP_SERVER_MIN_PACKET_SIZE
#define NX_HTTP_SERVER_MIN_PACKET_SIZE      600
#endif

#ifndef NX_HTTP_CLIENT_MIN_PACKET_SIZE
#define NX_HTTP_CLIENT_MIN_PACKET_SIZE      300
#endif

/* Define the HTTP server retry parameters.  */

#ifndef NX_HTTP_SERVER_RETRY_SECONDS
#define NX_HTTP_SERVER_RETRY_SECONDS        2           /* 2 second initial timeout                            */ 
#endif

#ifndef NX_HTTP_SERVER_TRANSMIT_QUEUE_DEPTH
#define NX_HTTP_SERVER_TRANSMIT_QUEUE_DEPTH 20          /* Maximum of 20 queued transmit packets               */ 
#endif

#ifndef NX_HTTP_SERVER_RETRY_MAX
#define NX_HTTP_SERVER_RETRY_MAX            10          /* Maximum of 10 retries per packet                    */ 
#endif

#ifndef NX_HTTP_SERVER_RETRY_SHIFT
#define NX_HTTP_SERVER_RETRY_SHIFT          1           /* Every retry is twice as long                        */
#endif

#define NX_HTTP_MAX_STRING                  NX_HTTP_MAX_NAME+NX_HTTP_MAX_PASSWORD+20
#define NX_HTTP_MAX_BINARY_MD5              16
#define NX_HTTP_MAX_ASCII_MD5               33


/* Define HTTP Server request types.  */

#define NX_HTTP_SERVER_GET_REQUEST          1
#define NX_HTTP_SERVER_POST_REQUEST         2
#define NX_HTTP_SERVER_HEAD_REQUEST         3
#define NX_HTTP_SERVER_PUT_REQUEST          4
#define NX_HTTP_SERVER_DELETE_REQUEST       5


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
#define NX_HTTP_IMPROPERLY_TERMINATED_PARAM 0xF3        /* Client request parameter not properly terminated     */


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


/* Define the HTTP Server data structure.  */

typedef struct NX_HTTP_SERVER_STRUCT
{
    ULONG           nx_http_server_id;                              /* HTTP Server ID                       */
    CHAR           *nx_http_server_name;                            /* Name of this HTTP Server             */
    NX_IP          *nx_http_server_ip_ptr;                          /* Pointer to associated IP structure   */
    CHAR            nx_http_server_request_resource[NX_HTTP_MAX_RESOURCE];
                                                                    /* Uniform Resource Locator (URL)       */
    UINT            nx_http_connection_pending;                     /* Connection pending flag              */
    NX_PACKET_POOL *nx_http_server_packet_pool_ptr;                 /* Pointer to HTTP Server packet pool   */
    FX_MEDIA       *nx_http_server_media_ptr;                       /* Pointer to media control block       */
    ULONG           nx_http_server_get_requests;                    /* Number of get requests               */
    ULONG           nx_http_server_head_requests;                   /* Number of head requests              */
    ULONG           nx_http_server_put_requests;                    /* Number of put requests               */
    ULONG           nx_http_server_delete_requests;                 /* Number of delete requests            */
    ULONG           nx_http_server_post_requests;                   /* Number of post requests              */
    ULONG           nx_http_server_unknown_requests;                /* Number of unknown requests           */
    ULONG           nx_http_server_total_bytes_sent;                /* Number of total bytes sent           */
    ULONG           nx_http_server_total_bytes_received;            /* Number of total bytes received       */
    ULONG           nx_http_server_allocation_errors;               /* Number of allocation errors          */
    ULONG           nx_http_server_connection_failures;             /* Number of failed connections         */
    ULONG           nx_http_server_connection_successes;            /* Number of successful connections     */
    ULONG           nx_http_server_invalid_http_headers;            /* Number of invalid http headers       */
    FX_FILE         nx_http_server_file;                            /* HTTP file control block              */
    NX_TCP_SOCKET   nx_http_server_socket;                          /* HTTP Server TCP socket               */
    TX_THREAD       nx_http_server_thread;                          /* HTTP server thread                   */
#ifdef  NX_HTTP_DIGEST_ENABLE
    NX_MD5          nx_http_server_md5data;                         /* HTTP server MD5 work area            */
#endif
    /* Define the user supplied routines that are used to inform the application of particular server requests.  */

    UINT (*nx_http_server_authentication_check)(struct NX_HTTP_SERVER_STRUCT *server_ptr, UINT request_type, CHAR *resource, CHAR **name, CHAR **password, CHAR **realm);
    UINT (*nx_http_server_request_notify)(struct NX_HTTP_SERVER_STRUCT *server_ptr, UINT request_type, CHAR *resource, NX_PACKET *packet_ptr);
} NX_HTTP_SERVER;



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
#define nx_http_server_callback_data_send           _nx_http_server_callback_data_send
#define nx_http_server_callback_response_send       _nx_http_server_callback_response_send
#define nx_http_server_content_get                  _nx_http_server_content_get
#define nx_http_server_content_length_get           _nx_http_server_content_length_get
#define nx_http_server_create                       _nx_http_server_create
#define nx_http_server_delete                       _nx_http_server_delete
#define nx_http_server_param_get                    _nx_http_server_param_get
#define nx_http_server_query_get                    _nx_http_server_query_get
#define nx_http_server_start                        _nx_http_server_start
#define nx_http_server_stop                         _nx_http_server_stop

#else

/* Services with error checking.  */

#define nx_http_client_create                       _nxe_http_client_create
#define nx_http_client_delete                       _nxe_http_client_delete
#define nx_http_client_get_start                    _nxe_http_client_get_start
#define nx_http_client_get_packet                   _nxe_http_client_get_packet
#define nx_http_client_put_start                    _nxe_http_client_put_start
#define nx_http_client_put_packet                   _nxe_http_client_put_packet
#define nx_http_server_callback_data_send           _nx_http_server_callback_data_send
#define nx_http_server_callback_response_send       _nx_http_server_callback_response_send
#define nx_http_server_content_get                  _nxe_http_server_content_get
#define nx_http_server_content_length_get           _nxe_http_server_content_length_get
#define nx_http_server_create                       _nxe_http_server_create
#define nx_http_server_delete                       _nxe_http_server_delete
#define nx_http_server_param_get                    _nxe_http_server_param_get
#define nx_http_server_query_get                    _nxe_http_server_query_get
#define nx_http_server_start                        _nxe_http_server_start
#define nx_http_server_stop                         _nxe_http_server_stop

#endif  /* NX_DISABLE_ERROR_CHECKING */

/* Define the prototypes accessible to the application software.  */

UINT        nx_http_client_create(NX_HTTP_CLIENT *client_ptr, CHAR *client_name, NX_IP *ip_ptr, NX_PACKET_POOL *pool_ptr, ULONG window_size);
UINT        nx_http_client_delete(NX_HTTP_CLIENT *client_ptr);
UINT        nx_http_client_get_start(NX_HTTP_CLIENT *client_ptr, ULONG ip_address, CHAR *resource, CHAR *input_ptr, UINT input_size, CHAR *username, CHAR *password, ULONG wait_option);
UINT        nx_http_client_get_packet(NX_HTTP_CLIENT *client_ptr, NX_PACKET **packet_ptr, ULONG wait_option);
UINT        nx_http_client_put_start(NX_HTTP_CLIENT *client_ptr, ULONG ip_address, CHAR *resource, CHAR *username, CHAR *password, ULONG total_bytes, ULONG wait_option);
UINT        nx_http_client_put_packet(NX_HTTP_CLIENT *client_ptr, NX_PACKET *packet_ptr, ULONG wait_option);

UINT        nx_http_server_callback_data_send(NX_HTTP_SERVER *server_ptr, VOID *data_ptr, ULONG data_length);
UINT        nx_http_server_callback_response_send(NX_HTTP_SERVER *server_ptr, const CHAR *header, const CHAR *information, const CHAR *additional_info);
UINT        nx_http_server_content_get(NX_HTTP_SERVER *server_ptr, NX_PACKET *packet_ptr, ULONG byte_offset, CHAR *destination_ptr, UINT destination_size, UINT *actual_size);
UINT        nx_http_server_content_length_get(NX_PACKET *packet_ptr);
UINT        nx_http_server_create(NX_HTTP_SERVER *http_server_ptr, CHAR *http_server_name, NX_IP *ip_ptr, FX_MEDIA *media_ptr, VOID *stack_ptr, ULONG stack_size, NX_PACKET_POOL *pool_ptr,
                                UINT (*authentication_check)(NX_HTTP_SERVER *server_ptr, UINT request_type, CHAR *resource, CHAR **name, CHAR **password, CHAR **realm),
                                UINT (*request_notify)(NX_HTTP_SERVER *server_ptr, UINT request_type, CHAR *resource, NX_PACKET *packet_ptr));
UINT        nx_http_server_delete(NX_HTTP_SERVER *http_server_ptr);
UINT        nx_http_server_param_get(NX_PACKET *packet_ptr, UINT param_number, CHAR *param_ptr, UINT max_param_size);
UINT        nx_http_server_query_get(NX_PACKET *packet_ptr, UINT query_number, CHAR *query_ptr, UINT max_query_size);
UINT        nx_http_server_start(NX_HTTP_SERVER *http_server_ptr);
UINT        nx_http_server_stop(NX_HTTP_SERVER *http_server_ptr);

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

UINT        _nx_http_server_callback_data_send(NX_HTTP_SERVER *server_ptr, VOID *data_ptr, ULONG data_length);
UINT        _nx_http_server_callback_response_send(NX_HTTP_SERVER *server_ptr, const CHAR *header, const CHAR *information, const CHAR *additional_info);
UINT        _nxe_http_server_content_get(NX_HTTP_SERVER *server_ptr, NX_PACKET *packet_ptr, ULONG byte_offset, CHAR *destination_ptr, UINT destination_size, UINT *actual_size);
UINT        _nx_http_server_content_get(NX_HTTP_SERVER *server_ptr, NX_PACKET *packet_ptr, ULONG byte_offset, CHAR *destination_ptr, UINT destination_size, UINT *actual_size);
UINT        _nxe_http_server_content_length_get(NX_PACKET *packet_ptr);
UINT        _nx_http_server_content_length_get(NX_PACKET *packet_ptr);
UINT        _nxe_http_server_create(NX_HTTP_SERVER *http_server_ptr, CHAR *http_server_name, NX_IP *ip_ptr, FX_MEDIA *media_ptr, VOID *stack_ptr, ULONG stack_size, NX_PACKET_POOL *pool_ptr,
                                UINT (*authentication_check)(NX_HTTP_SERVER *server_ptr, UINT request_type, CHAR *resource, CHAR **name, CHAR **password, CHAR **realm),
                                UINT (*request_notify)(NX_HTTP_SERVER *server_ptr, UINT request_type, CHAR *resource, NX_PACKET *packet_ptr));
UINT        _nx_http_server_create(NX_HTTP_SERVER *http_server_ptr, CHAR *http_server_name, NX_IP *ip_ptr, FX_MEDIA *media_ptr, VOID *stack_ptr, ULONG stack_size, NX_PACKET_POOL *pool_ptr,
                                UINT (*authentication_check)(NX_HTTP_SERVER *server_ptr, UINT request_type, CHAR *resource, CHAR **name, CHAR **password, CHAR **realm),
                                UINT (*request_notify)(NX_HTTP_SERVER *server_ptr, UINT request_type, CHAR *resource, NX_PACKET *packet_ptr));
UINT        _nxe_http_server_delete(NX_HTTP_SERVER *http_server_ptr);
UINT        _nx_http_server_delete(NX_HTTP_SERVER *http_server_ptr);
UINT        _nxe_http_server_param_get(NX_PACKET *packet_ptr, UINT param_number, CHAR *param_ptr, UINT max_param_size);
UINT        _nx_http_server_param_get(NX_PACKET *packet_ptr, UINT param_number, CHAR *param_ptr, UINT max_param_size);
UINT        _nxe_http_server_query_get(NX_PACKET *packet_ptr, UINT query_number, CHAR *query_ptr, UINT max_query_size);
UINT        _nx_http_server_query_get(NX_PACKET *packet_ptr, UINT query_number, CHAR *query_ptr, UINT max_query_size);
UINT        _nxe_http_server_start(NX_HTTP_SERVER *http_server_ptr);
UINT        _nx_http_server_start(NX_HTTP_SERVER *http_server_ptr);
UINT        _nxe_http_server_stop(NX_HTTP_SERVER *http_server_ptr);
UINT        _nx_http_server_stop(NX_HTTP_SERVER *http_server_ptr);


/* Define internal HTTP Server functions.  */

UINT        _nx_http_client_type_get(CHAR *name, CHAR *http_type_string);
UINT        _nx_http_client_content_length_get(NX_PACKET *packet_ptr);
UINT        _nx_http_client_calculate_content_offset(NX_PACKET *packet_ptr);
UINT        _nx_http_client_number_convert(UINT number, CHAR *string);
VOID        _nx_http_client_base64_encode(CHAR *name, CHAR *base64name);

VOID        _nx_http_server_connection_present(NX_TCP_SOCKET *socket_ptr, UINT port);
VOID        _nx_http_server_thread_entry(ULONG http_server_address);
UINT        _nx_http_server_get_client_request(NX_HTTP_SERVER *server_ptr, NX_PACKET **packet_ptr);
VOID        _nx_http_server_get_process(NX_HTTP_SERVER *server_ptr, UINT request_type, NX_PACKET *packet_ptr);
VOID        _nx_http_server_put_process(NX_HTTP_SERVER *server_ptr, NX_PACKET *packet_ptr);
VOID        _nx_http_server_delete_process(NX_HTTP_SERVER *server_ptr, NX_PACKET *packet_ptr);
VOID        _nx_http_server_response_send(NX_HTTP_SERVER *server_ptr, const CHAR *header, const CHAR *information, const CHAR *additional_info);
UINT        _nx_http_server_basic_authenticate(NX_HTTP_SERVER *server_ptr, NX_PACKET *packet_ptr, CHAR *name_ptr, CHAR *password_ptr, CHAR *realm_ptr);
UINT        _nx_http_server_retrieve_basic_authorization(NX_PACKET *packet_ptr, CHAR *authorization_request);
UINT        _nx_http_server_retrieve_resource(NX_PACKET *packet_ptr, CHAR *destination, UINT max_size);
UINT        _nx_http_server_calculate_content_offset(NX_PACKET *packet_ptr);
UINT        _nx_http_server_number_convert(UINT number, CHAR *string);
UINT        _nx_http_server_type_get(CHAR *name, CHAR *http_type_string);
VOID        _nx_http_base64_encode(CHAR *name, CHAR *base64name);
VOID        _nx_http_base64_decode(CHAR *base64name, CHAR *name);

#ifdef  NX_HTTP_DIGEST_ENABLE
UINT        _nx_http_server_digest_authenticate(NX_HTTP_SERVER *server_ptr, NX_PACKET *packet_ptr, CHAR *name_ptr, CHAR *password_ptr, CHAR *realm_ptr);
VOID        _nx_http_server_digest_response_calculate(NX_HTTP_SERVER *server_ptr, CHAR *username, CHAR *realm, CHAR *password, CHAR *nonce, CHAR *method, CHAR *uri, CHAR *nc, CHAR *cnonce, CHAR *result);
UINT        _nx_http_server_retrieve_digest_authorization(NX_PACKET *packet_ptr, CHAR *response, CHAR *uri, CHAR *nc, CHAR *cnonce);
VOID        _nx_http_server_hex_ascii_convert(CHAR *source, UINT source_length, CHAR *destination);
#endif /* NX_HTTP_DIGEST_ENABLE */


#endif  /* NX_HTTP_SOURCE_CODE */

/* Determine if a C++ compiler is being used.  If so, complete the standard
   C conditional started above.  */
#ifdef   __cplusplus
}
#endif  /* __cplusplus */

#endif  /* NX_HTTP_H */
