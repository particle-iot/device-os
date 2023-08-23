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

template<typename T>
inline void destroyList(T* head) {
    while (head) {
        auto next = head->next;
        delete head;
        head = next;
    }
}

} // namespace

struct CoapChannel::CoapMessage {
    enum State {
        NEW, // Message created
        READING, // Reading payload data
        WRITING, // Writing payload data
        WAITING_ACK, // Waiting for an ACK
        WAITING_RESPONSE // Waiting for a response
    };

    coap_ack_callback ackCallback; // Callback to invoke when the message is acknowledged
    coap_error_callback errorCallback; // Callback to invoke when an error occurs
    void* callbackArg; // User argument to pass to the callbacks
    CoapMessage* next; // Next message in the list
    char* pos; // Current position in the message buffer
    char* end; // End of the message buffer
    TokenValue tokenValue; // Token value
    size_t tokenSize; // Token size
    int id; // Internal message ID
    int coapId; // CoAP message ID
    int sessionId; // Session ID
    State state; // Message state
    bool isRequest; // Whether this is a request or response message

    union {
        // Fields specific to a request message
        struct {
            coap_response_callback responseCallback; // Callback to invoke when a response for this request is received
            coap_method method; // CoAP method code
            char uri; // Request URI. TODO: Support longer URIs
        };
        // Fields specific to a response message
        struct {
            int requestId; // Internal ID of the request which this response is meant for
            int responseCode; // CoAP response code
        };
    };

