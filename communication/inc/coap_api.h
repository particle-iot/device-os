/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdint.h>

/**
 * Maximum supported length of an URI path.
 */
#define COAP_MAX_URI_PATH_LENGTH 127

/**
 * Maximum size of payload data that can be sent or received without splitting the request or
 * response message into multiple CoAP messages.
 */
#define COAP_BLOCK_SIZE 1024

/**
 * Invalid request ID.
 */
#define COAP_INVALID_REQUEST_ID 0

#define COAP_CODE(_class, _detail) \
        (((_class & 0x07) << 5) | (_detail & 0x1f))

#define COAP_CODE_CLASS(_code) \
        ((_code >> 5) & 0x07)

#define COAP_CODE_DETAIL(_code) \
        (_code & 0x1f)

/**
 * Message.
 */
typedef struct coap_message coap_message;

/**
 * Payload data.
 */
typedef struct coap_payload coap_payload;

/**
 * Message option.
 */
typedef struct coap_option coap_option;

/**
 * Callback invoked when the status of the CoAP connection changes.
 *
 * @param error 0 if the status changed normally, otherwise an error code defined by the
 *        `system_error_t` enum.
 * @param status Current status as defined by the `coap_connection_status` enum.
 * @param arg User argument.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
typedef int (*coap_connection_callback)(int error, int status, void* arg);

/**
 * Callback invoked when a request message is received.
 *
 * The message instance must be destroyed by calling `coap_destroy_message()` when it's no longer
 * needed.
 *
 * @param msg Request message.
 * @param path URI path.
 * @param method Method code as defined by the `coap_method` enum.
 * @param req_id Request ID.
 * @param arg User argument.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
typedef int (*coap_request_callback)(coap_message* msg, const char* path, int method, int req_id, void* arg);

/**
 * Callback invoked when a response message is received.
 *
 * The message instance must be destroyed by calling `coap_destroy_message()` when it's no longer
 * needed.
 *
 * @param msg Response message.
 * @param status Response code as defined by the `coap_status` enum.
 * @param req_id ID of the request for which this response is being received.
 * @param arg User argument.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
typedef int (*coap_response_callback)(coap_message* msg, int status, int req_id, void* arg);

/**
 * Callback invoked when a block of a request or response message has been sent or received.
 *
 * @param msg Request or response message.
 * @param req_id ID of the request that started the message exchange.
 * @param arg User argument.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
typedef int (*coap_block_callback)(coap_message* msg, int req_id, void* arg);

/**
 * Callback invoked when a request or response message is acknowledged.
 *
 * @param req_id ID of the request that started the message exchange.
 * @param arg User argument.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
typedef int (*coap_ack_callback)(int req_id, void* arg);

/**
 * Callback invoked when an error occurs while sending a request or response message.
 *
 * @param error Error code as defined by the `system_error_t` enum.
 * @param req_id ID of the request that started the failed message exchange.
 * @param arg User argument.
 */
typedef void (*coap_error_callback)(int error, int req_id, void* arg);

/**
 * Method code.
 */
typedef enum coap_method {
    COAP_METHOD_GET = COAP_CODE(0, 1), ///< GET method.
    COAP_METHOD_POST = COAP_CODE(0, 2), ///< POST method.
    COAP_METHOD_PUT = COAP_CODE(0, 3), ///< PUT method.
    COAP_METHOD_DELETE = COAP_CODE(0, 4), ///< DELETE method.
} coap_method;

/**
 * Response code.
 */
