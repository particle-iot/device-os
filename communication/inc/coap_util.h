/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
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

#include "coap_api.h"
#include "coap_defs.h"
#include "logging.h"

namespace particle {

/**
 * Smart pointer for CoAP message instances.
 */
class CoapMessagePtr {
public:
    CoapMessagePtr() :
            msg_(nullptr) {
    }

    explicit CoapMessagePtr(coap_message* msg) :
            msg_(msg) {
    }

    CoapMessagePtr(const CoapMessagePtr&) = delete;

    CoapMessagePtr(CoapMessagePtr&& ptr) :
            msg_(ptr.msg_) {
        ptr.msg_ = nullptr;
    }

    ~CoapMessagePtr() {
        coap_destroy_message(msg_, nullptr);
    }

    coap_message* get() const {
        return msg_;
    }

    coap_message* release() {
        auto msg = msg_;
        msg_ = nullptr;
        return msg;
    }

    void reset(coap_message* msg = nullptr) {
        coap_destroy_message(msg_, nullptr);
        msg_ = msg;
    }

    CoapMessagePtr& operator=(const CoapMessagePtr&) = delete;

    CoapMessagePtr& operator=(CoapMessagePtr&& ptr) {
        coap_destroy_message(msg_, nullptr);
        msg_ = ptr.msg_;
        ptr.msg_ = nullptr;
        return *this;
    }

    explicit operator bool() const {
        return msg_;
    }

    friend void swap(CoapMessagePtr& ptr1, CoapMessagePtr& ptr2) {
        auto msg = ptr1.msg_;
        ptr1.msg_ = ptr2.msg_;
        ptr2.msg_ = msg;
    }

private:
    coap_message* msg_;
};

namespace protocol {

class MessageChannel;
class CoapOptionIterator;

/**
 * Send an empty ACK or RST message.
 *
 * @param channel Message channel.
 * @param msg Received message.
 * @param type Type of the message to send.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int sendEmptyAckOrRst(MessageChannel& channel, Message& msg, CoapType type);

/**
 * Append an URI path entry to a string.
 *
 * The output is null-terminated unless the size of the buffer is 0.
 *
 * @param buf Destination buffer.
 * @param bufSize Buffer size.
 * @param pathLen Length of the URI path already stored in the buffer.
 * @param it CoAP options iterator.
 * @return Length of the URI path entry plus one character for a path separator.
 */
size_t appendUriPath(char* buf, size_t bufSize, size_t pathLen, const CoapOptionIterator& it);

/**
 * Log the contents of a CoAP message.
 *
 * @param level Logging level.
 * @param category Logging category.
 * @param data Message data.
 * @param size Message size.
 * @param logPayload Whether to log the payload data of the message.
 */
void logCoapMessage(LogLevel level, const char* category, const char* data, size_t size, bool logPayload = false);

} // namespace protocol

} // namespace particle
