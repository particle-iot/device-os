/*
 * Copyright (c) 2015 Broadcom
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * 3. Neither the name of Broadcom nor the names of other contributors to this 
 * software may be used to endorse or promote products derived from this software 
 * without specific prior written permission.
 *
 * 4. This software may not be used as a standalone product, and may only be used as 
 * incorporated in your product or device that incorporates Broadcom wireless connectivity 
 * products and solely for the purpose of enabling the functionalities of such Broadcom products.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY WARRANTIES OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT, ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *
 * API for the HTTP / HTTPS Web server
 *
 * Web pages and other resources are provided via an array which gets passed as an argument when starting the web server.
 * The array is constructed using the START_OF_HTTP_PAGE_DATABASE() and END_OF_HTTP_PAGE_DATABASE() macros, and optionally the ROOT_HTTP_PAGE_REDIRECT() macro
 * Below is an example of a list of web pages (taken from one of the demo apps)
 *
 * START_OF_HTTP_PAGE_DATABASE(web_pages)
 *     ROOT_HTTP_PAGE_REDIRECT("/apps/temp_control/main.html"),
 *     { "/apps/temp_control/main.html",    "text/html",                WICED_RESOURCE_URL_CONTENT,  .url_content.resource_data  = &resources_apps_DIR_temp_control_DIR_main_html, },
 *     { "/temp_report.html",               "text/html",                WICED_DYNAMIC_URL_CONTENT,   .url_content.dynamic_data   = {process_temperature_update, 0 }, },
 *     { "/temp_up",                        "text/html",                WICED_DYNAMIC_URL_CONTENT,   .url_content.dynamic_data   = {process_temperature_up, 0 }, },
 *     { "/temp_down",                      "text/html",                WICED_DYNAMIC_URL_CONTENT,   .url_content.dynamic_data   = {process_temperature_down, 0 }, },
 *     { "/images/favicon.ico",             "image/vnd.microsoft.icon", WICED_RESOURCE_URL_CONTENT,  .url_content.resource_data  = &resources_images_DIR_favicon_ico, },
 *     { "/scripts/general_ajax_script.js", "application/javascript",   WICED_RESOURCE_URL_CONTENT,  .url_content.resource_data  = &resources_scripts_DIR_general_ajax_script_js, },
 *     { "/images/brcmlogo.png",            "image/png",                WICED_RESOURCE_URL_CONTENT,  .url_content.resource_data  = &resources_images_DIR_brcmlogo_png, },
 *     { "/images/brcmlogo_line.png",       "image/png",                WICED_RESOURCE_URL_CONTENT,  .url_content.resource_data  = &resources_images_DIR_brcmlogo_line_png, },
 *     { "/styles/buttons.css",             "text/css",                 WICED_RESOURCE_URL_CONTENT,  .url_content.resource_data  = &resources_styles_DIR_buttons_css, },
 * END_OF_HTTP_PAGE_DATABASE();
 */



#pragma once

#include "wiced_defaults.h"
#include "wiced_tcpip.h"
#include "wiced_rtos.h"
#include "wiced_resource.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/**
 * Macro which creates the start of a web-page list.
 */
#define START_OF_HTTP_PAGE_DATABASE(name) \
    const wiced_http_page_t name[] = {

/**
 * Macro which creates an entry in the web-page list which will redirect requests for the '/' page to another URL.
 */
#define ROOT_HTTP_PAGE_REDIRECT(url) \
    { "/", "text/html", WICED_RAW_STATIC_URL_CONTENT, .url_content.static_data  = {url, sizeof(url)-1 } }

/**
 * Macro which creates the end part of a web-page list.
 */
#define END_OF_HTTP_PAGE_DATABASE() \
    {0,0,0, .url_content.static_data  = {NULL, 0 } } \
    }

#define EXPAND_AS_ENUMERATION(a,b)   a,
#define EXPAND_AS_MIME_TABLE(a,b)    b,

/******************************************************
 *                    Constants
 ******************************************************/