typedef enum coap_status {
    // Success 2.xx
    COAP_STATUS_CREATED = COAP_CODE(2, 1), ///< 2.01 Created.
    COAP_STATUS_DELETED = COAP_CODE(2, 2), ///< 2.02 Deleted.
    COAP_STATUS_VALID = COAP_CODE(2, 3), ///< 2.03 Valid.
    COAP_STATUS_CHANGED = COAP_CODE(2, 4), ///< 2.04 Changed.
    COAP_STATUS_CONTENT = COAP_CODE(2, 5), ///< 2.05 Content.
    // Client Error 4.xx
    COAP_STATUS_BAD_REQUEST = COAP_CODE(4, 0), ///< 4.00 Bad Request.
    COAP_STATUS_UNAUTHORIZED = COAP_CODE(4, 1), ///< 4.01 Unauthorized.
    COAP_STATUS_BAD_OPTION = COAP_CODE(4, 2), ///< 4.02 Bad Option.
    COAP_STATUS_FORBIDDEN = COAP_CODE(4, 3), ///< 4.03 Forbidden.
    COAP_STATUS_NOT_FOUND = COAP_CODE(4, 4), ///< 4.04 Not Found.
    COAP_STATUS_METHOD_NOT_ALLOWED = COAP_CODE(4, 5), ///< 4.05 Method Not Allowed.
    COAP_STATUS_NOT_ACCEPTABLE = COAP_CODE(4, 6), ///< 4.06 Not Acceptable.
    COAP_STATUS_PRECONDITION_FAILED = COAP_CODE(4, 12), ///< 4.12 Precondition Failed.
    COAP_STATUS_REQUEST_ENTITY_TOO_LARGE = COAP_CODE(4, 13), ///< 4.13 Request Entity Too Large.
    COAP_STATUS_UNSUPPORTED_CONTENT_FORMAT = COAP_CODE(4, 15), ///< 4.15 Unsupported Content-Format.
    // Server Error 5.xx
    COAP_STATUS_INTERNAL_SERVER_ERROR = COAP_CODE(5, 0), ///< 5.00 Internal Server Error.
    COAP_STATUS_NOT_IMPLEMENTED = COAP_CODE(5, 1), ///< 5.01 Not Implemented.
    COAP_STATUS_BAD_GATEWAY = COAP_CODE(5, 2), ///< 5.02 Bad Gateway.
    COAP_STATUS_SERVICE_UNAVAILABLE = COAP_CODE(5, 3), ///< 5.03 Service Unavailable.
    COAP_STATUS_GATEWAY_TIMEOUT = COAP_CODE(5, 4), ///< 5.04 Gateway Timeout.
    COAP_STATUS_PROXYING_NOT_SUPPORTED = COAP_CODE(5, 5) ///< 5.05 Proxying Not Supported.
} coap_status;

/**
 * Option number.
 */
typedef enum coap_option_number {
    COAP_OPTION_IF_MATCH = 1, ///< If-Match.
    COAP_OPTION_URI_HOST = 3, ///< Uri-Host.
    COAP_OPTION_ETAG = 4, ///< ETag.
    COAP_OPTION_IF_NONE_MATCH = 5, ///< If-None-Match.
    COAP_OPTION_URI_PORT = 7, ///< Uri-Port.
    COAP_OPTION_LOCATION_PATH = 8, ///< Location-Path.
    COAP_OPTION_URI_PATH = 11, ///< Uri-Path.
    COAP_OPTION_CONTENT_FORMAT = 12, ///< Content-Format.
    COAP_OPTION_MAX_AGE = 14, ///< Max-Age.
    COAP_OPTION_URI_QUERY = 15, ///< Uri-Query.
    COAP_OPTION_ACCEPT = 17, ///< Accept.
    COAP_OPTION_LOCATION_QUERY = 20, ///< Location-Query.
    COAP_OPTION_PROXY_URI = 35, ///< Proxy-Uri.
    COAP_OPTION_PROXY_SCHEME = 39, ///< Proxy-Scheme.
    COAP_OPTION_SIZE1 = 60 ///< Size1.
} coap_option_number;

/**
 * Content format.
 *
 * https://www.iana.org/assignments/core-parameters/core-parameters.xhtml#content-formats
 */
typedef enum coap_format {
    COAP_FORMAT_TEXT_PLAIN = 0, // text/plain; charset=utf-8
    COAP_FORMAT_OCTET_STREAM = 42, // application/octet-stream
    COAP_FORMAT_JSON = 50, // application/json
    COAP_FORMAT_CBOR = 60 // application/cbor
} coap_format;

/**
 * Connection status.
 */
typedef enum coap_connection_status {
    COAP_CONNECTION_CLOSED = 0, ///< Connection is closed.
    COAP_CONNECTION_OPEN = 1 ///< Connection is open.
} coap_connection_status;

/**
 * Message option flags.
 */
