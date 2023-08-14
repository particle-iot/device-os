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

#include "logging.h"

LOG_SOURCE_CATEGORY("coap")

#include <algorithm>
#include <memory>
#include <cstring>
#include <cstdint>

#include "coap_channel_new.h"

#include "coap_message_encoder.h"
#include "coap_message_decoder.h"
#include "protocol.h"
#include "spark_protocol_functions.h"

#include "endian_util.h"
#include "enumclass.h"
#include "scope_guard.h"
#include "check.h"

#define CHECK_PROTOCOL(_expr) \
        do { \
            auto _r = _expr; \
            if (_r != ::particle::protocol::ProtocolError::NO_ERROR) { \
                return this->handleProtocolError(_r); \
            } \
        } while (false)

namespace particle::protocol::experimental {

namespace {

typedef uint32_t TokenValue;

const size_t MAX_TOKEN_SIZE = sizeof(token_t); // TODO: Support longer tokens

static_assert(MAX_TOKEN_SIZE > 0 && MAX_TOKEN_SIZE <= sizeof(TokenValue));

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

// TODO: Use an intrusive list container
template<typename T>
inline void removeFromList(T* elem, T*& head) {
    T* prev = nullptr;
    T* e = head;
    while (e) {
        if (e == elem) {
            break;
        }
        prev = e;
        e = e->next;
    }
    if (e) {
        if (prev) {
            prev->next = e->next;
        } else {
            head = e->next;
        }
    }
}

} // namespace

struct CoapMessage {
    enum State {
        NEW, // Message created
        READING, // Reading payload data
        WRITING, // Writing payload data
        WAITING_ACK, // Waiting an ACK for this message
        WAITING_RESPONSE // Waiting a response for this request
    };

    coap_ack_callback ackCallback; // Callback to invoke when the message is acknowledged
    coap_error_callback errorCallback; // Callback to invoke when an error occurs
    void* callbackArg; // User argument to pass to the callbacks
    CoapMessage* next; // Next message in the list
    char* pos; // Current position in the message buffer
    char* end; // End of the message buffer
    TokenValue tokenValue; // CoAP token
    size_t tokenSize; // Token size
    int sessionId; // Session ID
    int requestId; // ID of the request that started the exchange
    int coapId; // CoAP message ID
    State state; // Message state
    bool isRequest; // Whether this is a request message

    union {
        // Fields specific to a request message
        struct {
            coap_response_callback responseCallback; // Callback to invoke when a response for this request is received
            coap_method method; // CoAP method code
            char uri; // Request URI. TODO: Support longer URIs
        };
        // Fields specific to a response message
        struct {
            int responseCode; // CoAP response code
        };
    };

    CoapMessage() :
            ackCallback(nullptr),
            errorCallback(nullptr),
            callbackArg(nullptr),
            next(nullptr),
            pos(nullptr),
            end(nullptr),
            tokenValue(0),
            tokenSize(0),
            sessionId(0),
            requestId(0),
            coapId(0),
            state(State::NEW),
            isRequest(false),
            responseCallback(nullptr),
            method(),
            uri(0) {
    }
};

struct CoapChannel::RequestHandler {
    coap_request_callback callback;
    void* callbackArg;
    RequestHandler* next;
    coap_method method;
    char uri; // TODO: Support longer URIs

