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

#if !defined(DEBUG_BUILD) && !defined(UNIT_TEST)
#define NDEBUG // TODO: Define NDEBUG in release builds
#endif

#include <algorithm>
#include <cstring>

#include "v2/coap_channel.h"
#include "v2/coap_payload.h"
#include "v2/coap_options.h"
#include "coap_api.h"

#include "../../../system/inc/system_threading.h" // FIXME

#include "check.h"

using namespace particle;
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

int removeConnectionHandlerImpl(coap_connection_callback cb) {
    SYSTEM_THREAD_CONTEXT_SYNC(removeConnectionHandlerImpl(cb));

    CoapChannel::instance()->removeConnectionHandler(cb);
    return 0;
}

int removeRequestHandlerImpl(const char* path, int method) {
    SYSTEM_THREAD_CONTEXT_SYNC(removeRequestHandlerImpl(path, method));

    if (!isValidCoapMethod(method)) {
        return 0;
    }
    CoapChannel::instance()->removeRequestHandler(path, static_cast<coap_method>(method));
    return 0;
}

int destroyMessageImpl(coap_message* apiMsg) {
    SYSTEM_THREAD_CONTEXT_SYNC(destroyMessageImpl(apiMsg));

    auto msg = RefCountPtr<CoapMessage>::wrap(reinterpret_cast<CoapMessage*>(apiMsg));
    CoapChannel::instance()->disposeMessage(msg);
    return 0;
}

} // namespace

int coap_add_connection_handler(coap_connection_callback cb, void* arg, void* reserved) {
    SYSTEM_THREAD_CONTEXT_SYNC(coap_add_connection_handler(cb, arg, reserved));

    CHECK(CoapChannel::instance()->addConnectionHandler(cb, arg));
    return 0;
}

void coap_remove_connection_handler(coap_connection_callback cb, void* reserved) {
    removeConnectionHandlerImpl(cb);
}

int coap_add_request_handler(const char* path, int method, int flags, coap_request_callback cb, void* arg, void* reserved) {
    SYSTEM_THREAD_CONTEXT_SYNC(coap_add_request_handler(path, method, flags, cb, arg, reserved));

    if (!isValidCoapMethod(method)) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    CHECK(CoapChannel::instance()->addRequestHandler(path, static_cast<coap_method>(method), flags, cb, arg));
    return 0;
}

void coap_remove_request_handler(const char* path, int method, void* reserved) {
    removeRequestHandlerImpl(path, method);
}

int coap_begin_request(coap_message** apiMsg, const char* path, int method, int timeout, int flags, void* reserved) {
    SYSTEM_THREAD_CONTEXT_SYNC(coap_begin_request(apiMsg, path, method, timeout, flags, reserved));

    if (!isValidCoapMethod(method)) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    RefCountPtr<CoapMessage> msg;
    auto reqId = CHECK(CoapChannel::instance()->beginRequest(msg, path, static_cast<coap_method>(method), timeout, flags));
    assert(apiMsg);
    *apiMsg = reinterpret_cast<coap_message*>(msg.unwrap()); // Transfer ownership over the message to the calling code
    return reqId;
}

int coap_end_request(coap_message* apiMsg, coap_response_callback respCb, coap_ack_callback ackCb,
        coap_error_callback errorCb, void* arg, void* reserved) {
    // TODO: Use a mutex and a separate queue instead of the threading macros in all the CoAP API
    // functions to avoid blocking the application thread even if the operation cannot be performed
    // immediately
    SYSTEM_THREAD_CONTEXT_SYNC(coap_end_request(apiMsg, respCb, ackCb, errorCb, arg, reserved));

    auto msg = RefCountPtr<CoapMessage>::wrap(reinterpret_cast<CoapMessage*>(apiMsg));
    int r = CoapChannel::instance()->endRequest(msg, respCb, ackCb, errorCb, arg);
    if (r < 0) {
        msg.unwrap(); // The calling code retains ownership over the message if an error occurred
        return r;
    }
    return 0;
}

int coap_begin_response(coap_message** apiMsg, int status, int reqId, int flags, void* reserved) {
    SYSTEM_THREAD_CONTEXT_SYNC(coap_begin_response(apiMsg, status, reqId, flags, reserved));

    RefCountPtr<CoapMessage> msg;
    CHECK(CoapChannel::instance()->beginResponse(msg, status, reqId, flags));
    assert(apiMsg);
    *apiMsg = reinterpret_cast<coap_message*>(msg.unwrap()); // Transfer ownership over the message to the calling code
    return 0;
}