typedef enum coap_message_flag {
    /**
     * Store the entire payload data of the message prior to sending an outgoing message or notifying
     * the user when an incoming message is received.
     */
    COAP_MESSAGE_FULL = 0x01
} coap_message_flag;

/**
 * Position relative to which to change the current position in payload data.
 *
 * @see `coap_set_payload_pos()`.
 */
typedef enum coap_whence {
    COAP_SEEK_SET = 1, ///< Set an absolute position.
    COAP_SEEK_CUR = 2, ///< Set a position relative to the current position.
    COAP_SEEK_END = 3 ///< Set a position relative to the end of the payload data.
} coap_whence;

/**
 * Result code.
 */
typedef enum coap_result {
    COAP_RESULT_WAIT_BLOCK = 1 ///< Waiting for the next block of the message to be sent or received.
} coap_result;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Register a connection handler.
 *
 * @param cb Handler callback.
 * @param arg User argument to pass to the callback.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int coap_add_connection_handler(coap_connection_callback cb, void* arg, void* reserved);

/**
 * Unregister a connection handler.
 *
 * @param cb Handler callback.
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void coap_remove_connection_handler(coap_connection_callback cb, void* reserved);

/**
 * Register a handler for incoming requests.
 *
 * If a handler is already registered for the given combination of base URI path and method code, it
 * will be replaced.
 *
 * @param path Base URI path. If `NULL`, the handler will be invoked for any request not matching
 *        any other handler.
 * @param method Method code as defined by the `coap_method` enum.
 * @param flags Flags defined by the `coap_message_flag` enum. Supported flags:
 *        - `COAP_MESSAGE_FULL`.
 * @param cb Handler callback.
 * @param arg User argument to pass to the callback.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int coap_add_request_handler(const char* path, int method, int flags, coap_request_callback cb, void* arg, void* reserved);

/**
 * Unregister a handler for incoming requests.
 *
 * @param path Base URI path.
 * @param method Method code as defined by the `coap_method` enum.
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void coap_remove_request_handler(const char* path, int method, void* reserved);

/**
 * Begin sending a request message.
 *
 * @param[out] msg Request message.
 * @param path URI path.
 * @param method Method code as defined by the `coap_method` enum.
 * @param timeout Request timeout in milliseconds. If 0, the default timeout is used.
 * @param flags Flags defined by the `coap_message_flag` enum. Supported flags:
 *        - `COAP_MESSAGE_FULL`.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return ID of the request on success, otherwise an error code defined by the `system_error_t` enum.
 */
int coap_begin_request(coap_message** msg, const char* path, int method, int timeout, int flags, void* reserved);

/**
 * Finish sending a request message.
 *
 * If the function call succeeds, the message instance must not be used again with any of the
 * functions of this API.
 *
 * @param msg Request message.
 * @param resp_cb Callback to invoke when a response for this request is received. Can be `NULL`.
 * @param ack_cb Callback to invoke when the request is acknowledged. Can be `NULL`.
 * @param error_cb Callback to invoke when an error occurs while sending the request. Can be `NULL`.
 * @param arg User argument to pass to the callbacks.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int coap_end_request(coap_message* msg, coap_response_callback resp_cb, coap_ack_callback ack_cb,
        coap_error_callback error_cb, void* arg, void* reserved);

/**
 * Begin sending a response message.
 *
 * @param[out] msg Response message.
 * @param status Response code as defined by the `coap_status` enum.
 * @param req_id ID of the request which this response is meant for.
 * @param flags Flags defined by the `coap_message_flag` enum. Supported flags:
 *        - `COAP_MESSAGE_FULL`.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int coap_begin_response(coap_message** msg, int status, int req_id, int flags, void* reserved);

/**
 * Finish sending a response message.
 *
 * If the function call succeeds, the message instance must not be used again with any of the
 * functions of this API.
 *
 * @param msg Response message.
 * @param ack_cb Callback to invoke when the response is acknowledged. Can be `NULL`.
 * @param error_cb Callback to invoke when an error occurs while sending the response. Can be `NULL`.
 * @param arg User argument to pass to the callbacks.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int coap_end_response(coap_message* msg, coap_ack_callback ack_cb, coap_error_callback error_cb,
        void* arg, void* reserved);

/**
 * Destroy a message.
 *
 * Destroying an outgoing request or response message before `coap_end_request()` or
 * `coap_end_response()` is called cancels the message exchange.
 *
 * @param msg Request or response message.
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void coap_destroy_message(coap_message* msg, void* reserved);

/**
 * Cancel an ongoing request.
 *
 * Cancelling a request prevents any callbacks associated with the respective message exchange from
 * being invoked by the API.
 *
 * If the caller still owns a message instance associated with the given exchange, it needs to be
 * destroyed via `coap_destroy_message()`.
 *
 * @param req_id ID of the request that started the message exchange.
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void coap_cancel_request(int req_id, void* reserved);

/**
 * Write payload data to the current block of a message.
 *
 * If the data can't be sent in one message block, the function will return `COAP_RESULT_WAIT_BLOCK`.
 * When that happens, the caller must stop writing the payload data until the `block_cb` callback is
 * invoked by the system to notify the caller that the next block of the message can be sent.
 *
 * All message options must be set prior to writing the payload data.
 *
 * @param msg Request or response message.
 * @param data Input buffer.
 * @param[in,out] size **in:** Number of bytes to write.
 *        **out:** Number of bytes written.
 * @param block_cb Callback to invoke when the next block of the message can be sent. Can be `NULL`.
 * @param error_cb Callback to invoke when an error occurs while sending the current block of the
 *        message. Can be `NULL`.
 * @param arg User argument to pass to the callbacks.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 or `COAP_RESULT_WAIT_BLOCK` on success, otherwise an error code defined by the
 *        `system_error_t` enum.
 */