    CoapMessage() {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
        std::memset(this, 0, sizeof(*this));
#pragma GCC diagnostic pop
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

struct CoapChannel::ConnectionHandler {
    coap_connection_callback callback;
    void* callbackArg;
    ConnectionHandler* next;
    bool openFailed;

    ConnectionHandler(coap_connection_callback callback, void* callbackArg) :
            callback(callback),
            callbackArg(callbackArg),
            next(nullptr),
            openFailed(false) {
    }
};

CoapChannel::CoapChannel() :
        connHandlers_(nullptr),
        reqHandlers_(nullptr),
        sentReqs_(nullptr),
        recvReqs_(nullptr),
        unackMsgs_(nullptr),
        protocol_(spark_protocol_instance()),
        lastMsgId_(0),
        curMsgId_(0),
        sessId_(0),
        open_(false) {
}

CoapChannel::~CoapChannel() {
    if (sentReqs_ || recvReqs_ || unackMsgs_) { // Messages in recvReqs_ are owned by the user code and will be leaked
        LOG(WARN, "Destroying channel while a CoAP exchange is in progress");
    }
    close();
    destroyList(connHandlers_);
    destroyList(reqHandlers_);
}

int CoapChannel::beginRequest(coap_message** msg, const char* uri, coap_method method) {
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
    auto msgId = ++lastMsgId_;
    req->id = msgId;
    req->sessionId = sessId_;
    req->uri = *uri;
    req->method = method;
    req->state = CoapMessage::WRITING;
    req->isRequest = true;
    *msg = reinterpret_cast<coap_message*>(req.release());
    return msgId;
}

int CoapChannel::endRequest(coap_message* msg, coap_response_callback respCallback, coap_ack_callback ackCallback,
        coap_error_callback errorCallback, void* callbackArg) {
    auto req = reinterpret_cast<CoapMessage*>(msg);
    if (!req->isRequest) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (req->state != CoapMessage::WRITING) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!open_ || req->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    if (!curMsgId_) {
        CHECK(initProtocolMessage(req));
    } else if (curMsgId_ != req->id) {
        // TODO: Support asynchronous writing to multiple message instances
        return SYSTEM_ERROR_INTERNAL;
    }
    msgBuf_.set_length(req->end - (char*)msgBuf_.buf());
    CHECK_PROTOCOL(protocol_->get_channel().send(msgBuf_));
    req->coapId = msgBuf_.get_id();
    // TODO: Handle the case when a separate response arrives before an ACK for the request
    req->next = unackMsgs_;
    unackMsgs_ = req;
    // Release the message buffer
    msgBuf_.clear();
    curMsgId_ = 0;
    return 0;
}

int CoapChannel::beginResponse(coap_message** msg, int code, int requestId) {
    if (!open_) {
        return SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    auto req = recvReqs_;
    for (; req; req = req->next) {
        if (req->id == requestId) {
            break;
        }
    }
    if (!req) {
        return SYSTEM_ERROR_COAP_REQUEST_NOT_FOUND;
    }
    std::unique_ptr<CoapMessage> resp(new(std::nothrow) CoapMessage());
    if (!resp) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    resp->id = ++lastMsgId_;
    resp->requestId = req->id;
    resp->sessionId = sessId_;
    resp->tokenValue = req->tokenValue;
    resp->tokenSize = req->tokenSize;
    resp->responseCode = code;
    resp->state = CoapMessage::WRITING;
    *msg = reinterpret_cast<coap_message*>(resp.release());
    return 0;
}

int CoapChannel::endResponse(coap_message* msg, coap_ack_callback ackCallback, coap_error_callback errorCallback,
        void* callbackArg) {
    auto resp = reinterpret_cast<CoapMessage*>(msg);
    if (resp->isRequest) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (resp->state != CoapMessage::WRITING) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!open_ || resp->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    if (!curMsgId_) {
        CHECK(initProtocolMessage(resp));
    } else if (curMsgId_ != resp->id) {
        // TODO: Support asynchronous writing to multiple message instances
        return SYSTEM_ERROR_INTERNAL;
    }
    msgBuf_.set_length(resp->end - (char*)msgBuf_.buf());
    CHECK_PROTOCOL(protocol_->get_channel().send(msgBuf_));
    resp->coapId = msgBuf_.get_id();
    resp->next = unackMsgs_;
    unackMsgs_ = resp;
    // Release the message buffer
    msgBuf_.clear();
    curMsgId_ = 0;
    return 0;
}

int CoapChannel::writePayload(coap_message* apiMsg, const char* data, size_t& size, coap_block_callback /* blockCallback */,
        coap_error_callback errorCallback, void* callbackArg) {
    auto msg = reinterpret_cast<CoapMessage*>(apiMsg);
    if (msg->state != CoapMessage::WRITING) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!open_ || msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    if (size > 0) {
        if (msg->pos + size > msg->end) {
            // TODO: Support blockwise transfer
            return SYSTEM_ERROR_TOO_LARGE;
        }
        if (!curMsgId_) {
            CHECK(initProtocolMessage(msg));
            *msg->pos++ = 0xff; // Payload marker
            curMsgId_ = msg->id;
        } else if (curMsgId_ != msg->id) {
            // TODO: Support asynchronous writing to multiple message instances
            return SYSTEM_ERROR_INTERNAL;
        }
        memcpy(msg->pos, data, size);
        msg->pos += size;
    }
    msg->errorCallback = errorCallback;
    msg->callbackArg = callbackArg;
    return 0;
}

int CoapChannel::readPayload(coap_message* apiMsg, char* data, size_t& size, coap_block_callback /* blockCallback */,
        coap_error_callback errorCallback, void* callbackArg) {
    auto msg = reinterpret_cast<CoapMessage*>(apiMsg);
    if (msg->state != CoapMessage::READING) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!open_ || msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    if (size > 0) {
        if (msg->pos == msg->end) {
            // TODO: Support blockwise transfer
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        if (curMsgId_ != msg->id) {
            // TODO: Support asynchronous reading from multiple message instances
            return SYSTEM_ERROR_INTERNAL;
        }
        auto n = std::min<size_t>(size, msg->end - msg->pos);
        if (data) {
            memcpy(data, msg->pos, n);
        }
        msg->pos += n;
        if (msg->pos == msg->end) {
            // Release the message buffer
            msgBuf_.clear();
            curMsgId_ = 0;
        }
        size = n;
    }
    msg->errorCallback = errorCallback;
    msg->callbackArg = callbackArg;
    return 0;
}

int CoapChannel::peekPayload(coap_message* apiMsg, char* data, size_t size) {
    auto msg = reinterpret_cast<CoapMessage*>(apiMsg);
    if (msg->state != CoapMessage::READING) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!open_ || msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    if (size == 0) {
        return 0;
    }
    if (curMsgId_ != msg->id) {
        // TODO: Support asynchronous reading from multiple message instances
        return SYSTEM_ERROR_INTERNAL;
    }
    if (msg->pos == msg->end) {
        return SYSTEM_ERROR_END_OF_STREAM;
    }
    auto n = std::min<size_t>(size, msg->end - msg->pos);
    if (data) {
        memcpy(data, msg->pos, n);
    }
    return n;
}

void CoapChannel::destroyMessage(coap_message* apiMsg) {
    if (!apiMsg) {
        return;
    }
    auto msg = reinterpret_cast<CoapMessage*>(apiMsg);
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
    if (curMsgId_ == msg->id) {
        // Release the message buffer
        msgBuf_.clear();
        curMsgId_ = 0;
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
            } else {
                reqHandlers_ = h->next;
            }
            delete h;
            break;
        }
        prev = h;
    }
}

int CoapChannel::addConnectionHandler(coap_connection_callback callback, void* callbackArg) {
    if (!callback) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    auto h = connHandlers_;
    for (; h; h = h->next) {
        if (h->callback == callback) {
            break;
        }
    }
    if (!h) {
        std::unique_ptr<ConnectionHandler> h(new(std::nothrow) ConnectionHandler(callback, callbackArg));
        if (!h) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        h->next = connHandlers_;
        connHandlers_ = h.release();
    }
    return 0;
}

void CoapChannel::removeConnectionHandler(coap_connection_callback callback) {
    ConnectionHandler* prev = nullptr;
    for (auto h = connHandlers_; h; h = h->next) {
        if (h->callback == callback) {
            if (prev) {
                prev->next = h->next;
            } else {
                connHandlers_ = h->next;
            }
            delete h;
            break;
        }
        prev = h;
    }
}

void CoapChannel::open() {
    if (open_) {
        return;
    }
    // Mark the channel as open before invoking any user callbacks
    open_ = true;
    for (auto h = connHandlers_; h; h = h->next) {
        int r = h->callback(0 /* error */, COAP_CONNECTION_OPEN, h->callbackArg);
        if (r < 0) {
            // Handler errors are not propagated to the protocol layer as they're likely to persist
            // during further connection attempts and would otherwise prevent the device from
            // connecting to the cloud for an indefinite amount of time
            LOG(ERROR, "Connection handler failed: %d", r);
            h->openFailed = true;
        }
    }
}

void CoapChannel::close(int error) {
    if (!open_) {
        return;
    }
    // Mark the channel as closed before invoking any user callbacks
    open_ = false;
    cancelMessages(sentReqs_, error, true /* destroy */);
    sentReqs_ = nullptr;
    cancelMessages(unackMsgs_, error, true);
    unackMsgs_ = nullptr;
    cancelMessages(recvReqs_, error, false); // These are owned by the user code
    recvReqs_ = nullptr;
    // Release the message buffer
    msgBuf_.clear();
    curMsgId_ = 0;
    // Generate a new session ID to invalidate the message instances owned by the user code
    ++sessId_;
    for (auto h = connHandlers_; h; h = h->next) {
        if (!h->openFailed) {
            int r = h->callback(error, COAP_CONNECTION_CLOSED, h->callbackArg);
            if (r < 0) {
                LOG(ERROR, "Connection handler failed: %d", r);
            }
        }
        h->openFailed = false; // Clear the failed state
    }
}

int CoapChannel::handleCon(const Message& msgBuf) {
    CoapMessageDecoder d;
    CHECK(d.decode((const char*)msgBuf.buf(), msgBuf.length()));
    if (d.type() != CoapType::CON) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    int r = 0;
    if (isCoapRequestCode(d.code())) {
        r = CHECK(handleRequest(d, msgBuf));
    } else {
        r = CHECK(handleResponse(d, msgBuf));
    }
    return r; // 0 or Result::HANDLED
}

int CoapChannel::handleAck(const Message& msgBuf) {
    CoapMessageDecoder d;
    CHECK(d.decode((const char*)msgBuf.buf(), msgBuf.length()));
    auto msg = unackMsgs_;
    for (; msg; msg = msg->next) {
        if (msg->coapId == d.id()) {
            break;
        }
    }
    if (!msg) {
        return 0;
    }
    bool isReq = msg->isRequest;
    auto reqId = isReq ? msg->id : msg->requestId;
    auto callback = msg->ackCallback;
    auto arg = msg->callbackArg;
    removeFromList(msg, unackMsgs_);
    if (isReq) {
        msg->state = CoapMessage::WAITING_RESPONSE;
        msg->next = sentReqs_;
        sentReqs_ = msg;
    } else {
        delete msg;
    }
    if (callback) {
        CHECK(callback(reqId, arg));
    }
    if (isReq && d.code() != CoapCode::EMPTY) {
        CHECK(handleResponse(d, msgBuf)); // Handle the piggybacked response
    }
    return Result::HANDLED;
}

int CoapChannel::handleRst(const Message& msgBuf) {
    CoapMessageDecoder d;
    CHECK(d.decode((const char*)msgBuf.buf(), msgBuf.length()));
    auto msg = unackMsgs_;
    for (; msg; msg = msg->next) {
        if (msg->coapId == d.id()) {
            break;
        }
    }
    if (!msg) {
        return 0;
    }
    auto reqId = msg->isRequest ? msg->id : msg->requestId;
    auto callback = msg->errorCallback;
    auto arg = msg->callbackArg;
    removeFromList(msg, unackMsgs_);
    delete msg;
    if (callback) {
        callback(SYSTEM_ERROR_COAP_MESSAGE_RESET, reqId, arg);
    }
    return Result::HANDLED;
}

CoapChannel* CoapChannel::instance() {
    static CoapChannel channel;
    return &channel;
}

int CoapChannel::handleRequest(CoapMessageDecoder& d, const Message& msgBuf) {
    size_t tokenSize = d.tokenSize();
    if (!tokenSize || tokenSize > MAX_TOKEN_SIZE) { // TODO: Support empty tokens
        return 0;
    }
    // Get the request URI
    char uri = '/'; // TODO: Support longer URIs
    bool hasUri = false;
    auto it = d.findOption(CoapOption::URI_PATH);
    while (it) {
        if (it.size() > 1) {
            return 0; // URI is too long
        }
        if (it.size() > 0) {
            if (hasUri) {
                return 0; // URI is too long
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
    if (curMsgId_) {
        // TODO: Support asynchronous reading from multiple message instances
        return SYSTEM_ERROR_INTERNAL;
    }
    auto msgId = ++lastMsgId_;
    msgBuf_ = msgBuf; // Makes a shallow copy
    curMsgId_ = msgId;
    NAMED_SCOPE_GUARD(msgBufGuard, {
        msgBuf_.clear();
        curMsgId_ = 0;
    });
    if (d.type() == CoapType::CON) {
        // Acknowledge the request
        CHECK(sendEmptyAck(d.id()));
    }
    // Create a request message
    std::unique_ptr<CoapMessage> req(new(std::nothrow) CoapMessage());
    if (!req) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    req->id = msgId;
    req->sessionId = sessId_;
    req->uri = uri;
    req->method = static_cast<coap_method>(method);
    req->coapId = d.id();
    req->tokenSize = tokenSize;
    memcpy(&req->tokenValue, d.token(), tokenSize);
    req->state = CoapMessage::READING;
    req->isRequest = true;
    req->pos = const_cast<char*>(d.payload());
    req->end = req->pos + d.payloadSize();
    req->next = recvReqs_;
    recvReqs_ = req.release();
    NAMED_SCOPE_GUARD(recvReqsGuard, {
        if (recvReqs_ && recvReqs_->id == msgId) {
            auto next = recvReqs_->next;
            delete recvReqs_;
            recvReqs_ = next;
        }
    });
    // Invoke the request handler
    char uriStr[3] = { '/' };
    if (hasUri) {
        uriStr[1] = uri;
    }
    CHECK(h->callback(reinterpret_cast<coap_message*>(recvReqs_), uriStr, method, msgId, h->callbackArg));
    recvReqsGuard.dismiss();
    msgBufGuard.dismiss();
    return Result::HANDLED;
}

int CoapChannel::handleResponse(CoapMessageDecoder& d, const Message& msgBuf) {
    // Find the request which this response is meant for
    size_t tokenSize = d.tokenSize();
    if (!tokenSize || tokenSize > MAX_TOKEN_SIZE) { // TODO: Support empty tokens
        return 0;
    }
    TokenValue tokenVal = 0;
    memcpy(&tokenVal, d.token(), tokenSize);
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
            req->errorCallback(reqErr, req->id, req->callbackArg);
        }
        delete req;
    });
    if (curMsgId_) {
        // TODO: Support asynchronous reading from multiple message instances
        return (reqErr = SYSTEM_ERROR_INTERNAL);
    }
    auto msgId = ++lastMsgId_;
    msgBuf_ = msgBuf; // Makes a shallow copy
    curMsgId_ = msgId;
    NAMED_SCOPE_GUARD(msgBufGuard, {
        msgBuf_.clear();
        curMsgId_ = 0;
    });
    if (d.type() == CoapType::CON) {
        // Acknowledge the response
        CHECK(sendEmptyAck(d.id()));
    }
    // Create a response message
    std::unique_ptr<CoapMessage> resp(new(std::nothrow) CoapMessage());
    if (!resp) {
        return (reqErr = SYSTEM_ERROR_NO_MEMORY);
    }
    resp->id = msgId;
    resp->requestId = req->id;
    resp->sessionId = sessId_;
    resp->coapId = d.id();
    resp->tokenValue = tokenVal;
    resp->tokenSize = tokenSize;
    resp->state = CoapMessage::READING;
    resp->pos = const_cast<char*>(d.payload());
    resp->end = resp->pos + d.payloadSize();
    // Invoke the response handler
    if (req->responseCallback) {
        CHECK(req->responseCallback(reinterpret_cast<coap_message*>(resp.release()), d.code(), req->id, req->callbackArg));
    }
    msgBufGuard.dismiss();
    return Result::HANDLED;
}

int CoapChannel::sendEmptyAck(int coapId) {
    Message msg;
    CHECK_PROTOCOL(protocol_->get_channel().response(msgBuf_, msg, msgBuf_.capacity() - msgBuf_.length()));
    CoapMessageEncoder e((char*)msg.buf(), msg.capacity());
    e.type(CoapType::ACK);
    e.code(CoapCode::EMPTY);
    e.id(0); // Assigned by the message channel
    size_t n = CHECK(e.encode());
    if (n > msg.capacity()) {
        LOG(ERROR, "No enough space in CoAP message buffer");
        return SYSTEM_ERROR_TOO_LARGE;
    }
    msg.set_length(n);
    msg.set_id(coapId);
    CHECK_PROTOCOL(protocol_->get_channel().send(msg));
    return 0;
}

void CoapChannel::cancelMessages(CoapMessage* msgList, int error, bool destroy) {
    auto msg = msgList;
    while (msg) {
        auto next = msg->next;
        if (msg->errorCallback) {
            msg->errorCallback(error, msg->isRequest ? msg->id : msg->requestId, msg->callbackArg);
        }
        if (destroy) {
            delete msg;
        }
        msg = next;
    }
}

int CoapChannel::initProtocolMessage(CoapMessage* msg) {
    CHECK_PROTOCOL(protocol_->get_channel().create(msgBuf_));
    CoapMessageEncoder e((char*)msgBuf_.buf(), msgBuf_.capacity());
    // Encode header and token
    e.type(CoapType::CON);
    e.id(0); // Assigned by the message channel
    if (msg->isRequest) {
        e.code(to_underlying(msg->method));
        token_t token = protocol_->get_next_token();
        memcpy(&msg->tokenValue, &token, sizeof(token_t));
        msg->tokenSize = sizeof(token_t); // TODO: Support longer tokens
    } else {
        e.code(msg->responseCode);
    }
    e.token((const char*)&msg->tokenValue, msg->tokenSize);
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
    error = protocol_->get_channel().command(Channel::CLOSE);
    if (error != ProtocolError::NO_ERROR) {
        LOG(ERROR, "Channel CLOSE command failed: %d", (int)error);
    }
    return err;
}

} // namespace particle::protocol::experimental

using namespace particle::protocol::experimental;

int coap_add_connection_handler(coap_connection_callback cb, void* arg, void* reserved) {
    CHECK(CoapChannel::instance()->addConnectionHandler(cb, arg));
    return 0;
}

void coap_remove_connection_handler(coap_connection_callback cb, void* reserved) {
    CoapChannel::instance()->removeConnectionHandler(cb);
}

int coap_add_request_handler(const char* uri, int method, coap_request_callback cb, void* arg, void* reserved) {
    if (!isValidCoapMethod(method)) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    CHECK(CoapChannel::instance()->addRequestHandler(uri, static_cast<coap_method>(method), cb, arg));
    return 0;
}

void coap_remove_request_handler(const char* uri, int method, void* reserved) {
    if (!isValidCoapMethod(method)) {
        return;
    }
    CoapChannel::instance()->removeRequestHandler(uri, static_cast<coap_method>(method));
}

int coap_begin_request(coap_message** msg, const char* uri, int method, int timeout, int flags, void* reserved) {
    if (!isValidCoapMethod(method)) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (timeout != 0) {
        return SYSTEM_ERROR_NOT_SUPPORTED; // TODO
    }
    auto reqId = CHECK(CoapChannel::instance()->beginRequest(msg, uri, static_cast<coap_method>(method)));
    return reqId;
}

int coap_end_request(coap_message* msg, coap_response_callback resp_cb, coap_ack_callback ack_cb,
        coap_error_callback error_cb, void* arg, void* reserved) {
    CHECK(CoapChannel::instance()->endRequest(msg, resp_cb, ack_cb, error_cb, arg));
    return 0;
}

int coap_begin_response(coap_message** msg, int code, int req_id, int flags, void* reserved) {
    CHECK(CoapChannel::instance()->beginResponse(msg, code, req_id));
    return 0;
}

int coap_end_response(coap_message* msg, coap_ack_callback ack_cb, coap_error_callback error_cb,
        void* arg, void* reserved) {
    CHECK(CoapChannel::instance()->endResponse(msg, ack_cb, error_cb, arg));
    return 0;
}

void coap_destroy_message(coap_message* msg, void* reserved) {
    CoapChannel::instance()->destroyMessage(msg);
}

int coap_write_payload(coap_message* msg, const char* data, size_t* size, coap_block_callback block_cb,
        coap_error_callback error_cb, void* arg, void* reserved) {
    int r = CHECK(CoapChannel::instance()->writePayload(msg, data, *size, block_cb, error_cb, arg));
    return r; // 0 or COAP_RESULT_WAIT_BLOCK
}

int coap_read_payload(coap_message* msg, char* data, size_t* size, coap_block_callback block_cb,
        coap_error_callback error_cb, void* arg, void* reserved) {
    int r = CHECK(CoapChannel::instance()->readPayload(msg, data, *size, block_cb, error_cb, arg));
    return r; // 0 or COAP_RESULT_WAIT_BLOCK
}

int coap_peek_payload(coap_message* msg, char* data, size_t size, void* reserved) {
    size_t n = CHECK(CoapChannel::instance()->peekPayload(msg, data, size));
    return n;
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