int coap_end_response(coap_message* apiMsg, coap_ack_callback ackCb, coap_error_callback errorCb, void* arg, void* reserved) {
    SYSTEM_THREAD_CONTEXT_SYNC(coap_end_response(apiMsg, ackCb, errorCb, arg, reserved));

    auto msg = RefCountPtr<CoapMessage>::wrap(reinterpret_cast<CoapMessage*>(apiMsg));
    int r = CoapChannel::instance()->endResponse(msg, ackCb, errorCb, arg);
    if (r < 0) {
        msg.unwrap(); // The calling code retains ownership over the message if an error occurred
        return r;
    }
    return 0;
}

void coap_destroy_message(coap_message* apiMsg, void* reserved) {
    destroyMessageImpl(apiMsg);
}

int coap_cancel_request(int reqId, void* reserved) {
    SYSTEM_THREAD_CONTEXT_SYNC(coap_cancel_request(reqId, reserved));

    int r = CHECK(CoapChannel::instance()->cancelRequest(reqId));
    return r; // 0 or COAP_RESULT_CANCELLED
}

int coap_write_block(coap_message* apiMsg, const char* data, size_t* size, coap_block_callback blockCb,
        coap_error_callback errorCb, void* arg, void* reserved) {
    // TODO: coap_*_block() functions operate with the shared message buffer and thus cannot be
    // safely used in an application or user thread
    if (!SYSTEM_THREAD_CURRENT()) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    RefCountPtr<CoapMessage> msg(reinterpret_cast<CoapMessage*>(apiMsg));
    int r = CHECK(CoapChannel::instance()->writeBlock(msg, data, *size, blockCb, errorCb, arg));
    return r; // 0 or COAP_RESULT_WAIT_BLOCK
}

int coap_read_block(coap_message* apiMsg, char* data, size_t* size, coap_block_callback blockCb,
        coap_error_callback errorCb, void* arg, void* reserved) {
    if (!SYSTEM_THREAD_CURRENT()) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    RefCountPtr<CoapMessage> msg(reinterpret_cast<CoapMessage*>(apiMsg));
    int r = CHECK(CoapChannel::instance()->readBlock(msg, data, *size, blockCb, errorCb, arg));
    return r; // 0 or COAP_RESULT_WAIT_BLOCK
}

int coap_peek_block(coap_message* apiMsg, char* data, size_t size, void* reserved) {
    if (!SYSTEM_THREAD_CURRENT()) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    RefCountPtr<CoapMessage> msg(reinterpret_cast<CoapMessage*>(apiMsg));
    size_t n = CHECK(CoapChannel::instance()->peekBlock(msg, data, size));
    return n;
}

int coap_create_payload(coap_payload** apiPayload, size_t max_heap_size, void* reserved) {
    auto payload = makeRefCountPtr<CoapPayload>(max_heap_size);
    if (!payload) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    assert(apiPayload);
    *apiPayload = reinterpret_cast<coap_payload*>(payload.unwrap()); // Transfer ownership over the payload object to the calling code
    return 0;
}

void coap_destroy_payload(coap_payload* apiPayload, void* reserved) {
    RefCountPtr<CoapPayload>::wrap(reinterpret_cast<CoapPayload*>(apiPayload));
}

int coap_write_payload(coap_payload* apiPayload, const char* data, size_t size, size_t pos, void* reserved) {
    auto payload = reinterpret_cast<CoapPayload*>(apiPayload);
    size_t n = CHECK(payload->write(data, size, pos));
    return n;
}

int coap_read_payload(coap_payload* apiPayload, char* data, size_t size, size_t pos, void* reserved) {
    auto payload = reinterpret_cast<CoapPayload*>(apiPayload);
    size_t n = CHECK(payload->read(data, size, pos));
    return n;
}

int coap_set_payload_size(coap_payload* apiPayload, size_t size, void* reserved) {
    auto payload = reinterpret_cast<CoapPayload*>(apiPayload);
    CHECK(payload->setSize(size));
    return 0;
}

int coap_get_payload_size(coap_payload* apiPayload, void* reserved) {
    auto payload = reinterpret_cast<CoapPayload*>(apiPayload);
    return payload->size();
}