int coap_write_block(coap_message* msg, const char* data, size_t* size, coap_block_callback block_cb,
        coap_error_callback error_cb, void* arg, void* reserved);

/**
 * Read payload data from the current block of a message.
 *
 * If the end of the current message block is reached and more blocks are expected to be received for
 * this message, the function will return `COAP_RESULT_WAIT_BLOCK`. The `block_cb` callback will be
 * invoked by the system to notify the caller that the next message block is available for reading.
 *
 * @param msg Request or response message.
 * @param[out] data Output buffer. Can be `NULL`.
 * @param[in,out] size **in:** Number of bytes to read.
 *        **out:** Number of bytes read.
 * @param block_cb Callback to invoke when the next block of the message is received. Can be `NULL`.
 * @param error_cb Callback to invoke when an error occurs while receiving the next block of the
 *        message. Can be `NULL`.
 * @param arg User argument to pass to the callbacks.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 or `COAP_RESULT_WAIT_BLOCK` on success, otherwise an error code defined by the
 *        `system_error_t` enum.
 */
int coap_read_block(coap_message* msg, char* data, size_t *size, coap_block_callback block_cb,
        coap_error_callback error_cb, void* arg, void* reserved);

/**
 * Read payload data from the current block of a message without changing the reading position in it.
 *
 * @param msg Request or response message.
 * @param[out] data Output buffer. Can be `NULL`.
 * @param size Number of bytes to read.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return Number of bytes read or an error code defined by the `system_error_t` enum.
 */
int coap_peek_block(coap_message* msg, char* data, size_t size, void* reserved);

/**
 * Create a payload data instance.
 *
 * @param payload Payload data instance.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int coap_create_payload(coap_payload** payload, void* reserved);

/**
 * Destroy a payload data instance.
 *
 * @param payload Payload data instance.
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void coap_destroy_payload(coap_payload* payload, void* reserved);

/**
 * Write payload data.
 *
 * @param payload Payload data instance.
 * @param data Input buffer.
 * @param size Number of bytes to write.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return On success, the number of bytes written. Otherwise, an error code defined by the
 *     `system_error_t` enum.
 */
int coap_write_payload(coap_payload* payload, const char* data, size_t size, void* reserved);

/**
 * Read payload data.
 *
 * @param payload Payload data instance.
 * @param data Output buffer.
 * @param size Number of bytes to read.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return On success, the number of bytes read. Otherwise, an error code defined by the
 *     `system_error_t` enum.
 */
int coap_read_payload(coap_payload* payload, char* data, size_t size, void* reserved);

