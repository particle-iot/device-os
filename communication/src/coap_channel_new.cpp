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

#include <type_traits>
#include <algorithm>
#include <optional>

#include "coap_channel_new.h"
#include "coap_payload.h"
#include "coap_options.h"
#include "coap_message_encoder.h"
#include "coap_message_decoder.h"
#include "coap_util.h"
#include "protocol.h"
#include "spark_protocol_functions.h"

#include "c_string.h"
#include "random.h"
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
const size_t MAX_TAG_SIZE = 8; // Maximum size of an ETag (RFC 7252) or Request-Tag (RFC 9175) option

const unsigned BLOCK_SZX = 6; // Value of the SZX field for 1024-byte blocks (RFC 7959, 2.2)
static_assert(COAP_BLOCK_SIZE == 1024); // When changing the block size, make sure to update BLOCK_SZX accordingly

const unsigned DEFAULT_REQUEST_TIMEOUT = 60000;

static_assert(COAP_INVALID_REQUEST_ID == 0); // Used by value in the code

int parseUriPath(const char* path, CoapOptions& opts) {
    auto p = path;
    for (auto p2 = p;; ++p2) {
        if (*p2 == '/' || *p2 == '\0') {
            // Skip the leading '/' and make sure not to add an empty Uri-Path option if the path is empty
            if (p2 != path) {
                CHECK(opts.add(CoapOption::URI_PATH, p, p2 - p));
                p = p2 + 1;
            }
            if (*p2 == '\0') {
                break;
            }
        }
    }
    return 0;
}

unsigned encodeBlockOption(int num, bool m) {
    unsigned opt = (num << 4) | BLOCK_SZX;
    if (m) {
        opt |= 0x08;
    }
    return opt;
}