#define HTTP_404 \
    "HTTP/1.1 404 Not Found\r\n" \
    "Content-Type: text/html\r\n\r\n" \
    "<!doctype html>\n" \
    "<html><head><title>404 - WICED Web Server</title></head><body>\n" \
    "<h1>Address not found on WICED Web Server</h1>\n" \
    "<p><a href=\"/\">Return to home page</a></p>\n" \
    "</body>\n</html>\n"

#define MIME_TABLE( ENTRY ) \
    ENTRY( MIME_TYPE_TLV = 0 ,                "application/x-tlv8"               ) \
    ENTRY( MIME_TYPE_APPLE_BINARY_PLIST,      "application/x-apple-binary-plist" ) \
    ENTRY( MIME_TYPE_APPLE_PROXY_AUTOCONFIG,  "application/x-ns-proxy-autoconfig") \
    ENTRY( MIME_TYPE_BINARY_DATA,             "application/octet-stream"         ) \
    ENTRY( MIME_TYPE_JAVASCRIPT,              "application/javascript"           ) \
    ENTRY( MIME_TYPE_JSON,                    "application/json"                 ) \
    ENTRY( MIME_TYPE_HAP_JSON,                "application/hap+json"             ) \
    ENTRY( MIME_TYPE_HAP_PAIRING,             "application/pairing+tlv8"         ) \
    ENTRY( MIME_TYPE_HAP_VERIFY,              "application/hap+verify"           ) \
    ENTRY( MIME_TYPE_TEXT_HTML,               "text/html"                        ) \
    ENTRY( MIME_TYPE_TEXT_PLAIN,              "text/plain"                       ) \
    ENTRY( MIME_TYPE_TEXT_CSS,                "text/css"                         ) \
    ENTRY( MIME_TYPE_IMAGE_PNG,               "image/png"                        ) \
    ENTRY( MIME_TYPE_IMAGE_GIF,               "image/gif"                        ) \
    ENTRY( MIME_TYPE_IMAGE_MICROSOFT,         "image/vnd.microsoft.icon"         ) \
    ENTRY( MIME_TYPE_ALL,                     "*/*"                              ) /* This must always be the last mimne*/

/**
 * A string with the address which iOS searches during the captive-portal part of a Wi-Fi attachment.
 */
#define IOS_CAPTIVE_PORTAL_ADDRESS        "/library/test/success.html"

#define DEFAULT_URL_PROCESSOR_STACK_SIZE  5000

#define NO_CONTENT_LENGTH                 0
#define CHUNKED_CONTENT_LENGTH            NO_CONTENT_LENGTH

#define HTTP_HEADER_200                   "HTTP/1.1 200 OK"
#define HTTP_HEADER_204                   "HTTP/1.1 204 No Content"
#define HTTP_HEADER_207                   "HTTP/1.1 207 Multi-Status"
#define HTTP_HEADER_301                   "HTTP/1.1 301"
#define HTTP_HEADER_400                   "HTTP/1.1 400 Bad Request"
#define HTTP_HEADER_403                   "HTTP/1.1 403"
#define HTTP_HEADER_404                   "HTTP/1.1 404 Not Found"
#define HTTP_HEADER_405                   "HTTP/1.1 405 Method Not Allowed"
#define HTTP_HEADER_406                   "HTTP/1.1 406 Not Acceptable"
#define HTTP_HEADER_412                   "HTTP/1.1 412 Precondition Failed"
#define HTTP_HEADER_429                   "HTTP/1.1 429 Too Many Requests"
#define HTTP_HEADER_444                   "HTTP/1.1 444"
#define HTTP_HEADER_470                   "HTTP/1.1 470 Connection Authorization Required"
#define HTTP_HEADER_500                   "HTTP/1.1 500 Internal Server Error"
#define HTTP_HEADER_504                   "HTTP/1.1 504 Not Able to Connect"
#define HTTP_HEADER_CONTENT_LENGTH        "Content-Length: "
#define HTTP_HEADER_CONTENT_TYPE          "Content-Type: "
#define HTTP_HEADER_CHUNKED               "Transfer-Encoding: chunked"
#define HTTP_HEADER_LOCATION              "Location: "
#define HTTP_HEADER_ACCEPT                "Accept: "
#define HTTP_HEADER_KEEP_ALIVE            "Connection: Keep-Alive"
#define HTTP_HEADER_XHR					  "Access-Control-Allow-Origin: *"
#define HTTP_HEADER_CLOSE                 "Connection: close"
#define NO_CACHE_HEADER                   "Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0\r\n"\
                                          "Pragma: no-cache"
