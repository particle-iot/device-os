#pragma once

#include "coap_api.h"

#include "cloud/cloud.pb.h"

namespace particle::system {

/**
 * CoAP URI used with generic Protobuf-encoded requests.
 */
const auto PROTOBUF_REQUEST_URI = "g";

/**
 * CoAP method code used with generic Protobuf-encoded requests.
 */
const auto PROTOBUF_REQUEST_METHOD = COAP_METHOD_POST;

/**
 * Encode and send a `particle.cloud.Request` (see `cloud.proto`).
 *
 * @param path URI path.
 * @param status CoAP method code.
 * @param req Request data.
 * @param arg User argument passed to the `respCb`, `ackCb` and `errorCb` callbacks. Can be `nullptr`.
 * @param respCb Callback invoked when a response for the request is received. Can be `nullptr`.
 * @param ackCb Callback invoked when the request is acknowledged by the server. Can be `nullptr`.
 * @param errorCb Callback invoked if an error occurs while sending the request. Can be `nullptr`.
 * @return 0 on success, otherwise an error code defined by `system_error_t`.
 */
int sendProtobufRequest(const char* path, coap_method method, const particle_cloud_Request& req, void* arg = nullptr,
        coap_response_callback respCb = nullptr, coap_ack_callback ackCb = nullptr, coap_error_callback errorCb = nullptr);

/**
 * Decode a `particle.cloud.Request` (see `cloud.proto`).
 *
 * @param msg CoAP request message.
 * @param[out] req Request data.
 * @return 0 on success, otherwise an error code defined by `system_error_t`.
 */
int decodeProtobufRequest(coap_message* msg, particle_cloud_Request& req);

/**
 * Get the type of a `particle.cloud.Request` (see `cloud.proto`).
 *
 * Allows dispatching the request to the appropriate handler without fully decoding it.
 *
 * @param msg CoAP request message.
 * @param[out] type Value of the request's `type` field.
 * @return 0 on success, otherwise an error code defined by `system_error_t`.
 */
int getProtobufRequestType(coap_message* msg, int& type);

/**
 * Encode and send a `particle.cloud.Response` (see `cloud.proto`).
 *
 * @param reqId Request ID.
 * @param status CoAP response code.
 * @param resp Response data.
 * @param arg User argument passed to the `ackCb` and `errorCb` callbacks. Can be `nullptr`.
 * @param ackCb Callback invoked when the response is acknowledged by the server. Can be `nullptr`.
 * @param errorCb Callback invoked if an error occurs while sending the response. Can be `nullptr`.
 * @return 0 on success, otherwise an error code defined by `system_error_t`.
 */
int sendProtobufResponse(int reqId, coap_status status, const particle_cloud_Response& resp, void* arg = nullptr,
        coap_ack_callback ackCb = nullptr, coap_error_callback errorCb = nullptr);

/**
 * Send a `particle.cloud.Response` without payload data (see `cloud.proto`).
 *
 * @param reqId Request ID.
 * @param status CoAP response code.
 * @param result Value of the response's `result` field.
 * @param msg Diagnostic message. Can be `nullptr`.
 * @param arg User argument passed to the `ackCb` and `errorCb` callbacks. Can be `nullptr`.
 * @param ackCb Callback invoked when the response is acknowledged by the server. Can be `nullptr`.
 * @param errorCb Callback invoked if an error occurs while sending the response. Can be `nullptr`.
 * @return 0 on success, otherwise an error code defined by `system_error_t`.
 */
int sendEmptyProtobufResponse(int reqId, coap_status status, int result, const char* msg = nullptr, void* arg = nullptr,
        coap_ack_callback ackCb = nullptr, coap_error_callback errorCb = nullptr);

/**
 * Decode a `particle.cloud.Response` (see `cloud.proto`).
 *
 * @param msg CoAP response message.
 * @param[out] req Response data.
 * @return 0 on success, otherwise an error code defined by `system_error_t`.
 */
int decodeProtobufResponse(coap_message* msg, particle_cloud_Response& resp);

/**
 * Get the result of a `particle.cloud.Response` (see `cloud.proto`).
 *
 * @param msg CoAP response message.
 * @param status CoAP response code. Used for additional validation of the response data.
 * @param[out] result Value of the response's `result` field.
 * @return 0 on success, otherwise an error code defined by `system_error_t`.
 */
int getProtobufResponseResult(coap_message* msg, int status, int& result);

/**
 * Check the result of a `particle.cloud.Response` (see `cloud.proto`).
 *
 * @param msg CoAP response message.
 * @param status CoAP response code.
 * @return 0 on success, otherwise `SYSTEM_ERROR_SERVER_REQUEST_FAILED` if the CoAP response code or
 *         the `result` field of the Protobuf response data indicates an error, or another error code
 *         defined by `system_error_t`.
 */
int checkProtobufResponseResult(coap_message* msg, int status);

} // namespace particle::system