int coap_set_payload(coap_message* apiMsg, coap_payload* apiPayload, void* reserved) {
    RefCountPtr<CoapMessage> msg(reinterpret_cast<CoapMessage*>(apiMsg));
    RefCountPtr<CoapPayload> payload(reinterpret_cast<CoapPayload*>(apiPayload));
    CHECK(CoapChannel::instance()->setPayload(msg, payload));
    return 0;
}

int coap_get_payload(coap_message* apiMsg, coap_payload** apiPayload, void* reserved) {
    RefCountPtr<CoapMessage> msg(reinterpret_cast<CoapMessage*>(apiMsg));
    RefCountPtr<CoapPayload> payload;
    CHECK(CoapChannel::instance()->getPayload(msg, payload));
    assert(apiPayload);
    *apiPayload = reinterpret_cast<coap_payload*>(payload.unwrap());
    return 0;
}

int coap_get_option(coap_message* apiMsg, coap_option** apiOpt, int num, void* reserved) {
    const CoapOptions::Entry* opt = nullptr;
    if (num >= 0) {
        RefCountPtr<CoapMessage> msg(reinterpret_cast<CoapMessage*>(apiMsg));
        CHECK(CoapChannel::instance()->getOption(msg, num, opt));
    }
    assert(apiOpt);
    *apiOpt = const_cast<coap_option*>(reinterpret_cast<const coap_option*>(opt));
    return 0;
}

int coap_get_next_option(coap_message* apiMsg, coap_option** apiOpt, int* num, void* reserved) {
    RefCountPtr<CoapMessage> msg(reinterpret_cast<CoapMessage*>(apiMsg));
    assert(apiOpt);
    auto opt = reinterpret_cast<const CoapOptions::Entry*>(*apiOpt);
    if (opt) {
        opt = opt->next();
    } else {
        CHECK(CoapChannel::instance()->getFirstOption(msg, opt));
    }
    *apiOpt = const_cast<coap_option*>(reinterpret_cast<const coap_option*>(opt));
    assert(num);
    *num = opt ? opt->number() : 0u;
    return 0;
}

int coap_get_uint_option_value(coap_option* apiOpt, unsigned* val, void* reserved) {
    auto opt = reinterpret_cast<const CoapOptions::Entry*>(apiOpt);
    assert(opt && val);
    *val = opt->toUint();
    return 0;
}

int coap_get_string_option_value(coap_option* apiOpt, char* data, size_t size, void* reserved) {
    auto opt = reinterpret_cast<const CoapOptions::Entry*>(apiOpt);
    assert(opt);
    if (size > 0) {
        size_t n = std::min(size - 1, opt->size());
        std::memcpy(data, opt->data(), n);
        data[n] = '\0';
    }
    return opt->size();
}

int coap_get_opaque_option_value(coap_option* apiOpt, char* data, size_t size, void* reserved) {
    auto opt = reinterpret_cast<const CoapOptions::Entry*>(apiOpt);
    assert(opt);
    std::memcpy(data, opt->data(), std::min(size, opt->size()));
    return opt->size();
}

int coap_add_empty_option(coap_message* apiMsg, int num, void* reserved) {
    if (num < 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    RefCountPtr<CoapMessage> msg(reinterpret_cast<CoapMessage*>(apiMsg));
    CHECK(CoapChannel::instance()->addOption(msg, num, nullptr /* data */, 0 /* size */));
    return 0;
}

int coap_add_uint_option(coap_message* apiMsg, int num, unsigned val, void* reserved) {
    if (num < 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    RefCountPtr<CoapMessage> msg(reinterpret_cast<CoapMessage*>(apiMsg));
    CHECK(CoapChannel::instance()->addOption(msg, num, val));
    return 0;
}

int coap_add_string_option(coap_message* apiMsg, int num, const char* val, void* reserved) {
    if (num < 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    RefCountPtr<CoapMessage> msg(reinterpret_cast<CoapMessage*>(apiMsg));
    CHECK(CoapChannel::instance()->addOption(msg, num, val, std::strlen(val)));
    return 0;
}

int coap_add_opaque_option(coap_message* apiMsg, int num, const char* data, size_t size, void* reserved) {
    if (num < 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    RefCountPtr<CoapMessage> msg(reinterpret_cast<CoapMessage*>(apiMsg));
    CHECK(CoapChannel::instance()->addOption(msg, num, data, size));
    return 0;
}
