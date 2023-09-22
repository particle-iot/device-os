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

#if !defined(DEBUG_BUILD) && !defined(UNIT_TEST)
#define NDEBUG // TODO: Define NDEBUG in release builds
#endif

#include "logging.h"

LOG_SOURCE_CATEGORY("system.coap")

#include <algorithm>
#include <optional>
#include <cstring>
#include <cstdint>

#include "coap_channel_new.h"

#include "coap_message_encoder.h"
#include "coap_message_decoder.h"
#include "protocol.h"
#include "spark_protocol_functions.h"

#include "ref_count.h"
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

const size_t MAX_TOKEN_SIZE = sizeof(token_t); // TODO: Support longer tokens
const size_t MAX_ETAG_SIZE = 8; // RFC 7252, 5.10

const unsigned BLOCK_SZX = 6; // Value of the SZX field for 1024-byte blocks (RFC 7959, 2.2)
static_assert(COAP_BLOCK_SIZE == 1024); // When changing the block size, make sure to update BLOCK_SZX accordingly

const unsigned CONTINUE_CODE = COAP_CODE(2, 31); // 2.31 Continue

const unsigned DEFAULT_REQUEST_TIMEOUT = 60000;

static_assert(COAP_INVALID_REQUEST_ID == 0); // Used by value in the code

unsigned encodeBlockOption(unsigned num, bool m) {
    unsigned opt = (num << 4) | BLOCK_SZX;
    if (m) {
        opt |= 0x08;
    }
    return opt;
}

int decodeBlockOption(unsigned opt, unsigned& num, bool& m) {
    unsigned szx = opt & 0x07;
    if (szx != BLOCK_SZX) {
        // Server is required to use exactly the same block size
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    num = opt >> 4;
    m = opt & 0x08;
    return 0;
}

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

// TODO: Use a generic intrusive list container
template<typename T>
inline void prependToList(T*& head, T* elem) {
    assert(!elem->next && !elem->prev); // Debug-only
    elem->prev = nullptr;
    elem->next = head;
    if (head) {
        head->prev = elem;
    }
    head = elem;
}

template<typename T>
inline void removeFromList(T*& head, T* elem) {
    if (elem->prev) {
        assert(head != elem);
        elem->prev->next = elem->next;
    } else {
        assert(head == elem);
        head = elem->next;
    }
    if (elem->next) {
        elem->next->prev = elem->prev;
    }
#ifndef NDEBUG
    elem->next = nullptr;
    elem->prev = nullptr;
#endif
}

template<typename T, typename F>
inline T* findInList(T* head, const F& fn) {
    while (head) {
        if (fn(head)) {
            return head;
        }
        head = head->next;
    }
    return nullptr;
}

} // namespace

struct CoapChannel::CoapMessage: RefCount {
    coap_block_callback blockCallback; // Callback to invoke when a message block is sent or received
    coap_ack_callback ackCallback; // Callback to invoke when the message is acknowledged
    coap_error_callback errorCallback; // Callback to invoke when an error occurs
    void* callbackArg; // User argument to pass to the callbacks

    int id; // Internal message ID
    int requestId; // Internal ID of the request that started this message exchange
    int sessionId; // ID of the session for which this message was created

    char etag[MAX_ETAG_SIZE]; // ETag option
    size_t etagSize; // Size of the ETag option
    int coapId; // CoAP message ID
    token_t token; // CoAP token. TODO: Support longer tokens

    std::optional<int> blockIndex; // Index of the current message block
    std::optional<bool> hasMore; // Whether more blocks are expected for this message

    char* pos; // Current position in the message buffer
    char* end; // End of the message buffer
    size_t prefixSize; // Size of the CoAP framing

    MessageType type; // Message type
    MessageState state; // Message state

    CoapMessage* next; // Next message in the list
    CoapMessage* prev; // Previous message in the list

    explicit CoapMessage(MessageType type) :
            blockCallback(nullptr),
            ackCallback(nullptr),
            errorCallback(nullptr),
            callbackArg(nullptr),
            id(0),
            requestId(0),
            sessionId(0),
            etagSize(0),
            coapId(0),
            token(0),
            pos(nullptr),
            end(nullptr),
            prefixSize(0),
            type(type),
            state(MessageState::NEW),
            next(nullptr),
            prev(nullptr) {
    }
};

// TODO: Use separate message classes for different transfer directions
struct CoapChannel::RequestMessage: CoapMessage {
    coap_response_callback responseCallback; // Callback to invoke when a response for this request is received

    ResponseMessage* blockResponse; // Response for which this block request is retrieving data

    system_tick_t timeSent; // Time the request was sent
    unsigned timeout; // Request timeout