#define CRLF                              "\r\n"
#define CRLF_CRLF                         "\r\n\r\n"

/******************************************************
 *                   Enumerations
 ******************************************************/

/**
 * HTTP cache
 */
typedef enum
{
    HTTP_CACHE_DISABLED, /**< Do not cache previously fetched resources */
    HTTP_CACHE_ENABLED,  /**< Allow caching of previously fetched resources  */
} http_cache_t;

/**
 * HTTP MIME type
 */
typedef enum
{
    MIME_TABLE( EXPAND_AS_ENUMERATION )
    MIME_UNSUPPORTED
} wiced_packet_mime_type_t;

/**
 * HTTP status code
 */
typedef enum
{
    HTTP_200_TYPE,
    HTTP_204_TYPE,
    HTTP_207_TYPE,
    HTTP_301_TYPE,
    HTTP_400_TYPE,
    HTTP_403_TYPE,
    HTTP_404_TYPE,
    HTTP_405_TYPE,
    HTTP_406_TYPE,
    HTTP_412_TYPE,
    HTTP_415_TYPE,
    HTTP_429_TYPE,
    HTTP_444_TYPE,
    HTTP_470_TYPE,
    HTTP_500_TYPE,
    HTTP_504_TYPE
} http_status_codes_t;

/**
 * HTTP request type
 */
typedef enum
{
    WICED_HTTP_GET_REQUEST,
    WICED_HTTP_POST_REQUEST,
    WICED_HTTP_PUT_REQUEST,
    REQUEST_UNDEFINED
} wiced_http_request_type_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/**
 * HTTP server socket callback
 */
typedef wiced_result_t (*http_server_callback_t)( wiced_tcp_socket_t* socket, uint8_t** data, uint16_t* data_length );

/******************************************************
 *                    Structures
 ******************************************************/

/**
 * HTTP message structure that gets passed to dynamic URL processor functions
 */
typedef struct
{
    const uint8_t*            data;                         /* packet data in message body      */
    uint16_t                  message_data_length;          /* data length in current packet    */
    uint16_t                  total_message_data_remaining; /* data yet to be consumed          */
    wiced_bool_t              chunked_transfer;             /* chunked data format              */
    wiced_packet_mime_type_t  mime_type;                    /* mime type                        */
    wiced_http_request_type_t request_type;                 /* GET, POST or PUT request         */
    wiced_tcp_socket_t*		  socket;						/* The socket to retrieve additional packets from */
    void*					  user;							/* additional storage. */
} wiced_http_message_body_t;

/**
 * Workspace structure for HTTP server stream
 * Users should not access these values - they are provided here only
 * to provide the compiler with datatype size information allowing static declarations
 */
typedef struct
{
    wiced_tcp_stream_t tcp_stream;
    wiced_bool_t       chunked_transfer_enabled;
    wiced_bool_t	cross_host_requests_enabled;
} wiced_http_response_stream_t;

/**
 * Prototype for URL processor functions
 */
typedef int32_t (*url_processor_t)(  const char* url, wiced_http_response_stream_t* stream, void* arg, wiced_http_message_body_t* http_data );