/**
 * Set the current position in payload data.
 *
 * @param payload Payload data instance.
 * @param pos New position (can be negative).
 * @param whence Position relative to which to change the current position as defined by the
 *     `coap_whence` enum.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return On success, the new absolute position in the payload data. Otherwise, an error code
 *     defined by the `system_error_t` enum.
 */
int coap_set_payload_pos(coap_payload* payload, int pos, int whence, void* reserved);

/**
 * Get the current position in payload data.
 *
 * @param payload Payload data instance.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return On success, the current position in the payload data. Otherwise, an error code defined by
 *     the `system_error_t` enum.
 */
int coap_get_payload_pos(coap_payload* payload, void* reserved);

/**
 * Set the size of payload data.
 *
 * @param payload Payload data instance.
 * @param size New data size.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise, an error code defined by the `system_error_t` enum.
 */
int coap_set_payload_size(coap_payload* payload, size_t size, void* reserved);

/**
 * Get the size of payload data.
 *
 * @param payload Payload data instance.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return On success, the size of the payload data. Otherwise, an error code defined by the
 *     `system_error_t` enum.
 */
int coap_get_payload_size(coap_payload* payload, void* reserved);

/**
 * Set the payload data of a message.
 *
 * @param msg Request or response message.
 * @param payload Payload data instance.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise, an error code defined by the `system_error_t` enum.
 */
int coap_set_payload(coap_message* msg, coap_payload* payload, void* reserved);

/**
 * Get the payload data of a message.
 *
 * @param msg Request or response message.
 * @param[out] payload Payload data instance.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise, an error code defined by the `system_error_t` enum.
 */
int coap_get_payload(coap_message* msg, coap_payload** payload, void* reserved);

/**
 * Get the first message option with a given number.
 *
 * @param msg Request or response message.
 * @param[out] opt Message option. If the option cannot be found, the argument is set to `NULL`.
 * @param num Option number.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int coap_get_option(coap_message* msg, coap_option** opt, int num, void* reserved);

/**
 * Get the next message option.
 *
 * @param msg Request or response message.
 * @param[in,out] opt **in:** Current option. If `NULL`, the first option of the message is returned.
 *        **out:** Next option. If `NULL`, the option provided is the last option of the message.
 * @param[out] num Option number.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int coap_get_next_option(coap_message* msg, coap_option** opt, int* num, void* reserved);

/**
 * Get the value of a `uint` option.
 *
 * @param opt Message option.
 * @param[out] val Option value.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int coap_get_uint_option_value(coap_option* opt, unsigned* val, void* reserved);

/**
 * Get the value of a string option.
 *
 * The output is null-terminated unless the size of the output buffer is 0.
 *
 * @param opt Message option.
 * @param data Output buffer. Can be `NULL`.
 * @param size Size of the buffer.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return On success, the actual size of the option value not including the terminating null (can
 *        be greater than `size`). Otherwise, an error code defined by the `system_error_t` enum.
 */
int coap_get_string_option_value(coap_option* opt, char* data, size_t size, void* reserved);

/**
 * Get the value of an opaque option.
 *
 * @param opt Message option.
 * @param data Output buffer. Can be `NULL`.
 * @param size Size of the buffer.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return On success, the actual size of the option value (can be greater than `size`). Otherwise,
 *        an error code defined by the `system_error_t` enum.
 */
int coap_get_opaque_option_value(coap_option* opt, char* data, size_t size, void* reserved);

/**
 * Add an empty option to a message.
 *
 * @param msg Request of response message.
 * @param num Option number.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int coap_add_empty_option(coap_message* msg, int num, void* reserved);

/**
 * Add a `uint` option to a message.
 *
 * @param msg Request of response message.
 * @param num Option number.
 * @param val Option value.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int coap_add_uint_option(coap_message* msg, int num, unsigned val, void* reserved);

/**
 * Add a string option to a message.
 *
 * @param msg Request of response message.
 * @param num Option number.
 * @param str Option value.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int coap_add_string_option(coap_message* msg, int num, const char* str, void* reserved);

/**
 * Add an opaque option to a message.
 *
 * @param msg Request of response message.
 * @param num Option number.
 * @param data Option data.
 * @param size Size of the option data.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int coap_add_opaque_option(coap_message* msg, int num, const char* data, size_t size, void* reserved);

#ifdef __cplusplus
} // extern "C"
#endif