    coap_method method; // CoAP method code
    char uri; // Request URI. TODO: Support longer URIs

    explicit RequestMessage(bool blockRequest = false) :
            CoapMessage(blockRequest ? MessageType::BLOCK_REQUEST : MessageType::REQUEST),
            responseCallback(nullptr),
            blockResponse(nullptr),
            timeSent(0),
            timeout(0),
            method(),
            uri(0) {
    }
};

struct CoapChannel::ResponseMessage: CoapMessage {
    RefCountPtr<RequestMessage> blockRequest; // Request message used to get the last received block of this message

    int status; // CoAP response code

    ResponseMessage() :
            CoapMessage(MessageType::RESPONSE),
            status(0) {
    }
};

struct CoapChannel::RequestHandler {
    coap_request_callback callback; // Callback to invoke when a request is received
    void* callbackArg; // User argument to pass to the callback

    coap_method method; // CoAP method code
    char uri; // Request URI. TODO: Support longer URIs

    RequestHandler* next; // Next handler in the list
    RequestHandler* prev; // Previous handler in the list

    RequestHandler(char uri, coap_method method, coap_request_callback callback, void* callbackArg) :
            callback(callback),
            callbackArg(callbackArg),
            method(method),
            uri(uri),
            next(nullptr),
            prev(nullptr) {
    }
};

struct CoapChannel::ConnectionHandler {
    coap_connection_callback callback; // Callback to invoke when the connection status changes
    void* callbackArg; // User argument to pass to the callback

    bool openFailed; // If true, the user callback returned an error when the connection was opened

    ConnectionHandler* next; // Next handler in the list
    ConnectionHandler* prev; // Previous handler in the list

    ConnectionHandler(coap_connection_callback callback, void* callbackArg) :
            callback(callback),
            callbackArg(callbackArg),
            openFailed(false),
            next(nullptr),
            prev(nullptr) {
    }
};

CoapChannel::CoapChannel() :
        connHandlers_(nullptr),
        reqHandlers_(nullptr),
        sentReqs_(nullptr),
        recvReqs_(nullptr),
        blockMsgs_(nullptr),
        unackMsgs_(nullptr),
        protocol_(spark_protocol_instance()),
        state_(State::CLOSED),
        lastMsgId_(0),
        curMsgId_(0),
        sessId_(0),
        pendingCloseError_(0),
        openPending_(false) {
}

CoapChannel::~CoapChannel() {
    close();
    // Destroy connection handlers
    auto h = connHandlers_;
    while (h) {
        auto next = h->next;
        delete h;
        h = next;
    }
    // Destroy request handlers
    auto h2 = reqHandlers_;
    while (h2) {
        auto next = h2->next;
        delete h2;
        h2 = next;
    }
}