typedef enum
    {
        WICED_STATIC_URL_CONTENT,              /** Page is constant data in memory addressable area */
        WICED_DYNAMIC_URL_CONTENT,             /** Page is dynamically generated by a @ref url_processor_t type function */
        WICED_RESOURCE_URL_CONTENT,            /** Page data is proivded by a Wiced Resource which may reside off-chip */
        WICED_RAW_STATIC_URL_CONTENT,          /** Same as @ref WICED_STATIC_URL_CONTENT but HTTP header must be supplied as part of the content */
        WICED_RAW_DYNAMIC_URL_CONTENT,         /** Same as @ref WICED_DYNAMIC_URL_CONTENT but HTTP header must be supplied as part of the content */
        WICED_RAW_RESOURCE_URL_CONTENT         /** Same as @ref WICED_RESOURCE_URL_CONTENT but HTTP header must be supplied as part of the content */
    } url_content_type_t;                        /** The page type - this selects which part of the @url_content union will be used - also see above */


/**
 * HTTP page list structure
 */
typedef struct
{
    const char* url;                     /** String containing the path part of the URL of this page/file */
    const char* mime_type;               /** String containing the MIME type of this page/file */
    url_content_type_t url_content_type;                        /** The page type - this selects which part of the @url_content union will be used - also see above */
    union
    {
        struct                                 /* Used for WICED_DYNAMIC_URL_CONTENT and WICED_RAW_DYNAMIC_URL_CONTENT */
        {
            url_processor_t generator;   /* The function which will handle requests for this page */
            void*                 arg;         /* An argument to be passed to the generator function    */
        } dynamic_data;
        struct                                 /* Used for WICED_STATIC_URL_CONTENT and WICED_RAW_STATIC_URL_CONTENT */
        {
            const void* ptr;                   /* A pointer to the data for the page / file */
            uint32_t length;                   /* The length in bytes of the page / file */
        } static_data;
        const resource_hnd_t* resource_data;   /* A Wiced Resource containing the page / file - Used for WICED_RESOURCE_URL_CONTENT and WICED_RAW_RESOURCE_URL_CONTENT */
    } url_content;
} wiced_http_page_t;

/**
 * Workspace structure for HTTP server
 * Users should not access these values - they are provided here only
 * to provide the compiler with datatype size information allowing static declarations
 */
typedef struct
{
    wiced_tcp_server_t       tcp_server;
    wiced_thread_t           event_thread;
    wiced_queue_t            event_queue;
    wiced_worker_thread_t    worker_thread;
    volatile wiced_bool_t    quit;
    const wiced_http_page_t* page_database;
    http_server_callback_t   receive_callback;
} wiced_http_server_t;

/**
 * Workspace structure for HTTPS server
 * Users should not access these values - they are provided here only
 * to provide the compiler with datatype size information allowing static declarations
 */
