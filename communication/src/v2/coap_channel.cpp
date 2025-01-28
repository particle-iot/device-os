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

#undef LOG_COMPILE_TIME_LEVEL

#include "logging.h"

LOG_SOURCE_CATEGORY("comm.coap")

#include <type_traits>
#include <algorithm>
#include <optional>

#include "mbedtls_config.h" // For MBEDTLS_SSL_MAX_CONTENT_LEN

#include "../coap_channel.h" // For ACK_TIMEOUT, MAX_TRANSMIT_SPAN, etc.
#include "v2/coap_channel.h"
#include "coap_payload.h"
#include "coap_options.h"
#include "coap_message_encoder.h"
#include "coap_message_decoder.h"
#include "coap_util.h"
#include "protocol.h"
#include "spark_protocol_functions.h"
#include "communication_diagnostic.h"

#include "c_string.h"
#include "endian_util.h"
#include "scope_guard.h"
#include "check.h"

#define CHECK_PROTOCOL(_expr) \
        do { \
            auto _r = _expr; \
            if (_r != ::particle::protocol::ProtocolError::NO_ERROR) { \
                return this->handleProtocolError(_r); \
            } \
        } while (false)

namespace particle::protocol::v2 {

namespace {

// Maximum supported size of CoAP framing data not including the payload marker
const size_t MAX_MESSAGE_PREFIX_SIZE = 127;
static_assert(MAX_MESSAGE_PREFIX_SIZE + COAP_BLOCK_SIZE + 1 /* Payload marker */ <= PROTOCOL_BUFFER_SIZE);

const unsigned BLOCK_SZX = 6; // Value of the SZX field for 1024-byte blocks (RFC 7959, 2.2)
static_assert(COAP_BLOCK_SIZE == 1024); // When changing the block size, make sure to update BLOCK_SZX accordingly

const size_t DEFAULT_TAG_SIZE = 4; // Default size of an ETag (RFC 7252) or Request-Tag (RFC 9175) option

// Use the CoAP parameters from the old protocol implementation
const unsigned MAX_TRANSMIT_SPAN = protocol::MAX_TRANSMIT_SPAN;
const unsigned MAX_RETRANSMIT = protocol::MAX_RETRANSMIT;

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

unsigned encodeBlockOptionValue(int num, bool m) {
    unsigned opt = (num << 4) | BLOCK_SZX;
    if (m) {
        opt |= 0x08;
    }
    return opt;
}

int decodeBlockOptionValue(unsigned opt, int& num, bool& m) {
    unsigned szx = opt & 0x07;
    if (szx != BLOCK_SZX) {
        // Server is required to use exactly the same block size
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    num = opt >> 4;
    m = opt & 0x08;
    return 0;
}

bool isInternalOption(unsigned num) {
    switch (num) {
    // CoAP options that don't need to be exposed to the user
    case (unsigned)CoapOption::BLOCK1:
    case (unsigned)CoapOption::BLOCK2:
    case (unsigned)CoapOption::REQUEST_TAG:
        return true;
    default:
        return false;
    }
}

// Converts a legacy token_t value to a CoapToken
inline CoapToken tokenFrom(token_t token) {
    static_assert(sizeof(token) <= CoapToken::MAX_SIZE);
    token = nativeToLittleEndian(token);
    return CoapToken((const char*)&token, sizeof(token));
}

inline system_tick_t transmitTimeout(unsigned transmitCount) {
    // For consistency, use the same algorithm for calculating a retransmission timeout as the old
    // implementation, even though it deviates from the spec
    return protocol::transmit_timeout(transmitCount);
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

struct CoapChannel::Message: CoapMessage {
    coap_block_callback blockCallback; // Callback to invoke when a message block is sent or received
    coap_ack_callback ackCallback; // Callback to invoke when the message is acknowledged
    coap_error_callback errorCallback; // Callback to invoke when an error occurs
    void* callbackArg; // User argument to pass to the callbacks

    int id; // Internal message ID
    int requestId; // Internal ID of the request that started this message exchange
    int sessionId; // ID of the session for which this message was created
    int flags; // Message flags

    CoapToken token; // CoAP token
    CoapTag tag; // ETag or Request-Tag option
    int coapId; // CoAP message ID

    RefCountPtr<CoapPayload> payload; // Payload data
    size_t payloadPos; // Position in the payload data object

    CoapOptions options; // CoAP options

    std::optional<int> blockIndex; // Index of the current message block
    std::optional<bool> hasMore; // Whether more blocks are expected for this message

    char* pos; // Current position in the message buffer. If null, no message data has been written to the buffer yet
    char* end; // End of the message buffer
    size_t prefixSize; // Size of the CoAP framing not including the payload marker

    MessageType type; // Message type
    MessageState state; // Message state

    Message* next; // Next message in the list
    Message* prev; // Previous message in the list

    explicit Message(MessageType type) :
            blockCallback(nullptr),
            ackCallback(nullptr),
            errorCallback(nullptr),
            callbackArg(nullptr),
            id(0),
            requestId(0),
            sessionId(0),
            flags(0),
            coapId(0),
            payloadPos(0),
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
struct CoapChannel::RequestMessage: Message {
    coap_response_callback responseCallback; // Callback to invoke when a response for this request is received

    ResponseMessage* blockResponse; // Response for which this block request is retrieving data

    system_tick_t transmitTime; // Time the request was last sent or received
    unsigned transmitTimeout; // Retransmission timeout
    unsigned transmitCount; // Total number of transmissions
    unsigned timeout; // Request timeout

    coap_method method; // Method code

    explicit RequestMessage(bool blockRequest = false) :
            Message(blockRequest ? MessageType::BLOCK_REQUEST : MessageType::REQUEST),
            responseCallback(nullptr),
            blockResponse(nullptr),
            transmitTime(0),
            transmitTimeout(0),
            transmitCount(0),
            method() {
    }
};

struct CoapChannel::ResponseMessage: Message {
    RefCountPtr<RequestMessage> blockRequest; // Request message used to get the last received block of this message

    int status; // CoAP response code

    ResponseMessage() :
            Message(MessageType::RESPONSE),
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

    RequestHandler(CString path, size_t pathLen, coap_method method, int flags, coap_request_callback callback, void* callbackArg) :
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

struct CoapChannel::EncodingContext {
    CoapMessageEncoder& encoder;
    const Message* message;
    bool hasMoreBlocks;

    EncodingContext(CoapMessageEncoder& encoder, const Message* msg) :
            encoder(encoder),
            message(msg),
            hasMoreBlocks(false) {
    }
};

CoapChannel::CoapChannel() :
        connHandlers_(nullptr),
        reqHandlers_(nullptr),
        sentReqs_(nullptr),
        recvReqs_(nullptr),
        recvBlockReqs_(nullptr),
        blockResps_(nullptr),
        unackMsgs_(nullptr),
        protocol_(spark_protocol_instance()),
        lastReqTag_(CoapTag::generate(DEFAULT_TAG_SIZE)),
        state_(State::CLOSED),
        lastMsgId_(0),
        curMsgId_(0),
        sessId_(0),
        pendingCloseError_(0),
        openPending_(false) {
}

CoapChannel::~CoapChannel() {
    if (sentReqs_ || recvReqs_ || recvBlockReqs_ || blockResps_ || unackMsgs_) {
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

int CoapChannel::beginRequest(RefCountPtr<CoapMessage>& coapMsg, const char* path, coap_method method, int timeout, int flags) {
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
    coapMsg = std::move(req);
    return msgId;
}

int CoapChannel::endRequest(RefCountPtr<CoapMessage> coapMsg, coap_response_callback respCallback, coap_ack_callback ackCallback,
        coap_error_callback errorCallback, void* callbackArg) {
    auto msg = staticPtrCast<Message>(coapMsg);
    if (msg->type != MessageType::REQUEST) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_REQUEST_CANCELLED;
    }
    if (msg->state != MessageState::WRITE || msg->hasMore.value_or(false)) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    auto req = staticPtrCast<RequestMessage>(msg);
    if (!req->pos && curMsgId_) {
        // TODO: Support asynchronous writing to multiple message instances
        LOG(WARN, "CoAP message buffer is already in use");
        releaseMessageBuffer();
    }
    if (req->payload) {
        CHECK(sendPayloadBlock(req));
    } else {
        if (!req->pos) {
            CHECK(prepareMessage(req));
        } else if (curMsgId_ != req->id) {
            LOG(ERROR, "CoAP message buffer is no longer available");
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
        CHECK(sendMessage(req));
    }
    req->responseCallback = respCallback;
    req->ackCallback = ackCallback;
    req->errorCallback = errorCallback;
    req->callbackArg = callbackArg;
    return 0;
}

int CoapChannel::beginResponse(RefCountPtr<CoapMessage>& coapMsg, int status, int reqId, int flags) {
    if (state_ != State::OPEN) {
        return SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    auto req = findRefInList(recvReqs_, [=](auto req) {
        return req->id == reqId;
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
    resp->flags = flags;
    resp->state = MessageState::WRITE;
    coapMsg = std::move(resp);
    return 0;
}

int CoapChannel::endResponse(RefCountPtr<CoapMessage> coapMsg, coap_ack_callback ackCallback, coap_error_callback errorCallback,
        void* callbackArg) {
    auto msg = staticPtrCast<Message>(coapMsg);
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
    return 0;
}

int CoapChannel::writeBlock(const RefCountPtr<CoapMessage>& coapMsg, const char* data, size_t& size, coap_block_callback blockCallback,
        coap_error_callback errorCallback, void* callbackArg) {
    auto msg = staticPtrCast<Message>(coapMsg);
    if (msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_REQUEST_CANCELLED;
    }
    if (msg->state != MessageState::WRITE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (msg->payload || (msg->flags & COAP_MESSAGE_FULL)) {
        // Can't mix coap_write_block() with coap_set_payload() or use it together with the
        // COAP_MESSAGE_FULL flag
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
                msg->tag = lastReqTag_.next();
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

int CoapChannel::readBlock(const RefCountPtr<CoapMessage>& coapMsg, char* data, size_t& size, coap_block_callback blockCallback,
        coap_error_callback errorCallback, void* callbackArg) {
    auto msg = staticPtrCast<Message>(coapMsg);
    if (msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_REQUEST_CANCELLED;
    }
    if (msg->state != MessageState::READ) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (msg->payload) {
        // Can't use coap_read_block() together with the COAP_MESSAGE_FULL flag
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

int CoapChannel::peekBlock(const RefCountPtr<CoapMessage>& coapMsg, char* data, size_t size) {
    auto msg = staticPtrCast<Message>(coapMsg);
    if (msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_REQUEST_CANCELLED;
    }
    if (msg->state != MessageState::READ) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (msg->payload) {
        // Can't use coap_peek_block() together with the COAP_MESSAGE_FULL flag
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

int CoapChannel::setPayload(const RefCountPtr<CoapMessage>& coapMsg, RefCountPtr<CoapPayload> payload) {
    auto msg = staticPtrCast<Message>(coapMsg);
    if (msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_REQUEST_CANCELLED;
    }
    if (msg->state != MessageState::WRITE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!msg->payload && (msg->pos || msg->blockIndex.has_value())) {
        return SYSTEM_ERROR_INVALID_STATE; // Can't mix coap_set_payload() and coap_write_block()
    }
    msg->payload = std::move(payload);
    return 0;
}

int CoapChannel::getPayload(const RefCountPtr<CoapMessage>& coapMsg, RefCountPtr<CoapPayload>& payload) {
    auto msg = staticPtrCast<Message>(coapMsg);
    payload = msg->payload;
    return 0;
}

int CoapChannel::addOption(const RefCountPtr<CoapMessage>& coapMsg, unsigned num, const char* data, size_t size) {
    if (num > MAX_COAP_OPTION_NUMBER) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    auto msg = staticPtrCast<Message>(coapMsg);
    if (msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_REQUEST_CANCELLED;
    }
    if (msg->state != MessageState::WRITE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    CHECK(msg->options.add(num, data, size));
    return 0;
}

int CoapChannel::addOption(const RefCountPtr<CoapMessage>& coapMsg, unsigned num, unsigned val) {
    if (num > MAX_COAP_OPTION_NUMBER) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    auto msg = staticPtrCast<Message>(coapMsg);
    if (msg->sessionId != sessId_) {
        return SYSTEM_ERROR_COAP_REQUEST_CANCELLED;
    }
    if (msg->state != MessageState::WRITE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    CHECK(msg->options.add(num, val));
    return 0;
}

int CoapChannel::getOption(const RefCountPtr<CoapMessage>& coapMsg, unsigned num, const CoapOptionEntry*& opt) {
    auto msg = staticPtrCast<Message>(coapMsg);
    opt = msg->options.findFirst(num);
    return 0;
}

int CoapChannel::getFirstOption(const RefCountPtr<CoapMessage>& coapMsg, const CoapOptionEntry*& opt) {
    auto msg = staticPtrCast<Message>(coapMsg);
    opt = msg->options.first();
    return 0;
}

int CoapChannel::cancelRequest(int reqId) {
    if (reqId <= 0) {
        return 0;
    }
    // Search among the messages for which a user callback may still need to be invoked
    RefCountPtr<Message> msg = findRefInList(sentReqs_, [=](auto req) {
        return req->type != MessageType::BLOCK_REQUEST && req->id == reqId;
    });
    if (!msg) {
        msg = findRefInList(unackMsgs_, [=](auto msg) {
            return msg->type != MessageType::BLOCK_REQUEST && msg->requestId == reqId;
        });
        if (!msg) {
            msg = findRefInList(blockResps_, [=](auto msg) {
                return msg->requestId == reqId;
            });
        }
    }
    if (!msg) {
        return 0;
    }
    clearMessage(msg);
    return COAP_RESULT_CANCELLED;
}

void CoapChannel::disposeMessage(const RefCountPtr<CoapMessage>& coapMsg) {
    auto msg = staticPtrCast<Message>(coapMsg);
    clearMessage(msg);
}

int CoapChannel::addRequestHandler(const char* path, coap_method method, int flags, coap_request_callback callback,
        void* callbackArg) {
    if (!callback) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
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
        std::unique_ptr<RequestHandler> handler(new(std::nothrow) RequestHandler(std::move(pathStr), pathLen, method,
                flags, callback, callbackArg));
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
    // Cancel server blockwise requests in progress
    forEachRefInList(recvBlockReqs_, [](auto req) {
        // No need to invoke any callbacks for these
        req->state = MessageState::DONE;
        req->release();
    });
    recvBlockReqs_ = nullptr;
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

int CoapChannel::handleCon(const MessageBuffer& msgBuf) {
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

int CoapChannel::handleAck(const MessageBuffer& msgBuf) {
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

int CoapChannel::handleRst(const MessageBuffer& msgBuf) {
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
    if (state_ != State::OPEN) {
        return 0;
    }
    // Handle retransmission timeouts for requests with a payload object. Timeouts for other
    // confirmable messages are still handled by the old protocol implementation
    auto now = millis();
    auto msg = findRefInList(unackMsgs_, [&](auto msg) {
        if (msg->type != MessageType::REQUEST) {
            return false;
        }
        // Requests with a payload object have a non-zero `transmitTimeout`
        auto req = static_cast<RequestMessage*>(msg);
        return req->transmitTimeout && now - req->transmitTime >= req->transmitTimeout;
    });
    if (msg) {
        auto req = staticPtrCast<RequestMessage>(msg);
        if (req->transmitCount >= MAX_RETRANSMIT + 1) {
            LOG(ERROR, "CoAP message timeout; ID: %d", req->coapId);
            ++g_unacknowledgedMessageCounter;
            clearMessage(req);
            if (req->errorCallback) {
                req->errorCallback(SYSTEM_ERROR_COAP_TIMEOUT, req->requestId, req->callbackArg);
            }
            return SYSTEM_ERROR_COAP_TIMEOUT;
        }
        // Retransmit the message
        removeRefFromList(unackMsgs_, msg);
        msg->state = MessageState::WRITE;
        LOG(TRACE, "Retransmitting CoAP message; ID: %d; attempt %u of %u", req->coapId, req->transmitCount, MAX_RETRANSMIT);
        CHECK(sendPayloadBlock(req, true /* retransmit */));
        return 0;
    }

    // Handle blockwise transfer timeouts
    auto req = findRefInList(recvBlockReqs_, [&](auto req) {
        // As per RFC 7959, 2.5, a partial request body can be discarded when a period of EXCHANGE_LIFETIME
        // has elapsed since the most recent block was received. Here, MAX_TRANSMIT_SPAN is used instead as
        // it aligns better with the non-spec compliant timings used by the old protocol implementation
        return now - req->transmitTime >= MAX_TRANSMIT_SPAN;
    });
    if (req) {
        LOG(ERROR, "Blockwise transfer timeout; request ID: %d", req->id);
        clearMessage(req);
        return 0;
    }

    // TODO: As of now, the server always replies with piggybacked responses so we don't need to
    // handle separate response timeouts
    return 0;
}

CoapChannel* CoapChannel::instance() {
    static CoapChannel channel;
    return &channel;
}

int CoapChannel::handleRequest(CoapMessageDecoder& d) {
    assert(d.type() == CoapType::CON); // TODO: Support non-confirmable requests

    CoapCode errStatus = CoapCode(); // No error
    int result = handleRequestImpl(d, errStatus);
    if (result < 0) {
        LOG(ERROR, "Failed to handle request: %d", result);
        errStatus = CoapCode::SERVICE_UNAVAILABLE;
    }
    if (errStatus != CoapCode() && result != SYSTEM_ERROR_NO_MEMORY) {
        int r = sendResponseAck(errStatus);
        if (r < 0 && result >= 0) {
            result = r;
        }
    }
    return result;
}

int CoapChannel::handleRequestImpl(CoapMessageDecoder& d, CoapCode& errStatus) {
    char path[COAP_MAX_URI_PATH_LENGTH + 1] = { '/', '\0' };
    size_t pathLen = 0; // Path length not including the leading '/'

    CoapTag reqTag;
    int blockIndex = 0;
    bool hasBlockOpt = false;
    bool hasMore = false;

    // Parse the URI path and blockwise transfer options
    auto it = d.options();
    while (it.next()) {
        int r = 0;
        if (it.option() == CoapOption::BLOCK1) {
            r = decodeBlockOptionValue(it.toUInt(), blockIndex, hasMore);
            hasBlockOpt = true;
        } else if (it.option() == CoapOption::REQUEST_TAG) {
            if (it.size() <= CoapTag::MAX_SIZE) {
                reqTag = CoapTag(it.data(), it.size());
            } else {
                r = SYSTEM_ERROR_COAP;
            }
        } else if (it.option() == CoapOption::URI_PATH) {
            pathLen += appendUriPath(path + 1, sizeof(path) - 1, pathLen, it);
        }
        if (r < 0) {
            LOG(ERROR, "Failed to decode message options: %d", r);
            errStatus = CoapCode::BAD_OPTION;
            return Result::HANDLED; // Swallow the message
        }
    }
    if (pathLen >= COAP_MAX_URI_PATH_LENGTH) {
        LOG(ERROR, "URI path is too long");
        errStatus = CoapCode::BAD_OPTION;
        return Result::HANDLED;
    }
    if (pathLen > 0 && path[pathLen] == '/') {
        path[pathLen--] = '\0'; // Remove the trailing '/'
    }

    RefCountPtr<RequestMessage> req;

    if (hasBlockOpt) {
        // Received a blockwise request
        if (reqTag.isEmpty()) {
            // Blockwise requests are required to have a Request-Tag option even though it's not
            // mandatory as per the spec
            LOG(WARN, "Received blockwise request without Request-Tag option");
            errStatus = CoapCode::BAD_OPTION;
            return Result::HANDLED;
        }
        req = findRefInList(recvBlockReqs_, [&](auto req) {
            return req->tag == reqTag;
        });
        if (req) {
            assert(req->state == MessageState::WAIT_BLOCK);
            removeRefFromList(recvBlockReqs_, req);
            assert(req->blockIndex.has_value());
            if (blockIndex != req->blockIndex.value()) {
                LOG(WARN, "Received blockwise request with unexpected block number");
                errStatus = CoapCode::REQUEST_ENTITY_INCOMPLETE;
                return Result::HANDLED;
            }
            assert(req->payload);
            if (req->payload->size() + d.payloadSize() > COAP_MAX_PAYLOAD_SIZE) {
                LOG(WARN, "Blockwise request is too large");
                errStatus = CoapCode::REQUEST_ENTITY_TOO_LARGE;
                return Result::HANDLED;
            }
        } else if (blockIndex != 0) {
            LOG(WARN, "Received blockwise request with unexpected block number");
            errStatus = CoapCode::REQUEST_ENTITY_INCOMPLETE;
            return Result::HANDLED;
        }
        if ((hasMore && d.payloadSize() != COAP_BLOCK_SIZE) || (!hasMore && (!d.hasPayload() || d.payloadSize() > COAP_BLOCK_SIZE))) {
            LOG(WARN, "Received blockwise request with unexpected size of payload data");
            errStatus = CoapCode::BAD_REQUEST;
            return Result::HANDLED;
        }
    }

    RequestHandler* handler = nullptr;
    auto method = d.code();

    if (!hasBlockOpt || (hasBlockOpt && !hasMore)) {
        // Find a request handler
        handler = findInList(reqHandlers_, [=](auto h) {
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
            // The new CoAP API is implemented as an extension to the old protocol layer so, technically,
            // the request may still be handled elsewhere
            return 0;
        }
        if (hasBlockOpt && !(handler->flags & COAP_MESSAGE_FULL)) {
            // TODO: Support the asynchronous API (coap_read_block(), etc.) for incoming blockwise requests
            LOG(ERROR, "Cannot handle blockwise request using selected handler");
            errStatus = CoapCode::NOT_IMPLEMENTED;
            return Result::HANDLED;
        }
    }

    if (!req) {
        // Parse the remaining options
        CoapOptions opts;
        auto it = d.options();
        while (it.next()) {
            if (!isInternalOption(it.option())) {
                CHECK(opts.add(it.option(), it.data(), it.size()));
            }
        }

        // Create a message instance
        req = makeRefCountPtr<RequestMessage>();
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
        req->token = CoapToken(d.token(), d.tokenSize());
        if (hasBlockOpt || (handler && (handler->flags & COAP_MESSAGE_FULL))) {
            req->payload = makeRefCountPtr<CoapPayload>();
            if (!req->payload) {
                return SYSTEM_ERROR_NO_MEMORY;
            }
        }
        if (hasBlockOpt) {
            req->tag = reqTag;
            req->blockIndex = 0;
            req->state = MessageState::WAIT_BLOCK;
        } else {
            if (!req->payload) {
                req->pos = const_cast<char*>(d.payload());
                req->end = req->pos + d.payloadSize();
            }
            req->state = MessageState::READ;
        }
    }

    if (req->payload) {
        CHECK(req->payload->write(d.payload(), d.payloadSize(), req->payloadPos));
        req->payloadPos += d.payloadSize();
    }

    // Acknowledge the request
    if (hasBlockOpt) {
        if (hasMore) {
            CHECK(sendResponseAck(CoapCode::CONTINUE));
            req->transmitTime = millis();
            ++req->blockIndex.value();
            addRefToList(recvBlockReqs_, req);
            return Result::HANDLED;
        }
        req->state = MessageState::READ;
    }
    CHECK(sendEmptyAck());
    addRefToList(recvReqs_, req);

    if (!req->payload) {
        // Acquire the message buffer
        assert(!curMsgId_); // Cleared in handleCon()
        if (req->pos < req->end) {
            curMsgId_ = req->id;
        }
    }
    NAMED_SCOPE_GUARD(releaseMsgBufGuard, {
        releaseMessageBuffer();
    });

    // Invoke the request handler
    assert(handler && handler->callback);
    int r = handler->callback(reinterpret_cast<coap_message*>(req.get()), path, req->method, req->id, handler->callbackArg);
    if (r < 0) {
        LOG(ERROR, "Request handler failed: %d", r);
        clearMessage(req);
        // TODO: Send an error response if it wasn't sent by the handler
        return Result::HANDLED;
    }

    // Transfer ownership over the message to called code
    req.unwrap();
    releaseMsgBufGuard.dismiss();
    return Result::HANDLED;
}

int CoapChannel::handleResponse(CoapMessageDecoder& d) {
    CoapToken token(d.token(), d.tokenSize());
    // Find the request which this response is meant for
    auto req = findRefInList(sentReqs_, [&](auto req) {
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
        CHECK(sendEmptyAck());
    }
    // Check if it's a blockwise response
    auto resp = RefCountPtr(req->blockResponse); // blockResponse is a raw pointer. If null, a response object hasn't been created yet
    CoapTag etag;
    int blockIndex = -1;
    bool hasMore = false;
    auto it = d.options();
    while (it.next()) {
        int r = 0;
        if (it.option() == CoapOption::BLOCK2) {
            r = decodeBlockOptionValue(it.toUInt(), blockIndex, hasMore);
        } else if (it.option() == CoapOption::ETAG) {
            if (it.size() <= CoapTag::MAX_SIZE) {
                etag = CoapTag(it.data(), it.size());
            } else {
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
        if (blockIndex != req->blockIndex.value() || etag != req->tag) {
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
            if (req->payload) {
                req->payloadPos += COAP_BLOCK_SIZE;
                req->transmitCount = 0;
                CHECK(sendPayloadBlock(req));
            } else {
                // Invoke the block handler
                assert(req->blockCallback);
                int r = req->blockCallback(reinterpret_cast<coap_message*>(req.get()), req->id, req->callbackArg);
                if (r < 0) {
                    LOG(ERROR, "Message block handler failed: %d", r);
                    clearMessage(req);
                }
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
        if (blockIndex != 0 || etag.isEmpty()) {
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
        req->tag = etag;
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
    if (msg->type == MessageType::REQUEST && msg->payload) {
        // Messages with a payload object bypass the old CoAP implementation so the related diagnostics
        // need to be updated separately
        auto req = staticPtrCast<RequestMessage>(msg);
        g_coapRoundTripMSec = millis() - req->transmitTime;
    }
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
            bool noResp = false;
            if (msg->type == MessageType::REQUEST && !msg->hasMore.value_or(false)) {
                // Check if a response is needed
                auto opt = msg->options.findFirst(CoapOption::NO_RESPONSE);
                if (opt && (opt->toUint() & 26) == 26) { // Suppress all response codes (RFC 7967, 2.1)
                    noResp = true;
                }
            }
            if (!noResp) {
                msg->state = MessageState::WAIT_RESPONSE;
                addRefToList(sentReqs_, staticPtrCast<RequestMessage>(msg));
                if (isCoapResponseCode(d.code())) {
                    CHECK(handleResponse(d));
                }
            }
        }
    }
    return Result::HANDLED;
}

int CoapChannel::prepareMessage(const RefCountPtr<Message>& msg, bool retransmit) {
    assert(!curMsgId_);
    CHECK_PROTOCOL(protocol_->get_channel().create(msgBuf_));
    if (!retransmit && (msg->type == MessageType::REQUEST || msg->type == MessageType::BLOCK_REQUEST)) {
        msg->token = tokenFrom(protocol_->get_next_token()); // TODO: Use longer tokens with outgoing requests
    }
    msg->prefixSize = 0;
    msg->pos = (char*)msgBuf_.buf();
    CHECK(updateMessage(msg));
    curMsgId_ = msg->id;
    return 0;
}

int CoapChannel::updateMessage(const RefCountPtr<Message>& msg) {
    assert(curMsgId_ == msg->id);
    char prefix[MAX_MESSAGE_PREFIX_SIZE];
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
    e.token(msg->token.data(), msg->token.size());

    EncodingContext ctx(e, msg.get());
    ctx.hasMoreBlocks = msg->hasMore.value_or(false);

    if (isRequest) {
        auto req = staticPtrCast<RequestMessage>(msg);
        if (req->type == MessageType::BLOCK_REQUEST && req->tag.size() > 0) {
            // Requesting the next block of a blockwise response
            encodeOption(ctx, CoapOption::ETAG /* 4 */, req->tag.data(), req->tag.size());
        }
        if (req->blockIndex.has_value()) {
            // See control vs descriptive usage of the block options in RFC 7959, 2.3
            if (req->type == MessageType::BLOCK_REQUEST) {
                auto opt = encodeBlockOptionValue(req->blockIndex.value(), false /* m */);
                encodeOption(ctx, CoapOption::BLOCK2 /* 23 */, opt);
            } else {
                assert(req->hasMore.has_value());
                auto opt = encodeBlockOptionValue(req->blockIndex.value(), req->hasMore.value());
                encodeOption(ctx, CoapOption::BLOCK1 /* 27 */, opt);
            }
        }
        if (req->type == MessageType::REQUEST && req->tag.size() > 0) {
            // Sending the next block of a blockwise request
            encodeOption(ctx, CoapOption::REQUEST_TAG /* 292 */, req->tag.data(), req->tag.size());
        }
    } // TODO: Support device-to-cloud blockwise responses

    // Encode remaining options
    encodeOptions(ctx);

    auto msgBuf = (char*)msgBuf_.buf();
    size_t newPrefixSize = CHECK(e.encode());
    if (newPrefixSize > MAX_MESSAGE_PREFIX_SIZE) {
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

int CoapChannel::sendMessage(RefCountPtr<Message> msg, bool retransmit, bool passthrough) {
    assert(curMsgId_ == msg->id);
    msgBuf_.set_length(msg->pos - (char*)msgBuf_.buf());
    msgBuf_.passthrough(passthrough);
    if (retransmit) {
        msgBuf_.set_id(msg->coapId);
    }
    CHECK_PROTOCOL(protocol_->get_channel().send(msgBuf_));
    msg->coapId = msgBuf_.get_id();
    msg->state = MessageState::WAIT_ACK;
    msg->pos = nullptr;
    addRefToList(unackMsgs_, std::move(msg));
    releaseMessageBuffer();
    return 0;
}

int CoapChannel::sendPayloadBlock(const RefCountPtr<Message>& msg, bool retransmit) {
    assert(msg->type == MessageType::REQUEST && // TODO: Support device-to-cloud blockwise responses
            msg->payload && !curMsgId_);
    size_t bytesToSend = msg->payload->size() - msg->payloadPos;
    if (bytesToSend > COAP_BLOCK_SIZE || msg->blockIndex.has_value()) {
        if (bytesToSend > COAP_BLOCK_SIZE) {
            bytesToSend = COAP_BLOCK_SIZE;
        }
        if (!msg->blockIndex.has_value()) {
            // Add a Request-Tag option
            msg->tag = lastReqTag_.next();
        }
        msg->blockIndex = msg->payloadPos / COAP_BLOCK_SIZE;
        msg->hasMore = msg->payloadPos + bytesToSend < msg->payload->size();
    }
    CHECK(prepareMessage(msg, retransmit));
    if (bytesToSend > 0) {
        *msg->pos++ = 0xff; // Payload marker
        msg->pos += CHECK(msg->payload->read(msg->pos, bytesToSend, msg->payloadPos));
    }
    CHECK(sendMessage(msg, retransmit, true /* passthrough */));
    // TODO: Handle retransmissions for all messages sent by the new implementation, not just requests
    // that have a payload object
    auto req = staticPtrCast<RequestMessage>(msg);
    req->transmitTime = millis();
    req->transmitTimeout = transmitTimeout(req->transmitCount);
    ++req->transmitCount;
    // Messages with a payload object bypass the old CoAP implementation so the related diagnostics
    // need to be updated separately
    if (retransmit) {
        ++g_retransmittedMessageCounter;
    } else {
        ++g_trasmittedMessageCounter;
    }
    return 0;
}

void CoapChannel::clearMessage(const RefCountPtr<Message>& msg) {
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
        if (msg->type == MessageType::RESPONSE) {
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
        } else {
            assert(msg->type == MessageType::REQUEST);
            removeRefFromList(recvBlockReqs_, msg);
        }
        break;
    }
    default:
        break;
    }
    if (curMsgId_ == msg->id) {
        releaseMessageBuffer();
    }
    msg->payload = nullptr;
    msg->state = MessageState::DONE;
}

int CoapChannel::sendResponseAck(CoapCode code) {
    CHECK(protocol::sendResponseAck(protocol_->get_channel(), msgBuf_, code));
    return 0;
}

int CoapChannel::sendEmptyAck() {
    CHECK(protocol::sendEmptyAck(protocol_->get_channel(), msgBuf_));
    return 0;
}

void CoapChannel::encodeOption(EncodingContext& ctx, CoapOption num, const char* data, size_t size) {
    // Encode the options that must precede the given option in the serialized message
    encodeOptions(ctx, (unsigned)num);
    // Encode the given option
    ctx.encoder.option(num, data, size);
}

void CoapChannel::encodeOption(EncodingContext& ctx, CoapOption num, unsigned val) {
    encodeOptions(ctx, (unsigned)num);
    ctx.encoder.option(num, val);
}

void CoapChannel::encodeOptions(EncodingContext& ctx, unsigned lastNumToEncode) {
    auto lastEncodedNum = ctx.encoder.lastOptionNumber();
    auto opt = ctx.message->options.findNext(lastEncodedNum);
    unsigned num = 0;
    while (opt && (num = opt->number()) <= lastNumToEncode) {
        // The No-Response option is only added to the last block of the request
        if (num != CoapOption::NO_RESPONSE || !ctx.hasMoreBlocks) {
            ctx.encoder.option(num, opt->data(), opt->size());
        }
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

} // namespace particle::protocol::v2