    RequestHandler(char uri, coap_method method, coap_request_callback callback, void* callbackArg) :
            callback(callback),
            callbackArg(callbackArg),
            next(nullptr),
            method(method),
            uri(uri) {
    }
};

CoapChannel::CoapChannel() :
        reqHandlers_(nullptr),
        sentReqs_(nullptr),
        recvReqs_(nullptr),
        unackMsgs_(nullptr),
        proto_(spark_protocol_instance()),
        lastReqId_(0),
        curReqId_(0),
        sessId_(0),
        open_(false) {
}

CoapChannel::~CoapChannel() {
    if (sentReqs_ || recvReqs_ || recvReqs_) {
        LOG(ERROR, "Destroying channel while a CoAP exchange is in progress");
    }
    auto h = reqHandlers_;
    while (h) {
        auto next = h->next;
        delete h;
        h = next;
    }
    close();
}

int CoapChannel::beginRequest(const char* uri, coap_method method, CoapMessage*& msg) {
    if (std::strlen(uri) != 1) {
        return SYSTEM_ERROR_INVALID_ARGUMENT; // TODO: Support longer URIs
    }
    if (!open_) {
        return SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    std::unique_ptr<CoapMessage> req(new(std::nothrow) CoapMessage());
    if (!req) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    auto reqId = ++lastReqId_;
    req->requestId = reqId;
    req->sessionId = sessId_;
    req->uri = *uri;
    req->method = method;
    req->state = CoapMessage::WRITING;
    req->isRequest = true;
    msg = req.release();
    return reqId;
}

int CoapChannel::endRequest(CoapMessage* msg, coap_response_callback respCallback, coap_ack_callback ackCallback,
        coap_error_callback errorCallback, void* callbackArg) {
    if (!msg->isRequest) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (msg->state != CoapMessage::WRITING) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!open_ || msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    if (!curReqId_) {
        CHECK(initProtocolMessage(msg));
    } else if (curReqId_ != msg->requestId) {
        // TODO: Support asynchronous writing to multiple message instances
        return SYSTEM_ERROR_INTERNAL;
    }
    msgBuf_.set_length(msg->end - (char*)msgBuf_.buf());
    CHECK_PROTOCOL(proto_->get_channel().send(msgBuf_));
    msg->coapId = msgBuf_.get_id();
    msg->next = unackMsgs_;
    unackMsgs_ = msg;
    msgBuf_.clear();
    curReqId_ = 0;
    return 0;
}

int CoapChannel::beginResponse(int code, int requestId, CoapMessage*& msg) {
    if (!open_) {
        return SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    auto req = recvReqs_;
    for (; req; req = req->next) {
        if (req->requestId == requestId) {
            break;
        }
    }
    if (!req) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    std::unique_ptr<CoapMessage> resp(new(std::nothrow) CoapMessage());
    if (!resp) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    resp->requestId = req->requestId;
    resp->sessionId = sessId_;
    resp->tokenValue = req->tokenValue;
    resp->tokenSize = req->tokenSize;
    resp->responseCode = code;
    resp->state = CoapMessage::WRITING;
    msg = resp.release();
    return 0;
}

int CoapChannel::endResponse(CoapMessage* msg, coap_ack_callback ackCallback, coap_error_callback errorCallback,
        void* callbackArg) {
    if (msg->isRequest) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (msg->state != CoapMessage::WRITING) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!open_ || msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    if (!curReqId_) {
        CHECK(initProtocolMessage(msg));
    } else if (curReqId_ != msg->requestId) {
        // TODO: Support asynchronous writing to multiple message instances
        return SYSTEM_ERROR_INTERNAL;
    }
    msgBuf_.set_length(msg->end - (char*)msgBuf_.buf());
    CHECK_PROTOCOL(proto_->get_channel().send(msgBuf_));
    msg->coapId = msgBuf_.get_id();
    msg->next = unackMsgs_;
    unackMsgs_ = msg;
    msgBuf_.clear();
    curReqId_ = 0;
    return 0;
}

int CoapChannel::writePayload(CoapMessage* msg, const char* data, size_t& size, coap_block_callback /* blockCallback */,
        coap_error_callback errorCallback, void* callbackArg) {
    if (msg->state != CoapMessage::WRITING) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!open_ || msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    if (size == 0) {
        return 0;
    }
    if (msg->pos + size > msg->end) {
        // TODO: Support blockwise transfer
        return SYSTEM_ERROR_TOO_LARGE;
    }
    if (!curReqId_) {
        CHECK(initProtocolMessage(msg));
        *msg->pos++ = 0xff; // Payload marker
    } else if (curReqId_ != msg->requestId) {
        // TODO: Support asynchronous writing to multiple message instances
        return SYSTEM_ERROR_INTERNAL;
    }
    memcpy(msg->pos, data, size);
    msg->pos += size;
    msg->errorCallback = errorCallback;
    msg->callbackArg = callbackArg;
    curReqId_ = msg->requestId;
    return 0;
}

int CoapChannel::readPayload(CoapMessage* msg, char* data, size_t& size, coap_block_callback /* blockCallback */,
        coap_error_callback errorCallback, void* callbackArg) {
    if (msg->state != CoapMessage::READING) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!open_ || msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    if (msg->pos == msg->end) {
        // TODO: Support blockwise transfer
        return SYSTEM_ERROR_END_OF_STREAM;
    }
    if (curReqId_ != msg->requestId) {
        // TODO: Support asynchronous reading from multiple message instances
        return SYSTEM_ERROR_INTERNAL;
    }
    auto n = std::min<size_t>(size, msg->end - msg->pos);
    memcpy(data, msg->pos, n);
    msg->pos += n;
    msg->errorCallback = errorCallback;
    msg->callbackArg = callbackArg;
    if (msg->pos == msg->end) {
        // Release the message buffer
        msgBuf_.clear();
        curReqId_ = 0;
    }
    size = n;
    return 0;
}

int CoapChannel::peekPayload(CoapMessage* msg, char* data, size_t size) {
    if (msg->state != CoapMessage::READING) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!open_ || msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    if (curReqId_ != msg->requestId) {
        // TODO: Support asynchronous reading from multiple message instances
        return SYSTEM_ERROR_INTERNAL;
    }
    if (msg->pos == msg->end) {
        return SYSTEM_ERROR_END_OF_STREAM;
    }
    auto n = std::min<size_t>(size, msg->end - msg->pos);
    memcpy(data, msg->pos, n);
    return n;
}

void CoapChannel::destroyMessage(CoapMessage* msg) {
    switch (msg->state) {
    case CoapMessage::READING: {
        if (msg->isRequest) {
            removeFromList(msg, recvReqs_);
        }
        break;
    }
    case CoapMessage::WAITING_ACK: {
        removeFromList(msg, unackMsgs_);
        break;
    }
    case CoapMessage::WAITING_RESPONSE: {
        removeFromList(msg, sentReqs_);
        break;
    }
    default:
        break;
    }
    if (curReqId_ == msg->requestId) {
        // Release the message buffer
        msgBuf_.clear();
        curReqId_ = 0;
    }
    delete msg;
}

int CoapChannel::addRequestHandler(const char* uri, coap_method method, coap_request_callback callback, void* callbackArg) {
    if (!callback || std::strlen(uri) != 1) { // TODO: Support longer URIs
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    auto h = reqHandlers_;
    for (; h; h = h->next) {
        if (h->uri == *uri && h->method == method) {
            h->callback = callback;
            h->callbackArg = callbackArg;
            break;
        }
    }
    if (!h) {
        std::unique_ptr<RequestHandler> h(new(std::nothrow) RequestHandler(*uri, method, callback, callbackArg));
        if (!h) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        h->next = reqHandlers_;
        reqHandlers_ = h.release();
    }
    return 0;
}

void CoapChannel::removeRequestHandler(const char* uri, coap_method method) {
    RequestHandler* prev = nullptr;
    for (auto h = reqHandlers_; h; h = h->next) {
        if (h->uri == *uri && h->method == method) {
            if (prev) {
                prev->next = h->next;
            }
            if (reqHandlers_ == h) {
                reqHandlers_ = h->next;
            }
            delete h;
            break;
        }
        prev = h;
    }
}

void CoapChannel::close(int error) {
    if (!open_) {
        return;
    }
    // Mark the channel as closed before invoking any user callbacks
    open_ = false;
    cancelMessages(sentReqs_, error);
    sentReqs_ = nullptr;
    cancelMessages(unackMsgs_, error);
    unackMsgs_ = nullptr;
    recvReqs_ = nullptr; // These are owned by the user code
    msgBuf_.clear();
    curReqId_ = 0;
    ++sessId_;
}

int CoapChannel::handleCon(const Message& msg) {
    CoapMessageDecoder d;
    CHECK(d.decode((const char*)msg.buf(), msg.length()));
    if (d.type() != CoapType::CON) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    // All requests and responses sent or received via this API must have a non-empty token
    if (!d.hasToken() || d.tokenSize() > MAX_TOKEN_SIZE) {
        return 0;
    }
    if (isCoapRequestCode(d.code())) {
        return handleRequest(d);
    }
    return handleResponse(d);
}

int CoapChannel::handleAck(int coapId) {
    auto msg = unackMsgs_;
    for (; msg; msg = msg->next) {
        if (msg->coapId == coapId) {
            break;
        }
    }
    if (!msg) {
        return 0;
    }
    auto reqId = msg->requestId;
    auto ackCb = msg->ackCallback;
    auto cbArg = msg->callbackArg;
    removeFromList(msg, unackMsgs_);
    if (msg->isRequest) {
        msg->state = CoapMessage::WAITING_RESPONSE;
        msg->next = sentReqs_;
        sentReqs_ = msg;
    } else {
        delete msg;
    }
    if (ackCb) {
        CHECK(ackCb(reqId, cbArg));
    }
    return Result::HANDLED;
}

int CoapChannel::handleRst(int coapId) {
    auto msg = unackMsgs_;
    for (; msg; msg = msg->next) {
        if (msg->coapId == coapId) {
            break;
        }
    }
    if (!msg) {
        return 0;
    }
    auto reqId = msg->requestId;
    auto errCb = msg->errorCallback;
    auto cbArg = msg->callbackArg;
    removeFromList(msg, unackMsgs_);
    delete msg;
    if (errCb) {
        errCb(SYSTEM_ERROR_COAP_MESSAGE_RESET, reqId, cbArg);
    }
    return Result::HANDLED;
}

CoapChannel* CoapChannel::instance() {
    static CoapChannel channel;
    return &channel;
}

int CoapChannel::handleRequest(CoapMessageDecoder& d) {
    // Parse the request
    char uri = '/'; // TODO: Support longer URIs
    bool hasUri = false;
    auto it = d.findOption(CoapOption::URI_PATH);
    while (it) {
        if (it.size() > 1) {
            return 0;
        }
        if (it.size() > 0) {
            if (hasUri) {
                return 0;
            }
            uri = *it.data();
            hasUri = true;
        }
        if (it.next() && it.option() != CoapOption::URI_PATH) {
            break;
        }
    }
    // Find a request handler
    auto method = d.code();
    auto h = reqHandlers_;
    for (; h; h = h->next) {
        if (h->uri == uri && h->method == method) {
            break;
        }
    }
    if (!h) {
        return 0;
    }
    // Create a request message
    if (curReqId_) {
        // TODO: Support asynchronous reading from multiple message instances
        return SYSTEM_ERROR_INTERNAL;
    }
    std::unique_ptr<CoapMessage> req(new(std::nothrow) CoapMessage());
    if (!req) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    auto reqId = ++lastReqId_;
    req->requestId = reqId;
    req->sessionId = sessId_;
    req->uri = uri;
    req->method = static_cast<coap_method>(method);
    req->coapId = d.id();
    req->tokenSize = d.tokenSize();
    memcpy(&req->tokenValue, d.token(), req->tokenSize);
    req->tokenValue = littleEndianToNative(req->tokenValue);
    req->state = CoapMessage::READING;
    req->isRequest = true;
    req->pos = const_cast<char*>(d.payload());
    req->end = req->pos + d.payloadSize();
    req->next = recvReqs_;
    recvReqs_ = req.release();
    curReqId_ = reqId;
    NAMED_SCOPE_GUARD(g, {
        if (recvReqs_ && recvReqs_->requestId == reqId) {
            auto next = recvReqs_->next;
            delete recvReqs_;
            recvReqs_ = next;
        }
        curReqId_ = 0;
    });
    // Invoke the request handler
    char uriStr[3] = { '/' };
    if (hasUri) {
        uriStr[1] = uri;
    }
    CHECK(h->callback(reinterpret_cast<coap_message*>(recvReqs_), uriStr, method, reqId, h->callbackArg));
    g.dismiss();
    return Result::HANDLED;
}

int CoapChannel::handleResponse(CoapMessageDecoder& d) {
    // Find the request for which this response is meant to
    size_t tokenSize = d.tokenSize();
    TokenValue tokenVal = 0;
    memcpy(&tokenVal, d.token(), tokenSize);
    tokenVal = littleEndianToNative(tokenVal);
    auto req = sentReqs_;
    for (; req; req = req->next) {
        if (req->tokenValue == tokenVal && req->tokenSize == tokenSize) {
            break;
        }
    }
    if (!req) {
        return 0;
    }
    removeFromList(req, sentReqs_);
    int reqErr = 0;
    SCOPE_GUARD({
        if (reqErr < 0 && req->errorCallback) {
            req->errorCallback(reqErr, req->requestId, req->callbackArg);
        }
        delete req;
    });
    // Create a response message
    if (curReqId_) {
        // TODO: Support asynchronous reading from multiple message instances
        return (reqErr = SYSTEM_ERROR_INTERNAL);
    }
    std::unique_ptr<CoapMessage> resp(new(std::nothrow) CoapMessage());
    if (!resp) {
        return (reqErr = SYSTEM_ERROR_NO_MEMORY);
    }
    resp->requestId = req->requestId;
    resp->sessionId = sessId_;
    resp->coapId = d.id();
    resp->tokenValue = tokenVal;
    resp->tokenSize = tokenSize;
    req->state = CoapMessage::READING;
    req->pos = const_cast<char*>(d.payload());
    req->end = req->pos + d.payloadSize();
    curReqId_ = resp->requestId;
    NAMED_SCOPE_GUARD(g, {
        curReqId_ = 0;
    });
    // Invoke the response handler
    CHECK(req->responseCallback(reinterpret_cast<coap_message*>(resp.release()), d.code(), resp->requestId, req->callbackArg));
    g.dismiss();
    return Result::HANDLED;
}

void CoapChannel::cancelMessages(CoapMessage* msgList, int error) {
    auto msg = msgList;
    while (msg) {
        auto next = msg->next;
        if (msg->errorCallback) {
            msg->errorCallback(error, msg->requestId, msg->callbackArg);
        }
        delete msg;
        msg = next;
    }
}

int CoapChannel::initProtocolMessage(CoapMessage* msg) {
    CHECK_PROTOCOL(proto_->get_channel().create(msgBuf_));
    CoapMessageEncoder e((char*)msgBuf_.buf(), msgBuf_.capacity());
    // Encode header and token
    e.type(CoapType::CON);
    e.id(0); // Assigned by the message channel
    if (msg->isRequest) {
        e.code(to_underlying(msg->method));
        msg->tokenValue = proto_->get_next_token();
        msg->tokenSize = sizeof(token_t); // TODO: Support longer tokens
    } else {
        e.code(msg->responseCode);
    }
    auto token = nativeToLittleEndian(msg->tokenValue);
    e.token((const char*)&token, msg->tokenSize);
    // Encode options
    if (msg->isRequest) {
        // TODO: Support longer URIs and additional options
        e.option(CoapOption::URI_PATH, &msg->uri, 1);
    }
    if (e.maxPayloadSize() < COAP_BLOCK_SIZE) {
        LOG(ERROR, "No enough space in CoAP message buffer");
        return SYSTEM_ERROR_TOO_LARGE;
    }
    size_t n = CHECK(e.encode());
    msg->pos = (char*)msgBuf_.buf() + n;
    msg->end = msg->pos + COAP_BLOCK_SIZE + 1; // Add 1 byte for a payload marker
    return 0;
}

int CoapChannel::handleProtocolError(ProtocolError error) {
    if (error == ProtocolError::NO_ERROR) {
        return 0;
    }
    LOG(ERROR, "Protocol error: %d", (int)error);
    int err = toSystemError(error);
    close(err);
    error = proto_->get_channel().command(Channel::CLOSE);
    if (error != ProtocolError::NO_ERROR) {
        LOG(ERROR, "Channel CLOSE command failed: %d", (int)error);
    }
    return err;
}

} // namespace particle::protocol::experimental

using namespace particle::protocol::experimental;

int coap_add_request_handler(const char* uri, int method, coap_request_callback cb, void* arg, void* reserved) {
    if (!isValidCoapMethod(method)) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    return CoapChannel::instance()->addRequestHandler(uri, static_cast<coap_method>(method), cb, arg);
}

void coap_remove_request_handler(const char* uri, int method, void* reserved) {
    if (!isValidCoapMethod(method)) {
        return;
    }
    CoapChannel::instance()->removeRequestHandler(uri, static_cast<coap_method>(method));
}

int coap_begin_request(coap_message** msg, const char* uri, int method, void* reserved) {
    if (!isValidCoapMethod(method)) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    CoapMessage* m = nullptr;
    auto reqId = CHECK(CoapChannel::instance()->beginRequest(uri, static_cast<coap_method>(method), m));
    *msg = reinterpret_cast<coap_message*>(m);
    return reqId;
}

int coap_end_request(coap_message* msg, coap_response_callback resp_cb, coap_ack_callback ack_cb,
        coap_error_callback error_cb, void* arg, void* reserved) {
    auto m = reinterpret_cast<CoapMessage*>(msg);
    CHECK(CoapChannel::instance()->endRequest(m, resp_cb, ack_cb, error_cb, arg));
    return 0;
}

int coap_begin_response(coap_message** msg, int code, int req_id, void* reserved) {
    return 0;
}

int coap_end_response(coap_message* msg, coap_ack_callback ack_cb, coap_error_callback error_cb,
        void* arg, void* reserved) {
    return 0;
}

void coap_destroy_message(coap_message* msg, void* reserved) {
}

int coap_write_payload(coap_message* msg, const char* data, size_t* size, coap_block_callback block_cb,
        coap_error_callback error_cb, void* arg, void* reserved) {
    auto m = reinterpret_cast<CoapMessage*>(msg);
    CHECK(CoapChannel::instance()->writePayload(m, data, *size, block_cb, error_cb, arg));
    return 0;
}

int coap_read_payload(coap_message* msg, char* data, size_t* size, coap_block_callback block_cb,
        coap_error_callback error_cb, void* arg, void* reserved) {
    auto m = reinterpret_cast<CoapMessage*>(msg);
    CHECK(CoapChannel::instance()->readPayload(m, data, *size, block_cb, error_cb, arg));
    return 0;
}

int coap_peek_payload(coap_message* msg, char* data, size_t size, void* reserved) {
    return 0;
}

int coap_get_option(coap_option** opt, int num, coap_message* msg, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED; // TODO
}

int coap_get_next_option(coap_option** opt, int* num, coap_message* msg, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED; // TODO
}

int coap_get_uint_option_value(const coap_option* opt, unsigned* val, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED; // TODO
}

int coap_get_uint64_option_value(const coap_option* opt, uint64_t* val, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED; // TODO
}

int coap_get_string_option_value(const coap_option* opt, char* data, size_t size, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED; // TODO
}

int coap_get_opaque_option_value(const coap_option* opt, char* data, size_t size, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED; // TODO
}

int coap_add_empty_option(coap_message* msg, int num, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED; // TODO
}

int coap_add_uint_option(coap_message* msg, int num, unsigned val, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED; // TODO
}

int coap_add_uint64_option(coap_message* msg, int num, uint64_t val, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED; // TODO
}

int coap_add_string_option(coap_message* msg, int num, const char* val, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED; // TODO
}

int coap_add_opaque_option(coap_message* msg, int num, const char* data, size_t size, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED; // TODO
}