typedef struct
{
    wiced_http_server_t          http_server;
    wiced_tls_advanced_context_t tls_context;
} wiced_https_server_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/**
 * Start a HTTP server daemon (web server)
 *
 * The web server implements HTTP1.1 using a non-blocking architecture which allows
 * multiple sockets to be served simultaneously.
 * Web pages and other files can be served either dynamically from a function or
 * from static data in memory or internal/external flash resources
 *
 * @param[in] server         Structure workspace that will be used for this HTTP server instance - allocated by caller.
 * @param[in] port           TCP port number on which to listen - usually port 80 for normal HTTP
 * @param[in] max_sockets    Maximum number of sockets to be served simultaneously
 * @param[in] page_database  A list of web pages / files that will be served by the HTTP server. See @ref wiced_http_page_t for details and snippet apps for examples
 * @param[in] interface      Which network interface the HTTP server should listen on.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_http_server_start( wiced_http_server_t* server, uint16_t port, uint16_t max_sockets, const wiced_http_page_t* page_database, wiced_interface_t interface, uint32_t url_processor_stack_size );

/**
 *  Stop a HTTP server daemon (web server)
 *
 * @param[in] server   The structure workspace that was used with @ref wiced_http_server_start
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_http_server_stop( wiced_http_server_t* server );

/**
 * Start a HTTPS server daemon (secure web server)
 *
 * This is identical to @ref wiced_http_server_start except that it uses TLS to provide
 * a secure HTTPS link
 *
 * @param[in] server         Structure workspace that will be used for this HTTP server instance - allocated by caller.
 * @param[in] port           TCP port number on which to listen - usually port 80 for normal HTTP
 * @param[in] max_sockets    Maximum number of sockets to be served simultaneously
 * @param[in] page_database  A list of web pages / files that will be served by the HTTP server. See @ref wiced_http_page_t for details and snippet apps for examples
 * @param[in] server_cert    A string containing the X.509 server certificate which is BASE64 DER encoded
 * @param[in] server_key     A string containing the key for the server_cert (BASE64 DER encoded)
 * @param[in] interface      Which network interface the HTTP server should listen on.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_https_server_start( wiced_https_server_t* server, uint16_t port, uint16_t max_sockets, const wiced_http_page_t* page_database, const char* server_cert, const char* server_key, wiced_interface_t interface, uint32_t url_processor_stack_size );

/**
 *  Stop a HTTPS server daemon (web server)
 *
 * @param[in] server   The structure workspace that was used with @ref wiced_https_server_start
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_https_server_stop( wiced_https_server_t* server );

/**
 * Register HTTP server callback(s)
 *
 * @param[in] server           : HTTP server
 * @param[in] receive_callback : Callback function that will called when a packet is received by the server
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_http_server_register_callbacks( wiced_http_server_t* server, http_server_callback_t receive_callback );

/**
 * Deregister HTTP server callback(s)
 *
 * @param[in] server : HTTP server
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_http_server_deregister_callbacks( wiced_http_server_t* server );

/**
 * Queue a disconnect request to the HTTP server
 *
 * @param[in] server               : HTTP server
 * @param[in] socket_to_disconnect : TCP socket to disconnect
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_http_server_queue_disconnect_request( wiced_http_server_t* server, wiced_tcp_socket_t* socket_to_disconnect );

/**
 * Initialise HTTP server stream
 *
 * @param[in] stream : HTTP server stream
 * @param[in] socket : TCP socket for the stream to use
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_http_response_stream_init( wiced_http_response_stream_t* stream, wiced_tcp_socket_t* socket );

/**
 * Deinitialise HTTP server stream
 *
 * @param[in] stream : HTTP server stream
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_http_response_stream_deinit( wiced_http_response_stream_t* stream );

/**
 * Enable chunked transfer encoding on the HTTP stream
 *
 * @param[in] stream : HTTP stream
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_http_response_stream_enable_chunked_transfer( wiced_http_response_stream_t* stream );

/**
 * Disable chunked transfer encoding on the HTTP stream
 *
 * @param[in] stream : HTTP stream
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_http_response_stream_disable_chunked_transfer( wiced_http_response_stream_t* stream );

/**
 * Write HTTP header to the TCP stream provided
 *
 * @param[in] stream         : HTTP stream to write the header into
 * @param[in] status_code    : HTTP status code
 * @param[in] content_length : HTTP content length to follow in bytes
 * @param[in] cache_type     : HTTP cache type (enabled or disabled)
 * @param[in] mime_type      : HTTP MIME type
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_http_response_stream_write_header( wiced_http_response_stream_t* stream, http_status_codes_t status_code, uint32_t content_length, http_cache_t cache_type, wiced_packet_mime_type_t mime_type );

/**
 * Write data to HTTP stream
 *
 * @param[in] stream : HTTP stream to write the data into
 * @param[in] data   : data to write
 * @param[in] length : data length in bytes
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_http_response_stream_write( wiced_http_response_stream_t* stream, const void* data, uint32_t length );

/**
 * Write resource to HTTP stream
 *
 * @param[in] stream   : HTTP stream to write the resource into
 * @param[in] resource : Pointer to resource
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_http_response_stream_write_resource( wiced_http_response_stream_t* stream, const resource_hnd_t* res_id );

/**
 * Flush HTTP stream
 *
 * @param[in] stream : HTTP stream to flush
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_http_response_stream_flush( wiced_http_response_stream_t* stream );

#ifdef __cplusplus
} /* extern "C" */
#endif
