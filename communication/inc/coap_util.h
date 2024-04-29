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