int decodeBlockOption(unsigned opt, int& num, bool& m) {
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
template<typename T, typename E, typename = std::enable_if_t<std::is_base_of_v<T, E>>>
inline void addToList(T*& head, E* elem) {
    assert(!elem->next && !elem->prev); // Debug-only
    elem->prev = nullptr;
    elem->next = head;
    if (head) {
        head->prev = elem;
    }
    head = elem;
}

template<typename T, typename E, typename = std::enable_if_t<std::is_base_of_v<T, E> || std::is_base_of_v<E, T>>>
inline void removeFromList(T*& head, E* elem) {
    assert(elem->next || elem->prev);
    if (elem->prev) {
        assert(head != elem);
        elem->prev->next = elem->next;
    } else {
        assert(head == elem);
        head = static_cast<T*>(elem->next);
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
        head = static_cast<T*>(head->next);
    }
    return nullptr;
}

// Invokes a function object for each element in the list. It's not safe to use this function if the
// function object may delete multiple elements of the list and not just the element for which it
// was called
template<typename T, typename F>
inline void forEachInList(T* head, const F& fn) {
    while (head) {
        // Store the pointer to the next element in case the callback deletes the current element
        auto next = head->next;
        fn(head);
        head = static_cast<T*>(next);
    }
}

// Adds a reference-counted object to a list and increments the reference counter
template<typename T, typename E>
inline void addRefToList(T*& head, RefCountPtr<E> elem) {
    addToList(head, elem.unwrap());
}

// Removes a reference counted object from the list and decrements the reference counter
template<typename T, typename E>
inline void removeRefFromList(T*& head, const RefCountPtr<E>& elem) {
    removeFromList(head, elem.get());
    // Release the unmanaged reference to the object
    elem->release();
}

template<typename T, typename F>
inline RefCountPtr<T> findRefInList(T* head, const F& fn) {
    return findInList(head, fn);
}

template<typename T, typename F>
inline void forEachRefInList(T* head, const F& fn) {
    while (head) {
        // Prevent the object from being deleted by the callback
        RefCountPtr<T> elem(head);
        fn(elem.get());
        head = static_cast<T*>(elem->next);
    }
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
    int flags; // Message flags

    char tag[MAX_TAG_SIZE]; // ETag or Request-Tag option
    size_t tagSize; // Size of the ETag or Request-Tag option
    int coapId; // CoAP message ID
    token_t token; // CoAP token. TODO: Support longer tokens

    RefCountPtr<CoapPayload> payload; // Payload data
    CoapOptions options; // CoAP options

    std::optional<int> blockIndex; // Index of the current message block
    std::optional<bool> hasMore; // Whether more blocks are expected for this message

    char* pos; // Current position in the message buffer. If null, no message data has been written to the buffer yet
    char* end; // End of the message buffer
    size_t prefixSize; // Size of the CoAP framing not including the payload marker

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
            flags(0),
            tag(),
            tagSize(0),
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

    coap_method method; // Method code

    explicit RequestMessage(bool blockRequest = false) :
            CoapMessage(blockRequest ? MessageType::BLOCK_REQUEST : MessageType::REQUEST),
            responseCallback(nullptr),
            blockResponse(nullptr),
            timeSent(0),
            timeout(0),
            method() {
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
    CString path; // Base URI path
    size_t pathLen; // Path length
    coap_method method; // Method code

    coap_request_callback callback; // Callback to invoke when a request is received
    void* callbackArg; // User argument to pass to the callback

    int flags; // Message flags

    RequestHandler* next; // Next handler in the list
    RequestHandler* prev; // Previous handler in the list

    RequestHandler(CString path, size_t pathLen, coap_method method, coap_request_callback callback, void* callbackArg, int flags) :
            path(std::move(path)),
            pathLen(pathLen),
            method(method),
            callback(callback),
            callbackArg(callbackArg),
            flags(flags),
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
        blockResps_(nullptr),
        unackMsgs_(nullptr),
        protocol_(spark_protocol_instance()),
        state_(State::CLOSED),
        lastReqTag_(Random().gen<decltype(lastReqTag_)>()),
        lastMsgId_(0),
        curMsgId_(0),
        sessId_(0),
        pendingCloseError_(0),
        openPending_(false) {
}

CoapChannel::~CoapChannel() {
    if (sentReqs_ || recvReqs_ || blockResps_ || unackMsgs_) {
        LOG(ERROR, "Destroying channel while CoAP exchange is in progress");
    }
    close();
    forEachInList(connHandlers_, [](auto h) {
        delete h;
    });
    forEachInList(reqHandlers_, [](auto h) {
        delete h;
    });
}

int CoapChannel::beginRequest(coap_message** msg, const char* path, coap_method method, int timeout, int flags) {
    if (timeout < 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (state_ != State::OPEN) {
        return SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    auto req = makeRefCountPtr<RequestMessage>();
    if (!req) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    CHECK(parseUriPath(path, req->options));
    auto msgId = ++lastMsgId_;
    req->id = msgId;
    req->requestId = msgId;
    req->sessionId = sessId_;
    req->method = method;
    req->timeout = (timeout > 0) ? timeout : DEFAULT_REQUEST_TIMEOUT;
    req->flags = flags;
    req->state = MessageState::WRITE;
    // Transfer ownership over the message to the calling code
    *msg = reinterpret_cast<coap_message*>(req.unwrap());
    return msgId;
}

int CoapChannel::endRequest(coap_message* apiMsg, coap_response_callback respCallback, coap_ack_callback ackCallback,
        coap_error_callback errorCallback, void* callbackArg) {
    auto msg = RefCountPtr(reinterpret_cast<CoapMessage*>(apiMsg));
    if (msg->type != MessageType::REQUEST) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_REQUEST_CANCELLED;
    }
    auto req = staticPtrCast<RequestMessage>(msg);
    if (req->state != MessageState::WRITE || req->hasMore.value_or(false)) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!req->pos) {
        if (curMsgId_) {
            // TODO: Support asynchronous writing to multiple message instances
            LOG(WARN, "CoAP message buffer is already in use");
            releaseMessageBuffer();
        }
        CHECK(prepareMessage(req));
    } else if (curMsgId_ != req->id) {
        LOG(ERROR, "CoAP message buffer is no longer available");
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    CHECK(sendMessage(req));
    req->responseCallback = respCallback;
    req->ackCallback = ackCallback;
    req->errorCallback = errorCallback;
    req->callbackArg = callbackArg;
    // Take ownership over the message by releasing the reference to the message that was previously
    // managed by the calling code
    req->release();
    return 0;
}

int CoapChannel::beginResponse(coap_message** msg, int status, int requestId, int flags) {
    if (state_ != State::OPEN) {
        return SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    auto req = findRefInList(recvReqs_, [=](auto req) {
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
    // Transfer ownership over the message to the calling code
    *msg = reinterpret_cast<coap_message*>(resp.unwrap());
    return 0;
}

int CoapChannel::endResponse(coap_message* apiMsg, coap_ack_callback ackCallback, coap_error_callback errorCallback,
        void* callbackArg) {
    auto msg = RefCountPtr(reinterpret_cast<CoapMessage*>(apiMsg));
    if (msg->type != MessageType::RESPONSE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_REQUEST_CANCELLED;
    }
    auto resp = staticPtrCast<ResponseMessage>(msg);
    if (resp->state != MessageState::WRITE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!resp->pos) {
        if (curMsgId_) {
            // TODO: Support asynchronous writing to multiple message instances
            LOG(WARN, "CoAP message buffer is already in use");
            releaseMessageBuffer();
        }
        CHECK(prepareMessage(resp));
    } else if (curMsgId_ != resp->id) {
        LOG(ERROR, "CoAP message buffer is no longer available");
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    CHECK(sendMessage(resp));
    resp->ackCallback = ackCallback;
    resp->errorCallback = errorCallback;
    resp->callbackArg = callbackArg;
    // Take ownership over the message by releasing the reference to the message that was previously
    // managed by the calling code
    resp->release();
    return 0;
}

int CoapChannel::writeBlock(coap_message* apiMsg, const char* data, size_t& size, coap_block_callback blockCallback,
        coap_error_callback errorCallback, void* callbackArg) {
    auto msg = RefCountPtr(reinterpret_cast<CoapMessage*>(apiMsg));
    if (msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_REQUEST_CANCELLED;
    }
    if (msg->state != MessageState::WRITE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    bool sendBlock = false;
    if (size > 0) {
        if (!msg->pos) {
            if (curMsgId_) {
                // TODO: Support asynchronous writing to multiple message instances
                LOG(WARN, "CoAP message buffer is already in use");
                releaseMessageBuffer();
            }
            if (msg->blockIndex.has_value()) {
                // Writing another message block
                assert(msg->type == MessageType::REQUEST);
                ++msg->blockIndex.value();
                msg->hasMore = false;
            }
            CHECK(prepareMessage(msg));
            *msg->pos++ = 0xff; // Payload marker
        } else if (curMsgId_ != msg->id) {
            LOG(ERROR, "CoAP message buffer is no longer available");
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
        auto bytesToWrite = size;
        if (msg->pos + bytesToWrite > msg->end) {
            if (msg->type != MessageType::REQUEST || !blockCallback) { // TODO: Support blockwise device-to-cloud responses
                return SYSTEM_ERROR_TOO_LARGE;
            }
            bytesToWrite = msg->end - msg->pos;
            sendBlock = true;
        }
        std::memcpy(msg->pos, data, bytesToWrite);
        msg->pos += bytesToWrite;
        if (sendBlock) {
            if (!msg->blockIndex.has_value()) {
                msg->blockIndex = 0;
                // Add a Request-Tag option
                auto tag = ++lastReqTag_;
                static_assert(sizeof(tag) <= sizeof(msg->tag));
                std::memcpy(msg->tag, &tag, sizeof(tag));
                msg->tagSize = sizeof(tag);
            }
            msg->hasMore = true;
            CHECK(updateMessage(msg)); // Update or add blockwise transfer options to the message
            CHECK(sendMessage(msg));
        }
        size = bytesToWrite;
    }
    msg->blockCallback = blockCallback;
    msg->errorCallback = errorCallback;
    msg->callbackArg = callbackArg;
    return sendBlock ? COAP_RESULT_WAIT_BLOCK : 0;
}

int CoapChannel::readBlock(coap_message* apiMsg, char* data, size_t& size, coap_block_callback blockCallback,
        coap_error_callback errorCallback, void* callbackArg) {
    auto msg = RefCountPtr(reinterpret_cast<CoapMessage*>(apiMsg));
    if (msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_REQUEST_CANCELLED;
    }
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
            LOG(ERROR, "CoAP message buffer is no longer available");
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
        auto bytesToRead = std::min<size_t>(size, msg->end - msg->pos);
        if (data) {
            std::memcpy(data, msg->pos, bytesToRead);
        }
        msg->pos += bytesToRead;
        if (msg->pos == msg->end) {
            releaseMessageBuffer();
            if (msg->hasMore.value_or(false)) {
                if (blockCallback) {
                    assert(msg->type == MessageType::RESPONSE); // TODO: Support cloud-to-device blockwise requests
                    auto resp = staticPtrCast<ResponseMessage>(msg);
                    // Send a new request with the original options and updated block number
                    auto req = resp->blockRequest;
                    assert(req && req->blockIndex.has_value());
                    ++req->blockIndex.value();
                    CHECK(prepareMessage(req));
                    CHECK(sendMessage(std::move(req)));
                    resp->state = MessageState::WAIT_BLOCK;
                    addRefToList(blockResps_, std::move(resp));
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

int CoapChannel::peekBlock(coap_message* apiMsg, char* data, size_t size) {
    auto msg = RefCountPtr(reinterpret_cast<CoapMessage*>(apiMsg));
    if (msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_REQUEST_CANCELLED;
    }
    if (msg->state != MessageState::READ) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (size > 0) {
        if (msg->pos == msg->end) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        if (curMsgId_ != msg->id) {
            // TODO: Support asynchronous reading from multiple message instances
            LOG(ERROR, "CoAP message buffer is no longer available");
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
        size = std::min<size_t>(size, msg->end - msg->pos);
        if (data) {
            std::memcpy(data, msg->pos, size);
        }
    }
    return size;
}

int CoapChannel::createPayload(coap_payload** payload) {
    auto p = makeRefCountPtr<CoapPayload>();
    if (!p) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    *payload = reinterpret_cast<coap_payload*>(p.unwrap());
    return 0;
}

void CoapChannel::destroyPayload(coap_payload* payload) {
    if (!payload) {
        return;
    }
    auto p = reinterpret_cast<CoapPayload*>(payload);
    p->release();
}

int CoapChannel::writePayload(coap_payload* payload, const char* data, size_t size) {
    auto p = reinterpret_cast<CoapPayload*>(payload);
    size_t n = CHECK(p->write(data, size));
    return n;
}

int CoapChannel::readPayload(coap_payload* payload, char* data, size_t size) {
    auto p = reinterpret_cast<CoapPayload*>(payload);
    size_t n = CHECK(p->read(data, size));
    return n;
}

int CoapChannel::setPayloadPos(coap_payload* payload, size_t pos) {
    auto p = reinterpret_cast<CoapPayload*>(payload);
    size_t n = CHECK(p->setPos(pos));
    return n;
}

int CoapChannel::getPayloadPos(coap_payload* payload) {
    auto p = reinterpret_cast<CoapPayload*>(payload);
    return p->pos();
}

int CoapChannel::setPayloadSize(coap_payload* payload, size_t size) {
    auto p = reinterpret_cast<CoapPayload*>(payload);
    CHECK(p->setSize(size));
    return 0;
}

int CoapChannel::getPayloadSize(coap_payload* payload) {
    auto p = reinterpret_cast<CoapPayload*>(payload);
    return p->size();
}

int CoapChannel::setPayload(coap_message* apiMsg, coap_payload* apiPayload) {
    auto msg = RefCountPtr(reinterpret_cast<CoapMessage*>(apiMsg));
    assert(msg);
    if (msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_REQUEST_CANCELLED;
    }
    if (msg->state != MessageState::WRITE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    auto payload = RefCountPtr(reinterpret_cast<CoapPayload*>(apiPayload));
    assert(payload);
    msg->payload = std::move(payload);
    return 0;
}

int CoapChannel::getPayload(coap_message* apiMsg, coap_payload** apiPayload) {
    auto msg = RefCountPtr(reinterpret_cast<CoapMessage*>(apiMsg));
    assert(msg);
    if (msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_REQUEST_CANCELLED;
    }
    // FIXME
    // if (msg->state != MessageState::READ) {
    //     return SYSTEM_ERROR_INVALID_STATE;
    // }
    auto payload = RefCountPtr(reinterpret_cast<CoapPayload*>(apiPayload));
    assert(apiPayload);
    *apiPayload = reinterpret_cast<coap_payload*>(payload.unwrap());
    return 0;
}

int CoapChannel::getOption(coap_message* apiMsg, coap_option** apiOpt, int num) {
    if (num < 0 || (unsigned)num > MAX_COAP_OPTION_NUMBER) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    assert(apiMsg);
    auto msg = RefCountPtr(reinterpret_cast<CoapMessage*>(apiMsg));
    if (msg->state != MessageState::READ) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    auto opt = msg->options.findFirst(num);
    assert(apiOpt);
    *apiOpt = const_cast<coap_option*>(reinterpret_cast<const coap_option*>(opt));
    return 0;
}

int CoapChannel::getNextOption(coap_message* apiMsg, coap_option** apiOpt, int* num) {
    assert(apiOpt);
    auto opt = reinterpret_cast<const CoapOptions::Option*>(*apiOpt);
    if (opt) {
        opt = opt->next();
    } else {
        assert(apiMsg);
        auto msg = reinterpret_cast<const CoapMessage*>(apiMsg);
        opt = msg->options.first();
    }
    *apiOpt = const_cast<coap_option*>(reinterpret_cast<const coap_option*>(opt));
    assert(num);
    *num = opt ? opt->number() : 0;
    return 0;
}

int CoapChannel::getUintOptionValue(coap_option* apiOpt, unsigned* val) {
    auto opt = reinterpret_cast<const CoapOptions::Option*>(apiOpt);
    assert(opt && val);
    *val = opt->toUint();
    return 0;
}

int CoapChannel::getStringOptionValue(coap_option* apiOpt, char* data, size_t size) {
    auto opt = reinterpret_cast<const CoapOptions::Option*>(apiOpt);
    assert(opt);
    if (data && size > 0) {
        auto n = std::min(opt->size(), size - 1);
        std::memcpy(data, opt->data(), n);
        data[n] = '\0';
    }
    return opt->size();
}

int CoapChannel::getOpaqueOptionValue(coap_option* apiOpt, char* data, size_t size) {
    auto opt = reinterpret_cast<const CoapOptions::Option*>(apiOpt);
    assert(opt);
    if (data) {
        std::memcpy(data, opt->data(), std::min(opt->size(), size));
    }
    return opt->size();
}

int CoapChannel::addUintOption(coap_message* apiMsg, int num, unsigned val) {
    if (num < 0 || (unsigned)num > MAX_COAP_OPTION_NUMBER) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    auto msg = RefCountPtr(reinterpret_cast<CoapMessage*>(apiMsg));
    if (msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_REQUEST_CANCELLED;
    }
    if (msg->state != MessageState::WRITE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    CHECK(msg->options.add(num, val));
    return 0;
}

int CoapChannel::addOpaqueOption(coap_message* apiMsg, int num, const char* data, size_t size) {
    if (num < 0 || (unsigned)num > MAX_COAP_OPTION_NUMBER) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    auto msg = RefCountPtr(reinterpret_cast<CoapMessage*>(apiMsg));
    if (msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_REQUEST_CANCELLED;
    }
    if (msg->state != MessageState::WRITE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    CHECK(msg->options.add(num, data, size));
    return 0;
}

void CoapChannel::destroyMessage(coap_message* apiMsg) {
    if (!apiMsg) {
        return;
    }
    auto msg = RefCountPtr<CoapMessage>::wrap(reinterpret_cast<CoapMessage*>(apiMsg)); // Take ownership
    clearMessage(msg);
}

void CoapChannel::cancelRequest(int requestId) {
    if (requestId <= 0) {
        return;
    }
    // Search among the messages for which a user callback may still need to be invoked
    RefCountPtr<CoapMessage> msg = findRefInList(sentReqs_, [=](auto req) {
        return req->type != MessageType::BLOCK_REQUEST && req->id == requestId;
    });
    if (!msg) {
        msg = findRefInList(unackMsgs_, [=](auto msg) {
            return msg->type != MessageType::BLOCK_REQUEST && msg->requestId == requestId;
        });
        if (!msg) {
            msg = findRefInList(blockResps_, [=](auto msg) {
                return msg->requestId == requestId;
            });
        }
    }
    if (msg) {
        clearMessage(msg);
    }
}

int CoapChannel::addRequestHandler(const char* path, coap_method method, int flags, coap_request_callback callback,
        void* callbackArg) {
    if (!callback) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (state_ != State::CLOSED) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    size_t pathLen = 0;
    if (path) {
        if (path[0] == '/') {
            ++path; // Skip the leading '/'
        }
        pathLen = std::strlen(path);
        if (pathLen && path[pathLen - 1] == '/') {
            --pathLen; // Skip the trailing '/'
        }
        if (!pathLen) {
            path = nullptr;
        }
    }
    if (pathLen > COAP_MAX_URI_PATH_LENGTH) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    auto handler = findInList(reqHandlers_, [=](auto h) {
        return h->method == method && h->pathLen == pathLen && (!pathLen || std::memcmp(h->path, path, pathLen) == 0);
    });
    if (handler) {
        handler->callback = callback;
        handler->callbackArg = callbackArg;
    } else {
        CString pathStr(path, pathLen);
        if (!pathStr && path) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        std::unique_ptr<RequestHandler> handler(new(std::nothrow) RequestHandler(std::move(pathStr), pathLen, method, callback,
                callbackArg, flags));
        if (!handler) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        // Keep the handlers sorted from longest to shortest path
        RequestHandler* last = nullptr;
        auto h = reqHandlers_;
        while (h && h->pathLen > pathLen) {
            last = h;
            h = h->next;
        }
        if (h) {
            handler->next = h;
            handler->prev = h->prev;
            h->prev = handler.get();
        } else if (last) {
            handler->prev = last;
            last->next = handler.get();
        }
        if (!handler->prev) {
            reqHandlers_ = handler.get();
        }
        handler.release();
    }
    return 0;
}

void CoapChannel::removeRequestHandler(const char* path, coap_method method) {
    if (state_ != State::CLOSED) {
        LOG(ERROR, "Cannot remove handler while channel is open");
        return;
    }
    size_t pathLen = 0;
    if (path) {
        if (path[0] == '/') {
            ++path; // Skip the leading '/'
        }
        pathLen = std::strlen(path);
        if (pathLen && path[pathLen - 1] == '/') {
            --pathLen; // Skip the trailing '/'
        }
    }
    auto h = findInList(reqHandlers_, [=](auto h) {
        return h->method == method && h->pathLen == pathLen && (!pathLen || std::memcmp(h->path, path, pathLen) == 0);
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
        addToList(connHandlers_, h.release());
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
            // open() is being called from a connection handler
            openPending_ = true;
        }
        return;
    }
    state_ = State::OPENING;
    forEachInList(connHandlers_, [](auto h) {
        assert(h->callback);
        int r = h->callback(0 /* error */, COAP_CONNECTION_OPEN, h->callbackArg);
        if (r < 0) {
            // XXX: Handler errors are not propagated to the protocol layer. We may want to
            // reconsider that
            LOG(ERROR, "Connection handler failed: %d", r);
            h->openFailed = true;
        }
    });
    state_ = State::OPEN;
    if (pendingCloseError_) {
        // close() was called from a connection handler
        int error = pendingCloseError_;
        pendingCloseError_ = 0;
        close(error); // TODO: Call asynchronously
    }
}

void CoapChannel::close(int error) {
    if (!error) {
        error = SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    openPending_ = false;
    if (state_ != State::OPEN) {
        if (state_ == State::OPENING) {
            // close() is being called from a connection handler
            pendingCloseError_ = error;
        }
        return;
    }
    state_ = State::CLOSING;
    // Generate a new session ID to prevent the user code from messing up with the messages during
    // the cleanup
    ++sessId_;
    releaseMessageBuffer();
    // Cancel device requests awaiting a response
    forEachRefInList(sentReqs_, [=](auto req) {
        if (req->type != MessageType::BLOCK_REQUEST && req->state == MessageState::WAIT_RESPONSE && req->errorCallback) {
            req->errorCallback(error, req->id, req->callbackArg); // Callback passed to coap_write_block() or coap_end_request()
        }
        req->state = MessageState::DONE;
        // Release the unmanaged reference to the message
        req->release();
    });
    sentReqs_ = nullptr;
    // Cancel device requests and responses awaiting an ACK
    forEachRefInList(unackMsgs_, [=](auto msg) {
        if (msg->type != MessageType::BLOCK_REQUEST && msg->state == MessageState::WAIT_ACK && msg->errorCallback) {
            msg->errorCallback(error, msg->requestId, msg->callbackArg); // Callback passed to coap_write_block(), coap_end_request() or coap_end_response()
        }
        msg->state = MessageState::DONE;
        msg->release();
    });
    unackMsgs_ = nullptr;
    // Cancel transfer of server blockwise responses
    forEachRefInList(blockResps_, [=](auto msg) {
        assert(msg->state == MessageState::WAIT_BLOCK);
        if (msg->errorCallback) {
            msg->errorCallback(error, msg->requestId, msg->callbackArg); // Callback passed to coap_read_block()
        }
        msg->state = MessageState::DONE;
        msg->release();
    });
    blockResps_ = nullptr;
    // Cancel server requests awaiting a response
    forEachRefInList(recvReqs_, [](auto req) {
        // No need to invoke any callbacks for these
        req->state = MessageState::DONE;
        req->release();
    });
    recvReqs_ = nullptr;
    // Invoke connection handlers
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
        // open() was called from a connection handler
        openPending_ = false;
        open(); // TODO: Call asynchronously
    }
}

int CoapChannel::handleCon(const Message& msgBuf) {
    if (curMsgId_) {
        // TODO: Support asynchronous reading/writing to multiple message instances
        LOG(WARN, "CoAP message buffer is already in use");
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
        LOG(WARN, "CoAP message buffer is already in use");
        // Contents of the buffer have already been overwritten at this point
        releaseMessageBuffer();
    }
    msgBuf_ = msgBuf; // Makes a shallow copy
    CoapMessageDecoder d;
    CHECK(d.decode((const char*)msgBuf_.buf(), msgBuf_.length()));
    if (d.type() != CoapType::ACK) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    return CHECK(handleAck(d));
}

int CoapChannel::handleRst(const Message& msgBuf) {
    if (curMsgId_) {
        // TODO: Support asynchronous reading/writing to multiple message instances
        LOG(WARN, "CoAP message buffer is already in use");
        // Contents of the buffer have already been overwritten at this point
        releaseMessageBuffer();
    }
    msgBuf_ = msgBuf; // Makes a shallow copy
    CoapMessageDecoder d;
    CHECK(d.decode((const char*)msgBuf_.buf(), msgBuf_.length()));
    if (d.type() != CoapType::RST) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    auto msg = findRefInList(unackMsgs_, [=](auto msg) {
        return msg->coapId == d.id();
    });
    if (!msg) {
        return 0;
    }
    assert(msg->state == MessageState::WAIT_ACK);
    if (msg->type == MessageType::BLOCK_REQUEST) {
        auto req = staticPtrCast<RequestMessage>(msg);
        auto resp = RefCountPtr(req->blockResponse); // blockResponse is a raw pointer
        assert(resp);
        if (resp->errorCallback) {
            resp->errorCallback(SYSTEM_ERROR_COAP_MESSAGE_RESET, resp->requestId, resp->callbackArg); // Callback passed to coap_read_response()
        }
    } else if (msg->errorCallback) { // REQUEST or RESPONSE
        msg->errorCallback(SYSTEM_ERROR_COAP_MESSAGE_RESET, msg->requestId, msg->callbackArg); // Callback passed to coap_write_block(), coap_end_request() or coap_end_response()
    }
    clearMessage(msg);
    return Result::HANDLED;
}

int CoapChannel::run() {
    // TODO: ACK timeouts are handled by the old protocol implementation. As of now, the server always
    // replies with piggybacked responses so we don't need to handle separate response timeouts either
    return 0;
}

CoapChannel* CoapChannel::instance() {
    static CoapChannel channel;
    return &channel;
}

int CoapChannel::handleRequest(CoapMessageDecoder& d) {
    if (d.tokenSize() != sizeof(token_t)) { // TODO: Support empty tokens
        return 0;
    }
    // Get the request URI path
    char path[COAP_MAX_URI_PATH_LENGTH + 1] = { '/', '\0' };
    size_t pathLen = 0; // Path length not including the leading '/'
    auto it = d.options();
    while (it.next()) {
        if (it.option() == CoapOption::URI_PATH) {
            pathLen += appendUriPath(path + 1, sizeof(path) - 1, pathLen, it);
        }
    }
    if (pathLen >= COAP_MAX_URI_PATH_LENGTH) {
        LOG(WARN, "URI path is too long");
        // The new CoAP API is implemented as an extension to the old protocol layer so, technically,
        // the request may still be handled elsewhere
        return 0;
    }
    if (pathLen > 0 && path[pathLen] == '/') {
        path[pathLen--] = '\0'; // Remove the trailing '/'
    }
    // Find a request handler
    auto method = d.code();
    auto handler = findInList(reqHandlers_, [=](auto h) {
        if (h->method != method) {
            return false;
        }
        if (!h->pathLen) {
            return true; // Catch-all handler
        }
        if (h->pathLen > pathLen || std::memcmp(h->path, path + 1, h->pathLen) != 0) {
            return false;
        }
        if (h->pathLen < pathLen && path[h->pathLen + 1] != '/') { // Match complete path segments
            return false;
        }
        return true;
    });
    if (!handler) {
        return 0;
    }
    // Parse options
    CoapOptions opts;
    bool hasBlockOpt = false;
    it = d.options();
    while (it.next()) {
        if (it.option() == CoapOption::BLOCK1) {
            hasBlockOpt = true;
        } else {
            CHECK(opts.add(it.option(), it.data(), it.size()));
        }
    }
    if (hasBlockOpt) {
        // TODO: Support cloud-to-device blockwise requests
        LOG(WARN, "Received blockwise request");
        CHECK(sendAck(d.id(), true /* rst */));
        return Result::HANDLED;
    }
    // Acknowledge the request
    assert(d.type() == CoapType::CON); // TODO: Support non-confirmable requests
    CHECK(sendAck(d.id()));
    // Create a message object
    auto req = makeRefCountPtr<RequestMessage>();
    if (!req) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    auto msgId = ++lastMsgId_;
    req->id = msgId;
    req->requestId = msgId;
    req->sessionId = sessId_;
    req->options = std::move(opts);
    req->method = static_cast<coap_method>(method);
    req->coapId = d.id();
    assert(d.tokenSize() == sizeof(req->token));
    std::memcpy(&req->token, d.token(), d.tokenSize());
    req->pos = const_cast<char*>(d.payload());
    req->end = req->pos + d.payloadSize();
    req->state = MessageState::READ;
    addRefToList(recvReqs_, req);
    // Acquire the message buffer
    assert(!curMsgId_); // Cleared in handleCon()
    if (req->pos < req->end) {
        curMsgId_ = msgId;
    }
    NAMED_SCOPE_GUARD(releaseMsgBufGuard, {
        releaseMessageBuffer();
    });
    // Invoke the request handler
    assert(handler->callback);
    int r = handler->callback(reinterpret_cast<coap_message*>(req.get()), path, req->method, req->id, handler->callbackArg);
    if (r < 0) {
        LOG(ERROR, "Request handler failed: %d", r);
        clearMessage(req);
        return Result::HANDLED;
    }
    // Transfer ownership over the message to called code
    req.unwrap();
    releaseMsgBufGuard.dismiss();
    return Result::HANDLED;
}

int CoapChannel::handleResponse(CoapMessageDecoder& d) {
    if (d.tokenSize() != sizeof(token_t)) { // TODO: Support empty tokens
        return 0;
    }
    token_t token = 0;
    std::memcpy(&token, d.token(), d.tokenSize());
    // Find the request which this response is meant for
    auto req = findRefInList(sentReqs_, [=](auto req) {
        return req->token == token;
    });
    if (!req) {
        int r = 0;
        if (d.type() == CoapType::CON) {
            // Check the unack'd requests as this response could arrive before the ACK. In that case,
            // handleResponse() will be called recursively
            r = CHECK(handleAck(d));
        }
        return r; // 0 or Result::HANDLED
    }
    assert(req->state == MessageState::WAIT_RESPONSE);
    removeRefFromList(sentReqs_, req);
    req->state = MessageState::DONE;
    if (d.type() == CoapType::CON) {
        // Acknowledge the response
        CHECK(sendAck(d.id()));
    }
    // Check if it's a blockwise response
    auto resp = RefCountPtr(req->blockResponse); // blockResponse is a raw pointer. If null, a response object hasn't been created yet
    const char* etag = nullptr;
    size_t etagSize = 0;
    int blockIndex = -1;
    bool hasMore = false;
    auto it = d.options();
    while (it.next()) {
        int r = 0;
        if (it.option() == CoapOption::BLOCK2) {
            r = decodeBlockOption(it.toUInt(), blockIndex, hasMore);
        } else if (it.option() == CoapOption::ETAG) {
            etag = it.data();
            etagSize = it.size();
            if (etagSize > MAX_TAG_SIZE) {
                r = SYSTEM_ERROR_COAP;
            }
        }
        if (r < 0) {
            LOG(ERROR, "Failed to decode message options: %d", r);
            if (resp) {
                if (resp->errorCallback) {
                    resp->errorCallback(SYSTEM_ERROR_COAP, resp->requestId, resp->callbackArg);
                }
                clearMessage(resp);
            }
            return Result::HANDLED;
        }
    }
    if (req->type == MessageType::BLOCK_REQUEST) {
        // Received another block of a blockwise response
        assert(req->blockIndex.has_value() && resp);
        if (blockIndex != req->blockIndex.value() || !etag || etagSize != req->tagSize ||
                std::memcmp(etag, req->tag, etagSize) != 0) {
            auto code = d.code();
            LOG(ERROR, "Blockwise transfer failed: %d.%02d", (int)coapCodeClass(code), (int)coapCodeDetail(code));
            if (resp->errorCallback) {
                resp->errorCallback(SYSTEM_ERROR_COAP, resp->requestId, resp->callbackArg);
            }
            clearMessage(resp);
            return Result::HANDLED;
        }
        resp->blockIndex = blockIndex;
        resp->hasMore = hasMore;
        resp->pos = const_cast<char*>(d.payload());
        resp->end = resp->pos + d.payloadSize();
        assert(resp->state == MessageState::WAIT_BLOCK);
        removeRefFromList(blockResps_, resp);
        resp->state = MessageState::READ;
        // Acquire the message buffer
        assert(!curMsgId_); // Cleared in handleCon()
        if (resp->pos < resp->end) {
            curMsgId_ = resp->id;
        }
        NAMED_SCOPE_GUARD(releaseMsgBufGuard, {
            releaseMessageBuffer();
        });
        // Invoke the block handler
        assert(resp->blockCallback);
        int r = resp->blockCallback(reinterpret_cast<coap_message*>(resp.get()), resp->requestId, resp->callbackArg);
        if (r < 0) {
            LOG(ERROR, "Message block handler failed: %d", r);
            clearMessage(resp);
            return Result::HANDLED;
        }
        releaseMsgBufGuard.dismiss();
        return Result::HANDLED;
    }
    if (req->blockIndex.has_value() && req->hasMore.value()) {
        // Received a response for a non-final block of a blockwise request
        auto code = d.code();
        if (code == CoapCode::CONTINUE) {
            req->state = MessageState::WRITE;
            // Invoke the block handler
            assert(req->blockCallback);
            int r = req->blockCallback(reinterpret_cast<coap_message*>(req.get()), req->id, req->callbackArg);
            if (r < 0) {
                LOG(ERROR, "Message block handler failed: %d", r);
                clearMessage(req);
            }
        } else {
            LOG(ERROR, "Blockwise transfer failed: %d.%02d", (int)coapCodeClass(code), (int)coapCodeDetail(code));
            if (req->errorCallback) {
                req->errorCallback(SYSTEM_ERROR_COAP, req->id, req->callbackArg); // Callback passed to coap_write_block()
            }
            clearMessage(req);
        }
        return Result::HANDLED;
    }
    // Received a regular response or the first block of a blockwise response
    if (!req->responseCallback) {
        return Result::HANDLED; // :shrug:
    }
    // Create a message object
    resp = makeRefCountPtr<ResponseMessage>();
    if (!resp) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    resp->id = ++lastMsgId_;
    resp->requestId = req->id;
    resp->sessionId = sessId_;
    resp->coapId = d.id();
    resp->token = token;
    resp->status = d.code();
    resp->pos = const_cast<char*>(d.payload());
    resp->end = resp->pos + d.payloadSize();
    resp->state = MessageState::READ;
    if (blockIndex >= 0) {
        // This CoAP implementation requires the server to use a ETag option with all blockwise
        // responses. The first block must have an index of 0
        if (blockIndex != 0 || !etagSize) {
            LOG(ERROR, "Received invalid blockwise response");
            if (req->errorCallback) {
                req->errorCallback(SYSTEM_ERROR_COAP, req->id, req->callbackArg); // Callback passed to coap_end_request()
            }
            return Result::HANDLED;
        }
        resp->blockIndex = blockIndex;
        resp->hasMore = hasMore;
        resp->blockRequest = req;
        req->type = MessageType::BLOCK_REQUEST;
        req->blockResponse = resp.get();
        req->blockIndex = resp->blockIndex;
        req->hasMore = false;
        req->tagSize = etagSize;
        std::memcpy(req->tag, etag, etagSize);
    }
    // Acquire the message buffer
    assert(!curMsgId_);
    if (resp->pos < resp->end) {
        curMsgId_ = resp->id;
    }
    NAMED_SCOPE_GUARD(releaseMsgBufGuard, {
        releaseMessageBuffer();
    });
    // Invoke the response handler
    int r = req->responseCallback(reinterpret_cast<coap_message*>(resp.get()), resp->status, req->id, req->callbackArg);
    if (r < 0) {
        LOG(ERROR, "Response handler failed: %d", r);
        clearMessage(resp);
        return Result::HANDLED;
    }
    // Transfer ownership over the message to called code
    resp.unwrap();
    releaseMsgBufGuard.dismiss();
    return Result::HANDLED;
}

int CoapChannel::handleAck(CoapMessageDecoder& d) {
    auto msg = findRefInList(unackMsgs_, [=](auto msg) {
        return msg->coapId == d.id();
    });
    if (!msg) {
        return 0;
    }
    assert(msg->state == MessageState::WAIT_ACK);
    // For a blockwise request, the ACK callback is invoked when the last message block is acknowledged
    if (msg->ackCallback && ((msg->type == MessageType::REQUEST && !msg->hasMore.value_or(false)) ||
            msg->type == MessageType::RESPONSE)) {
        int r = msg->ackCallback(msg->requestId, msg->callbackArg);
        if (r < 0) {
            LOG(ERROR, "ACK handler failed: %d", r);
            clearMessage(msg);
            return Result::HANDLED;
        }
    }
    if (msg->state == MessageState::WAIT_ACK) {
        removeRefFromList(unackMsgs_, msg);
        msg->state = MessageState::DONE;
        if (msg->type == MessageType::REQUEST || msg->type == MessageType::BLOCK_REQUEST) {
            msg->state = MessageState::WAIT_RESPONSE;
            addRefToList(sentReqs_, staticPtrCast<RequestMessage>(msg));
            if (isCoapResponseCode(d.code())) {
                CHECK(handleResponse(d));
            }
        }
    }
    return Result::HANDLED;
}

int CoapChannel::prepareMessage(const RefCountPtr<CoapMessage>& msg) {
    assert(!curMsgId_);
    CHECK_PROTOCOL(protocol_->get_channel().create(msgBuf_));
    if (msg->type == MessageType::REQUEST || msg->type == MessageType::BLOCK_REQUEST) {
        msg->token = protocol_->get_next_token();
    }
    msg->prefixSize = 0;
    msg->pos = (char*)msgBuf_.buf();
    CHECK(updateMessage(msg));
    curMsgId_ = msg->id;
    return 0;
}

int CoapChannel::updateMessage(const RefCountPtr<CoapMessage>& msg) {
    assert(curMsgId_ == msg->id);
    char prefix[128];
    CoapMessageEncoder e(prefix, sizeof(prefix));
    e.type(CoapType::CON);
    e.id(0); // Will be set by the underlying message channel
    bool isRequest = msg->type == MessageType::REQUEST || msg->type == MessageType::BLOCK_REQUEST;
    if (isRequest) {
        auto req = staticPtrCast<RequestMessage>(msg);
        e.code((int)req->method);
    } else {
        auto resp = staticPtrCast<ResponseMessage>(msg);
        e.code(resp->status);
    }
    e.token((const char*)&msg->token, sizeof(msg->token));

    if (isRequest) {
        auto req = staticPtrCast<RequestMessage>(msg);
        if (req->type == MessageType::BLOCK_REQUEST && req->tagSize > 0) {
            // Requesting the next block of a blockwise response
            encodeOption(e, req, CoapOption::ETAG /* 4 */, req->tag, req->tagSize);
        }
        if (req->blockIndex.has_value()) {
            // See control vs descriptive usage of the block options in RFC 7959, 2.3
            if (req->type == MessageType::BLOCK_REQUEST) {
                auto opt = encodeBlockOption(req->blockIndex.value(), false /* m */);
                encodeOption(e, req, CoapOption::BLOCK2 /* 23 */, opt);
            } else {
                assert(req->hasMore.has_value());
                auto opt = encodeBlockOption(req->blockIndex.value(), req->hasMore.value());
                encodeOption(e, req, CoapOption::BLOCK1 /* 27 */, opt);
            }
        }
        if (req->type == MessageType::REQUEST && req->tagSize > 0) {
            // Sending the next block of a blockwise request
            encodeOption(e, req, CoapOption::REQUEST_TAG /* 292 */, req->tag, req->tagSize);
        }
    } // TODO: Support device-to-cloud blockwise responses

    // Encode remaining options
    encodeOptions(e, msg);

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

int CoapChannel::sendMessage(RefCountPtr<CoapMessage> msg) {
    assert(curMsgId_ == msg->id);
    msgBuf_.set_length(msg->pos - (char*)msgBuf_.buf());
    CHECK_PROTOCOL(protocol_->get_channel().send(msgBuf_));
    msg->coapId = msgBuf_.get_id();
    msg->state = MessageState::WAIT_ACK;
    msg->pos = nullptr;
    addRefToList(unackMsgs_, std::move(msg));
    releaseMessageBuffer();
    return 0;
}

int CoapChannel::sendAck(int coapId, bool rst) {
    Message msg;
    CHECK_PROTOCOL(protocol_->get_channel().response(msgBuf_, msg, msgBuf_.capacity() - msgBuf_.length()));
    CoapMessageEncoder e((char*)msg.buf(), msg.capacity());
    e.type(rst ? CoapType::RST : CoapType::ACK);
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

void CoapChannel::clearMessage(const RefCountPtr<CoapMessage>& msg) {
    if (!msg || msg->sessionId != sessId_) {
        return;
    }
    switch (msg->state) {
    case MessageState::READ: {
        if (msg->type == MessageType::REQUEST) {
            removeRefFromList(recvReqs_, msg);
        }
        break;
    }
    case MessageState::WAIT_ACK: {
        removeRefFromList(unackMsgs_, msg);
        break;
    }
    case MessageState::WAIT_RESPONSE: {
        assert(msg->type == MessageType::REQUEST);
        removeRefFromList(sentReqs_, msg);
        break;
    }
    case MessageState::WAIT_BLOCK: {
        assert(msg->type == MessageType::RESPONSE);
        auto resp = staticPtrCast<ResponseMessage>(msg);
        removeRefFromList(blockResps_, resp);
        // Cancel the ongoing block request for this response
        auto req = resp->blockRequest;
        assert(req);
        switch (req->state) {
        case MessageState::WAIT_ACK: {
            removeRefFromList(unackMsgs_, req);
            break;
        }
        case MessageState::WAIT_RESPONSE: {
            removeRefFromList(sentReqs_, req);
            break;
        }
        default:
            break;
        }
        req->state = MessageState::DONE;
        break;
    }
    default:
        break;
    }
    if (curMsgId_ == msg->id) {
        releaseMessageBuffer();
    }
    msg->state = MessageState::DONE;
}

void CoapChannel::encodeOption(CoapMessageEncoder& e, const RefCountPtr<CoapMessage>& msg, CoapOption num,
        const char* data, size_t size) {
    // Encode the options that must precede the given option in the serialized message
    encodeOptions(e, msg, (unsigned)num);
    // Encode the given option
    e.option(num, data, size);
}

void CoapChannel::encodeOption(CoapMessageEncoder& e, const RefCountPtr<CoapMessage>& msg, CoapOption num, unsigned val) {
    encodeOptions(e, msg, (unsigned)num);
    e.option(num, val);
}

void CoapChannel::encodeOptions(CoapMessageEncoder& e, const RefCountPtr<CoapMessage>& msg, unsigned lastNum) {
    auto lastEncodedNum = e.lastOptionNumber();
    auto opt = msg->options.findNext(lastEncodedNum);
    while (opt && opt->number() <= lastNum) {
        e.option(opt->number(), opt->data(), opt->size());
        opt = opt->next();
    }
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

void CoapChannel::releaseMessageBuffer() {
    msgBuf_.clear();
    curMsgId_ = 0;
}

system_tick_t CoapChannel::millis() const {
    return protocol_->get_callbacks().millis();
}

} // namespace particle::protocol::experimental

using namespace particle::protocol;
using namespace particle::protocol::experimental;

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

int coap_set_payload_pos(coap_payload* payload, size_t pos, void* reserved) {
    CHECK(CoapChannel::instance()->setPayloadPos(payload, pos));
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