int CoapChannel::beginRequest(coap_message** msg, const char* uri, coap_method method, int timeout) {
    if (timeout < 0 || std::strlen(uri)) { // TODO: Support longer URIs
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (state_ != State::OPEN) {
        return SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    auto req = makeRefCountPtr<RequestMessage>();
    if (!req) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    auto msgId = ++lastMsgId_;
    req->id = msgId;
    req->requestId = msgId;
    req->sessionId = sessId_;
    req->uri = *uri;
    req->method = method;
    req->timeout = (timeout > 0) ? timeout : DEFAULT_REQUEST_TIMEOUT;
    req->state = MessageState::WRITE;
    *msg = reinterpret_cast<coap_message*>(req.unwrap());
    return msgId;
}

int CoapChannel::endRequest(coap_message* apiMsg, coap_response_callback respCallback, coap_ack_callback ackCallback,
        coap_error_callback errorCallback, void* callbackArg) {
    auto msg = reinterpret_cast<CoapMessage*>(apiMsg);
    if (msg->type != MessageType::REQUEST) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_REQUEST_CANCELLED;
    }
    if (state_ != State::OPEN) {
        return SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    auto req = static_cast<RequestMessage*>(msg);
    if (req->state != MessageState::WRITE || req->hasMore.value_or(false)) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!curMsgId_) {
        CHECK(prepareMessage(req));
    } else if (curMsgId_ != req->id) {
        // TODO: Support asynchronous writing to multiple message instances
        LOG(ERROR, "CoAP message buffer is already in use");
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    CHECK(sendMessage(req));
    req->responseCallback = respCallback;
    req->ackCallback = ackCallback;
    req->errorCallback = errorCallback;
    req->callbackArg = callbackArg;
    return 0;
}

int CoapChannel::beginResponse(coap_message** msg, int status, int requestId) {
    if (state_ != State::OPEN) {
        return SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    auto req = findInList(recvReqs_, [](auto req) {
        return req->id == requestId;
    });
    if (!req) {
        return SYSTEM_ERROR_COAP_REQUEST_NOT_FOUND;
    }
    auto resp = makeRefCountPtr<ResponseMessage>();
    if (!resp) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    resp->id = ++lastMsgId_;
    resp->requestId = req->id;
    resp->sessionId = sessId_;
    resp->token = req->token;
    resp->status = status;
    resp->state = MessageState::WRITE;
    *msg = reinterpret_cast<coap_message*>(resp.unwrap());
    return 0;
}

int CoapChannel::endResponse(coap_message* apiMsg, coap_ack_callback ackCallback, coap_error_callback errorCallback,
        void* callbackArg) {
    auto msg = reinterpret_cast<CoapMessage*>(msg);
    if (msg->type != MessageType::RESPONSE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_REQUEST_CANCELLED;
    }
    if (state_ != State::OPEN) {
        return SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    auto resp = static_cast<ResponseMessage*>(msg);
    if (resp->state != MessageState::WRITE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!curMsgId_) {
        CHECK(prepareMessage(resp));
    } else if (curMsgId_ != resp->id) {
        // TODO: Support asynchronous writing to multiple message instances
        LOG(ERROR, "CoAP message buffer is already in use");
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    CHECK(sendMessage(resp));
    resp->ackCallback = ackCallback;
    resp->errorCallback = errorCallback;
    resp->callbackArg = callbackArg;
    return 0;
}

int CoapChannel::writePayload(coap_message* apiMsg, const char* data, size_t& size, coap_block_callback blockCallback,
        coap_error_callback errorCallback, void* callbackArg) {
    if (msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_REQUEST_CANCELLED;
    }
    if (state_ != State::OPEN) {
        return SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    auto msg = reinterpret_cast<CoapMessage*>(apiMsg);
    if (msg->state != MessageState::WRITE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    bool sendBlock = false;
    if (size > 0) {
        if (!curMsgId_) {
            if (msg->blockIndex.has_value()) {
                // Writing another message block
                ++msg->blockIndex.value();
                msg->hasMore = false;
            }
            CHECK(prepareMessage(msg));
            *msg->pos++ = 0xff; // Payload marker
        } else if (curMsgId_ != msg->id) {
            // TODO: Support asynchronous writing to multiple message instances
            LOG(ERROR, "CoAP message buffer is already in use");
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
        auto bytesToWrite = size;
        if (msg->pos + bytesToWrite > msg->end) {
            if (msg->type != MessageType::REQUEST || !blockCallback) { // TODO: Support blockwise device-to-cloud responses
                return SYSTEM_ERROR_TOO_LARGE;
            }
            auto req = static_cast<RequestMessage*>(msg);
            // The spec doesn't define a way to send multiple concurrent blockwise requests to the
            // same URI so we need to check if there's an ongoing request to this URI already
            bool found = findInList(sentReqs_, [=](auto req2) {
                return req2->type == MessageType::REQUEST && req2->uri == req->uri && req2->blockIndex.has_value(); // Ignore BLOCK_REQUESTs
            });
            if (!found) {
                // Look among the requests that are being written by the user code
                found = findInList(blockMsgs_, [=](auto msg) {
                    if (msg->type != MessageType::REQUEST) {
                        return false;
                    }
                    auto req2 = static_cast<RequestMessage*>(msg);
                    return req2->uri == req->uri;
                });
            }
            if (found) {
                LOG(ERROR, "Another blockwise transfer is in progress");
                return SYSTEM_ERROR_NOT_SUPPORTED; // TODO: Put the new request in a queue
            }
            bytesToWrite = msg->end - msg->pos;
            sendBlock = true;
        }
        memcpy(msg->pos, data, bytesToWrite);
        msg->pos += bytesToWrite;
        if (sendBlock) {
            if (!msg->blockIndex.has_value()) {
                msg->blockIndex = 0;
            }
            msg->hasMore = true; // bytesToWrite < size
            CHECK(updateMessage(msg)); // Update or add a block option to the message
            CHECK(sendMessage(msg));
        }
        size = bytesToWrite;
    }
    msg->blockCallback = blockCallback;
    msg->errorCallback = errorCallback;
    msg->callbackArg = callbackArg;
    return sendBlock ? COAP_RESULT_WAIT_BLOCK : 0;
}

int CoapChannel::readPayload(coap_message* apiMsg, char* data, size_t& size, coap_block_callback blockCallback,
        coap_error_callback errorCallback, void* callbackArg) {
    if (msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_REQUEST_CANCELLED;
    }
    if (state_ != State::OPEN) {
        return SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    auto msg = reinterpret_cast<CoapMessage*>(apiMsg);
    if (msg->state != MessageState::READ) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    bool getBlock = false;
    if (size > 0) {
        if (msg->pos == msg->end) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        if (curMsgId_ != msg->id) {
            // TODO: Support asynchronous reading from multiple message instances
            LOG(ERROR, "CoAP message buffer is already in use");
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
        auto bytesToRead = std::min<size_t>(bytesToRead, msg->end - msg->pos);
        if (data) {
            memcpy(data, msg->pos, bytesToRead);
        }
        msg->pos += bytesToRead;
        if (msg->pos == msg->end) {
            releaseMessageBuffer();
            if (msg->hasMore.value_or(false)) {
                if (blockCallback) {
                    assert(msg->type == MessageType::RESPONSE); // TODO: Support cloud-to-device blockwise requests
                    auto resp = static_cast<ResponseMessage*>(msg);
                    auto req = resp->blockRequest.get();
                    assert(req);
                    // Send a new request with the original options and updated block number
                    ++req->blockIndex.value();
                    CHECK(prepareMessage(req));
                    CHECK(sendMessage(req));
                    resp->state = MessageState::WAIT_BLOCK;
                    prependToList(blockMsgs_, resp);
                    getBlock = true;
                } else {
                    LOG(WARN, "Incomplete read of blockwise response");
                }
            }
        }
        size = bytesToRead;
    }
    msg->blockCallback = blockCallback;
    msg->errorCallback = errorCallback;
    msg->callbackArg = callbackArg;
    return getBlock ? COAP_RESULT_WAIT_BLOCK : 0;
}

int CoapChannel::peekPayload(coap_message* apiMsg, char* data, size_t size) {
    if (msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_REQUEST_CANCELLED;
    }
    if (state_ != State::OPEN) {
        return SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    auto msg = reinterpret_cast<CoapMessage*>(apiMsg);
    if (msg->state != MessageState::READ) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (size > 0) {
        if (msg->pos == msg->end) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        if (curMsgId_ != msg->id) {
            // TODO: Support asynchronous reading from multiple message instances
            LOG(ERROR, "CoAP message buffer is already in use");
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
        size = std::min<size_t>(size, msg->end - msg->pos);
        if (data) {
            memcpy(data, msg->pos, size);
        }
    }
    return size;
}

void CoapChannel::destroyMessage(coap_message* apiMsg) {
    if (!apiMsg) {
        return;
    }
    auto msg = reinterpret_cast<CoapMessage*>(apiMsg);
    if (msg->sessionId == sessId_) {
        switch (msg->state) {
        case MessageState::READ: {
            if (msg->type == MessageType::REQUEST) {
                removeFromList(recvReqs_, static_cast<RequestMessage*>(msg));
            }
            break;
        }
        case MessageState::WRITE: {
            if (msg->type == MessageType::REQUEST && msg->blockIndex.has_value()) {
                removeFromList(blockMsgs_, msg);
            }
            break;
        }
        case MessageState::WAIT_ACK: {
            removeFromList(unackMsgs_, msg);
            break;
        }
        case MessageState::WAIT_RESPONSE: {
            assert(msg->type == MessageType::REQUEST);
            removeFromList(sentReqs_, static_cast<RequestMessage*>(msg));
            break;
        }
        case MessageState::WAIT_BLOCK: {
            removeFromList(blockMsgs_, msg);
            if (msg->type == MessageType::RESPONSE) {
                // Cancel the ongoing block request for this response
                auto resp = static_cast<ResponseMessage*>(msg);
                auto req = resp->blockRequest.get();
                assert(req);
                switch (req->state) {
                case MessageState::WAIT_ACK: {
                    removeFromList(unackMsgs_, req);
                    break;
                }
                case MessageState::WAIT_RESPONSE: {
                    removeFromList(sentReqs_, req);
                    break;
                }
                default:
                    assert(false); // Unreachable
                    break;
                }
                req->state = MessageState::DONE;
                // Lifetime of the block request is managed by the response object
            }
            break;
        }
        default:
            break;
        }
    }
    if (curMsgId_ == msg->id) {
        releaseMessageBuffer();
    }
    msg->state = MessageState::DONE;
    msg->release();
}

int CoapChannel::addRequestHandler(const char* uri, coap_method method, coap_request_callback callback, void* callbackArg) {
    if (!callback) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (state_ != State::CLOSED) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (std::strlen(uri) != 1) {
        return SYSTEM_ERROR_NOT_SUPPORTED; // TODO: Support longer URIs
    }
    auto h = findInList(reqHandlers_, [=](auto h) {
        return h->uri == *uri && h->method;
    });
    if (h) {
        h->callback = callback;
        h->callbackArg = callbackArg;
    } else {
        std::unique_ptr<RequestHandler> h(new(std::nothrow) RequestHandler(*uri, method, callback, callbackArg));
        if (!h) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        prependToList(reqHandlers_, h.release());
    }
    return 0;
}

void CoapChannel::removeRequestHandler(const char* uri, coap_method method) {
    if (std::strlen(uri) != 1) { // TODO: Support longer URIs
        return;
    }
    if (state_ != State::CLOSED) {
        LOG(ERROR, "Cannot remove handler while channel is open");
        return;
    }
    auto h = findInList(reqHandlers_, [=](auto h) {
        return h->uri == *uri && h->method == method;
    });
    if (h) {
        removeFromList(reqHandlers_, h);
        delete h;
    }
}

int CoapChannel::addConnectionHandler(coap_connection_callback callback, void* callbackArg) {
    if (!callback) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (state_ != State::CLOSED) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    auto h = findInList(connHandlers_, [=](auto h) {
        return h->callback == callback;
    });
    if (!h) {
        std::unique_ptr<ConnectionHandler> h(new(std::nothrow) ConnectionHandler(callback, callbackArg));
        if (!h) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        prependToList(connHandlers_, h.release());
    }
    return 0;
}

void CoapChannel::removeConnectionHandler(coap_connection_callback callback) {
    if (state_ != State::CLOSED) {
        LOG(ERROR, "Cannot remove handler while channel is open");
        return;
    }
    auto h = findInList(connHandlers_, [=](auto h) {
        return h->callback == callback;
    });
    if (h) {
        removeFromList(connHandlers_, h);
        delete h;
    }
}

void CoapChannel::open() {
    pendingCloseError_ = 0;
    if (state_ != State::CLOSED) {
        if (state_ == State::CLOSING) {
            // open() is being called in a connection handler
            openPending_ = true;
        }
        return;
    }
    state_ = State::OPENING;
    for (auto h = connHandlers_; h; h = h->next) {
        assert(h->callback);
        int r = h->callback(0 /* error */, COAP_CONNECTION_OPEN, h->callbackArg);
        if (r < 0) {
            // Handler errors are not propagated to the protocol layer
            LOG(ERROR, "Connection handler failed: %d", r);
            h->openFailed = true;
        }
    }
    state_ = State::OPEN;
    if (pendingCloseError_) {
        // close() was called in a connection handler
        int error = pendingCloseError_;
        pendingCloseError_ = 0;
        close(error);
    }
}

void CoapChannel::close(int error) {
    if (!error) {
        error = SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    openPending_ = false;
    if (state_ != State::OPEN) {
        if (state_ == State::OPENING) {
            // close() is being called in a connection handler
            pendingCloseError_ = error;
        }
        return;
    }
    state_ = State::CLOSING;
    ++sessId_;

    // For safety, prevent any messages from being destroyed from within a user callback
    for (auto msg = sentReqs_; msg; msg = msg->next) {
        msg->addRef();
    }
    for (auto msg = recvReqs_; msg; msg = msg->next) {
        msg->addRef();
    }
    for (auto msg = blockMsgs_; msg; msg = msg->next) {
        msg->addRef();
    }
    for (auto msg = unackMsgs_; msg; msg = msg->next) {
        msg->addRef();
    }

    // Cancel device requests awaiting a response
    for (auto msg = sentReqs_; msg; msg = msg->next) {
        if (msg->type == MessageType::BLOCK_REQUEST) {
            if (req->state != MessageState::DONE && req->errorCallback) {
                req->errorCallback(error, req->id, req->callbackArg); // Callback passed to coap_write_payload() or coap_end_request()
            }
            if (!req->hasMore.value_or(false)) {
                // coap_end_request() has been called for this message so it's no longer owned by
                // the user code
                req->release();
            }
        } // else: Lifetime of a block request is managed by its parent response object
        auto next = req->next;
        req->release();
        req = next;
    }
    sentReqs_ = nullptr;

    // Cancel server requests awaiting a response. These are owned by the user code and there's no
    // need to invoke any callbacks for them
    recvReqs_ = nullptr;

    // Cancel device requests and responses awaiting an ACK
    for (auto msg = unackMsgs_; msg; msg = msg->next) {
        msg->addRef();
    }
    auto msg = unackMsgs_;
    while (msg) {
        if (msg->isRequest) {
            auto req = static_cast<RequestMessage*>(msg);
            if (!req->blockResponse) {
                // This is a user request
                if (req->state != MessageState::DONE && req->errorCallback) {
                    req->errorCallback(error, req->id, req->callbackArg); // Callback passed to coap_write_payload() or coap_end_request()
                }
                if (!req->hasMore.value_or(false)) {
                    // coap_end_request() has been called for this message so it's no longer owned by
                    // the user code
                    req->release();
                }
            } // else: Lifetime of a block request is managed by its parent response object
        } else {
            if (msg->errorCallback) {
                msg->errorCallback(error, msg->requestId, msg->callbackArg); // Callback passed to coap_end_response()
            }
            msg->release();
        }
        auto next = msg->next;
        msg->release();
        msg = next;
    }
    unackMsgs_ = nullptr;

    // Cancel device requests for which a blockwise response is being received

    forEachInList(blockMsgs_, [=](auto msg) {
        if (!msg->isRequest) {
            notifyExchangeError(msg, error); // Invokes the callback passed to coap_read_payload()
        }
    });
    blockMsgs_ = nullptr;

    releaseMessageBuffer();

    // Invoke connection handler callbacks
    forEachInList(connHandlers_, [=](auto h) {
        if (!h->openFailed) {
            assert(h->callback);
            int r = h->callback(error, COAP_CONNECTION_CLOSED, h->callbackArg);
            if (r < 0) {
                LOG(ERROR, "Connection handler failed: %d", r);
            }
        }
        h->openFailed = false; // Clear the failed state
    });

    state_ = State::CLOSED;
    if (openPending_) {
        // open() was called in a connection handler
        openPending_ = false;
        open();
    }
}

int CoapChannel::handleCon(const Message& msgBuf) {
    if (curMsgId_) {
        // TODO: Support asynchronous reading/writing to multiple message instances
        LOG(ERROR, "CoAP message buffer is already in use");
        // Contents of the buffer have already been overwritten at this point
        releaseMessageBuffer();
    }
    msgBuf_ = msgBuf; // Makes a shallow copy
    CoapMessageDecoder d;
    CHECK(d.decode((const char*)msgBuf_.buf(), msgBuf_.length()));
    if (d.type() != CoapType::CON) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    int r = 0;
    if (isCoapRequestCode(d.code())) {
        r = CHECK(handleRequest(d));
    } else {
        r = CHECK(handleResponse(d));
    }
    return r; // 0 or Result::HANDLED
}

int CoapChannel::handleAck(const Message& msgBuf) {
    if (curMsgId_) {
        // TODO: Support asynchronous reading/writing to multiple message instances
        LOG(ERROR, "CoAP message buffer is already in use");
        // Contents of the buffer have already been overwritten at this point
        releaseMessageBuffer();
    }
    msgBuf_ = msgBuf; // Makes a shallow copy
    CoapMessageDecoder d;
    CHECK(d.decode((const char*)msgBuf_.buf(), msgBuf_.length()));
    if (d.type() != CoapType::ACK) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    auto msg = findInList(unackMsgs_, [=](auto msg) {
        return msg->coapId == d.id();
    });
    if (!msg) {
        return 0;
    }
    bool isReq = msg->isRequest;
    auto reqId = msg->requestId;
    auto ackCb = msg->ackCallback;
    auto cbArg = msg->callbackArg;
    bool notifyAck = true;
    removeFromList(unackMsgs_, msg);
    if (isReq) {
        auto req = static_cast<RequestMessage*>(msg);
        if (!req->blockResponse && req->blockIndex.has_value()) {
            // For a blockwise request, the ACK callback is only invoked when the last message
            // block is acknowledged
            assert(req->hasMore.has_value());
            notifyAck = !req->hasMore.value();
        }
        msg->state = MessageState::WAIT_RESPONSE;
        prependToList(sentReqs_, msg);
    } else {
        delete msg;
    }
    if (ackCb && notifyAck) {
        CHECK(ackCb(reqId, cbArg));
    }
    if (isReq && d.code() != CoapCode::EMPTY) {
        CHECK(handleResponse(d, msgBuf)); // Handle the piggybacked response
    }
    return Result::HANDLED;
}

int CoapChannel::handleRst(const Message& msgBuf) {
    if (curMsgId_) {
        // TODO: Support asynchronous reading/writing to multiple message instances
        LOG(ERROR, "CoAP message buffer is already in use");
        // Contents of the buffer have already been overwritten at this point
        releaseMessageBuffer();
    }
    msgBuf_ = msgBuf; // Makes a shallow copy
    CoapMessageDecoder d;
    CHECK(d.decode((const char*)msgBuf_.buf(), msgBuf_.length()));
    if (d.type() != CoapType::RST) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    auto msg = findInList(unackMsgs_, [=](auto msg) {
        return msg->coapId == d.id();
    });
    if (!msg) {
        return 0;
    }
    removeFromList(unackMsgs_, msg);
    if (msg->isRequest) {
        auto req = static_cast<RequestMessage*>(msg);
        if (req->blockResponse) {
            notifyExchangeError(req->blockResponse, error); // Invokes the callback passed to coap_read_payload()
        } else {
            notifyExchangeError(req, error); // Invokes the callback passed to coap_write_payload() or coap_end_request()
            if (!req->hasMore.value_or(false)) {
                // coap_end_request() has been called for this message so it's no longer owned by
                // the user code
                delete msg;
            }
        }
    } else {
        notifyExchangeError(msg, error); // Invokes the callback passed to coap_end_response()
        delete msg;
    }
    return Result::HANDLED;
}

CoapChannel* CoapChannel::instance() {
    static CoapChannel channel;
    return &channel;
}

int CoapChannel::handleRequest(CoapMessageDecoder& d) {
    if (d.tokenSize() != sizeof(token_t)) { // TODO: Support empty tokens
        return 0;
    }
    // Get the request URI
    char uri = '/'; // TODO: Support longer URIs
    bool hasUri = false;
    // TODO: Add a helper function for reconstructing the URI string from CoAP options
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
    auto h = findInList(reqHandlers_, [=](auto h) {
        return h->uri == uri && h->method == method;
    });
    if (!h) {
        return 0;
    }
    // Acknowledge the request
    assert(d.type() == CoapType::CON); // TODO: Support non-confirmable requests
    CHECK(sendEmptyAck(d.id()));
    // Create a request message
    auto msgId = ++lastMsgId_;
    assert(!curMsgId_);
    curMsgId_ = msgId;
    NAMED_SCOPE_GUARD(releaseMsgBufGuard, {
        releaseMessageBuffer();
    });
    std::unique_ptr<CoapMessage> req(new(std::nothrow) RequestMessage());
    if (!req) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    req->id = msgId;
    req->requestId = msgId;
    req->sessionId = sessId_;
    req->uri = uri;
    req->method = static_cast<coap_method>(method);
    req->coapId = d.id();
    assert(d.tokenSize() == sizeof(req->token));
    memcpy(&req->token, d.tokenData(), d.tokenSize());
    req->state = MessageState::READ;
    req->pos = const_cast<char*>(d.payload());
    req->end = req->pos + d.payloadSize();
    prependToList(recvReqs_, req.get());
    auto reqPtr = req.release();
    NAMED_SCOPE_GUARD(removeRecvReqGuard, {
        if (recvReqs_ == reqPtr) {
            removeFromList(recvReqs_, reqPtr);
        }
    });
    // Invoke the request handler
    char uriStr[3] = { '/' };
    if (hasUri) {
        uriStr[1] = uri;
    }
    CHECK(h->callback(reinterpret_cast<coap_message*>(reqPtr), uriStr, method, msgId, h->callbackArg));
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
    removeFromList(sentReqs_, req);
    int reqErr = 0;
    SCOPE_GUARD({
        if (reqErr < 0 && req->errorCallback) {
            req->errorCallback(reqErr, req->id, req->callbackArg);
        }
        delete req;
    });
    if (curMsgId_) {
        // TODO: Support asynchronous reading from multiple message instances
        LOG(ERROR, "CoAP message buffer is already in use");
        return (reqErr = SYSTEM_ERROR_NOT_SUPPORTED);
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
    resp->state = MessageState::READING;
    resp->pos = const_cast<char*>(d.payload());
    resp->end = resp->pos + d.payloadSize();
    // Invoke the response handler
    if (req->responseCallback) {
        CHECK(req->responseCallback(reinterpret_cast<coap_message*>(resp.release()), d.code(), req->id, req->callbackArg));
    }
    msgBufGuard.dismiss();
    return Result::HANDLED;
}

int CoapChannel::prepareMessage(CoapMessage* msg) {
    assert(!curMsgId_);
    CHECK_PROTOCOL(protocol_->get_channel().create(msgBuf_));
    msg->token = protocol_->get_next_token();
    msg->prefixSize = 0;
    msg->pos = msgBuf_.buf();
    CHECK(updateMessage(msg));
    curMsgId_ = msg->id;
    return 0;
}

int CoapChannel::updateMessage(CoapMessage* msg) {
    assert(curMsgId_ == msg->id);
    char prefix[128];
    CoapMessageEncoder e(prefix, sizeof(prefix));
    e.type(CoapType::CON);
    e.id(0); // Will be set by the underlying message channel
    if (msg->isRequest) {
        auto req = static_cast<RequestMessage*>(msg);
        e.code((int)req->method);
    } else {
        auto resp = static_cast<ResponseMessage*>(msg);
        e.code(resp->status);
    }
    e.token((const char*)&msg->token, sizeof(msg->token));
    // TODO: Support user-provided options
    if (msg->isRequest) {
        auto req = static_cast<RequestMessage*>(msg);
        bool getBlock = false; // See control vs descriptive usage of block options in RFC 7959, 2.3
        if (req->etagSize > 0) {
            // If ETag presents in the request, we're requesting a message block, not sending it
            assert(req->blockIndex.has_value());
            e.option(CoapOption::ETAG /* 4 */, req->etag, req->etagSize);
            getBlock = true;
        }
        e.option(CoapOption::URI_PATH /* 11 */, &req->uri, 1); // TODO: Support longer URIs
        if (req->blockIndex.has_value()) {
            if (getBlock) {
                auto opt = encodeBlockOption(req->blockIndex.value(), false /* m */);
                e.option(CoapOption::BLOCK2 /* 23 */, opt);
            } else {
                assert(req->hasMore.has_value());
                auto opt = encodeBlockOption(req->blockIndex.value(), req->hasMore.value());
                e.option(CoapOption::BLOCK1 /* 27 */, opt);
            }
        }
    } // TODO: Support device-to-cloud blockwise responses
    auto msgBuf = (char*)msgBuf_.buf();
    size_t newPrefixSize = CHECK(e.encode());
    if (newPrefixSize > sizeof(prefix)) {
        LOG(ERROR, "Too many CoAP options");
        return SYSTEM_ERROR_TOO_LARGE;
    }
    if (msg->prefixSize != newPrefixSize) {
        size_t maxMsgSize = newPrefixSize + COAP_BLOCK_SIZE + 1; // Add 1 byte for a payload marker
        if (maxMsgSize > msgBuf_.capacity()) {
            LOG(ERROR, "No enough space in CoAP message buffer");
            return SYSTEM_ERROR_TOO_LARGE;
        }
        // Make room for the updated prefix data
        size_t suffixSize = msg->pos - msgBuf - msg->prefixSize; // Size of the payload data with the payload marker
        std::memmove(msgBuf + newPrefixSize, msgBuf + msg->prefixSize, suffixSize);
        msg->pos += (int)newPrefixSize - (int)msg->prefixSize;
        msg->end = msgBuf + maxMsgSize;
        msg->prefixSize = newPrefixSize;
    }
    std::memcpy(msgBuf, prefix, msg->prefixSize);
    return 0;
}

int CoapChannel::sendMessage(CoapMessage* msg) {
    assert(curMsgId_ == msg->id && !msg->next && !msg->prev);
    msgBuf_.set_length(msg->pos - (char*)msgBuf_.buf());
    CHECK_PROTOCOL(protocol_->get_channel().send(msgBuf_));
    msg->coapId = msgBuf_.get_id();
    msg->state = MessageState::WAIT_ACK;
    prependToList(unackMsgs_, msg);
    releaseMessageBuffer();
    return 0;
}

int CoapChannel::sendEmptyAck(int coapId) {
    Message msg;
    CHECK_PROTOCOL(protocol_->get_channel().response(msgBuf_, msg, msgBuf_.capacity() - msgBuf_.length()));
    CoapMessageEncoder e((char*)msg.buf(), msg.capacity());
    e.type(CoapType::ACK);
    e.code(CoapCode::EMPTY);
    e.id(0); // Will be set by the underlying message channel
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

void CoapChannel::releaseMessageBuffer() {
    msgBuf_.clear();
    curMsgId_ = 0;
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

system_tick_t CoapChannel::millis() const {
    return protocol_->get_callbacks().millis();
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

int coap_add_request_handler(const char* uri, int method, int flags, coap_request_callback cb, void* arg, void* reserved) {
    if (!isValidCoapMethod(method) || flags != 0) {
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
    if (!isValidCoapMethod(method) || flags != 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    auto reqId = CHECK(CoapChannel::instance()->beginRequest(msg, uri, static_cast<coap_method>(method), timeout));
    return reqId;
}

int coap_end_request(coap_message* msg, coap_response_callback resp_cb, coap_ack_callback ack_cb,
        coap_error_callback error_cb, void* arg, void* reserved) {
    CHECK(CoapChannel::instance()->endRequest(msg, resp_cb, ack_cb, error_cb, arg));
    return 0;
}

int coap_begin_response(coap_message** msg, int status, int req_id, int flags, void* reserved) {
    CHECK(CoapChannel::instance()->beginResponse(msg, status, req_id));
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
