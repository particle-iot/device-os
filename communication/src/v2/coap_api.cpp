/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
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

#include "v2/coap_channel.h"
#include "coap_api.h"

#include "check.h"

using namespace particle::protocol;
using namespace particle::protocol::v2;

namespace {

bool isValidCoapMethod(int method) {
    switch (method) {
    case COAP_METHOD_GET:
    case COAP_METHOD_POST:
    case COAP_METHOD_PUT:
    case COAP_METHOD_DELETE:
        return true;
    default:
        return false;
    }
}

} // namespace

int coap_add_connection_handler(coap_connection_callback cb, void* arg, void* reserved) {
    CHECK(CoapChannel::instance()->addConnectionHandler(cb, arg));
    return 0;
}

void coap_remove_connection_handler(coap_connection_callback cb, void* reserved) {
    CoapChannel::instance()->removeConnectionHandler(cb);
}

int coap_add_request_handler(const char* path, int method, int flags, coap_request_callback cb, void* arg, void* reserved) {
    if (!isValidCoapMethod(method) || flags != 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    CHECK(CoapChannel::instance()->addRequestHandler(path, static_cast<coap_method>(method), flags, cb, arg));
    return 0;
}

void coap_remove_request_handler(const char* path, int method, void* reserved) {
    if (!isValidCoapMethod(method)) {
        return;
    }
    CoapChannel::instance()->removeRequestHandler(path, static_cast<coap_method>(method));
}

int coap_begin_request(coap_message** msg, const char* path, int method, int timeout, int flags, void* reserved) {
    if (!isValidCoapMethod(method) || flags != 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    auto reqId = CHECK(CoapChannel::instance()->beginRequest(msg, path, static_cast<coap_method>(method), timeout, flags));
    return reqId;
}

int coap_end_request(coap_message* msg, coap_response_callback resp_cb, coap_ack_callback ack_cb,
        coap_error_callback error_cb, void* arg, void* reserved) {
    CHECK(CoapChannel::instance()->endRequest(msg, resp_cb, ack_cb, error_cb, arg));
    return 0;
}

int coap_begin_response(coap_message** msg, int status, int req_id, int flags, void* reserved) {
    CHECK(CoapChannel::instance()->beginResponse(msg, status, req_id, flags));
    return 0;
}

int coap_end_response(coap_message* msg, coap_ack_callback ack_cb, coap_error_callback error_cb, void* arg, void* reserved) {
    CHECK(CoapChannel::instance()->endResponse(msg, ack_cb, error_cb, arg));
    return 0;
}

void coap_destroy_message(coap_message* msg, void* reserved) {
    CoapChannel::instance()->destroyMessage(msg);
}

void coap_cancel_request(int req_id, void* reserved) {
    CoapChannel::instance()->cancelRequest(req_id);
}

int coap_write_block(coap_message* msg, const char* data, size_t* size, coap_block_callback block_cb,
        coap_error_callback error_cb, void* arg, void* reserved) {
    int r = CHECK(CoapChannel::instance()->writeBlock(msg, data, *size, block_cb, error_cb, arg));
    return r; // 0 or COAP_RESULT_WAIT_BLOCK
}

int coap_read_block(coap_message* msg, char* data, size_t* size, coap_block_callback block_cb,
        coap_error_callback error_cb, void* arg, void* reserved) {
    int r = CHECK(CoapChannel::instance()->readBlock(msg, data, *size, block_cb, error_cb, arg));
    return r; // 0 or COAP_RESULT_WAIT_BLOCK
}

int coap_peek_block(coap_message* msg, char* data, size_t size, void* reserved) {
    size_t n = CHECK(CoapChannel::instance()->peekBlock(msg, data, size));
    return n;
}

int coap_create_payload(coap_payload** payload, void* reserved) {
    CHECK(CoapChannel::instance()->createPayload(payload));
    return 0;
}

void coap_destroy_payload(coap_payload* payload, void* reserved) {
    CoapChannel::instance()->destroyPayload(payload);
}

int coap_write_payload(coap_payload* payload, const char* data, size_t size, void* reserved) {
    size_t n = CHECK(CoapChannel::instance()->writePayload(payload, data, size));
    return n;
}

int coap_read_payload(coap_payload* payload, char* data, size_t size, void* reserved) {
    size_t n = CHECK(CoapChannel::instance()->readPayload(payload, data, size));
    return n;
}

int coap_set_payload_pos(coap_payload* payload, int pos, int whence, void* reserved) {
    CHECK(CoapChannel::instance()->setPayloadPos(payload, pos, whence));
    return 0;
}

int coap_get_payload_pos(coap_payload* payload, void* reserved) {
    size_t pos = CHECK(CoapChannel::instance()->getPayloadPos(payload));
    return pos;
}

int coap_set_payload_size(coap_payload* payload, size_t size, void* reserved) {
    CHECK(CoapChannel::instance()->setPayloadSize(payload, size));
    return 0;
}

int coap_get_payload_size(coap_payload* payload, void* reserved) {
    size_t size = CHECK(CoapChannel::instance()->getPayloadSize(payload));
    return size;
}

int coap_set_payload(coap_message* msg, coap_payload* payload, void* reserved) {
    CHECK(CoapChannel::instance()->setPayload(msg, payload));
    return 0;
}

int coap_get_payload(coap_message* msg, coap_payload** payload, void* reserved) {
    CHECK(CoapChannel::instance()->getPayload(msg, payload));
    return 0;
}

int coap_get_option(coap_message* msg, coap_option** opt, int num, void* reserved) {
    CHECK(CoapChannel::instance()->getOption(msg, opt, num));
    return 0;
}

int coap_get_next_option(coap_message* msg, coap_option** opt, int* num, void* reserved) {
    CHECK(CoapChannel::instance()->getNextOption(msg, opt, num));
    return 0;
}

int coap_get_uint_option_value(coap_option* opt, unsigned* val, void* reserved) {
    CHECK(CoapChannel::instance()->getUintOptionValue(opt, val));
    return 0;
}

int coap_get_string_option_value(coap_option* opt, char* data, size_t size, void* reserved) {
    CHECK(CoapChannel::instance()->getStringOptionValue(opt, data, size));
    return 0;
}

int coap_get_opaque_option_value(coap_option* opt, char* data, size_t size, void* reserved) {
    CHECK(CoapChannel::instance()->getOpaqueOptionValue(opt, data, size));
    return 0;
}

int coap_add_empty_option(coap_message* msg, int num, void* reserved) {
    CHECK(CoapChannel::instance()->addEmptyOption(msg, num));
    return 0;
}

int coap_add_uint_option(coap_message* msg, int num, unsigned val, void* reserved) {
    CHECK(CoapChannel::instance()->addUintOption(msg, num, val));
    return 0;
}

int coap_add_string_option(coap_message* msg, int num, const char* val, void* reserved) {
    CHECK(CoapChannel::instance()->addStringOption(msg, num, val));
    return 0;
}

int coap_add_opaque_option(coap_message* msg, int num, const char* data, size_t size, void* reserved) {
    CHECK(CoapChannel::instance()->addOpaqueOption(msg, num, data, size));
    return 0;
}
