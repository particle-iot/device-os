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

#include "spark_wiring_stream.h"

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

    coap_message** operator&() {
        coap_destroy_message(msg_, nullptr);
        return &msg_;
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

/**
 * Smart pointer for CoAP payload instances.
 */
class CoapPayloadPtr {
public:
    CoapPayloadPtr() :
            p_(nullptr) {
    }

    explicit CoapPayloadPtr(coap_payload* payload) :
            p_(payload) {
    }

    CoapPayloadPtr(const CoapPayloadPtr&) = delete;

    CoapPayloadPtr(CoapPayloadPtr&& ptr) :
            p_(ptr.p_) {
        ptr.p_ = nullptr;
    }

    ~CoapPayloadPtr() {
        coap_destroy_payload(p_, nullptr);
    }

    coap_payload* get() const {
        return p_;
    }

    coap_payload* release() {
        auto p = p_;
        p_ = nullptr;
        return p;
    }

    void reset(coap_payload* payload = nullptr) {
        coap_destroy_payload(p_, nullptr);
        p_ = payload;
    }

    coap_payload** operator&() {
        coap_destroy_payload(p_, nullptr);
        return &p_;
    }

    CoapPayloadPtr& operator=(const CoapPayloadPtr&) = delete;

    CoapPayloadPtr& operator=(CoapPayloadPtr&& ptr) {
        coap_destroy_payload(p_, nullptr);
        p_ = ptr.p_;
        ptr.p_ = nullptr;
        return *this;
    }

    explicit operator bool() const {
        return p_;
    }

    friend void swap(CoapPayloadPtr& ptr1, CoapPayloadPtr& ptr2) {
        auto p = ptr1.p_;
        ptr1.p_ = ptr2.p_;
        ptr2.p_ = p;
    }

private:
    coap_payload* p_;
};

/**
 * An input stream adapter for CoAP payload data.
 */
class CoapPayloadInputStream: public Stream {
public:
    explicit CoapPayloadInputStream(coap_payload* payload) :
            payload_(payload),
            pos_(0) {
    }

    int read() override {
        char c;
        if (readBytes(&c, 1) != 1) {
            return -1;
        }
        return (unsigned char)c;
    }

    size_t readBytes(char* data, size_t size) override {
        int n = coap_read_payload(payload_, data, size, pos_, nullptr /* reserved */);
        if (n < 0) {
            return 0;
        }
        pos_ += n;
        return n;
    }

    int peek() override {
        char c;
        int n = coap_read_payload(payload_, &c, 1, pos_, nullptr /* reserved */);
        if (n != 1) {
            return -1;
        }
        return (unsigned char)c;
    }

    int available() override {
        int size = coap_get_payload_size(payload_, nullptr /* reserved */);
        if (size < 0) {
            return 0;
        }
        return size - pos_;
    }

    size_t write(uint8_t b) override {
        return 0; // Not supported
    }

    size_t write(const uint8_t* data, size_t size) override {
        return 0; // Not supported
    }

    void flush() override {
    }

private:
    coap_payload* payload_;
    size_t pos_;
};

namespace protocol {

class Message;
class MessageChannel;
class CoapOptionIterator;

/**
 * Send a response ACK.
 *
 * @param channel Message channel.
 * @param msg CON request message.
 * @param code Status code.
 * @param payload Payload data.
 * @param payloadSize Payload size.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int sendResponseAck(MessageChannel& channel, Message& msg, CoapCode code, const char* payload = nullptr, size_t payloadSize = 0);

/**
 * Send an empty ACK.
 *
 * @param channel Message channel.
 * @param msg CON message.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int sendEmptyAck(MessageChannel& channel, Message& msg);

/**
 * Send an RST.
 *
 * @param channel Message channel.
 * @param msg CON or NON message.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int sendRst(MessageChannel& channel, Message& msg);

/**
 * Append an URI path entry to a string.
 *
 * If the path in the buffer is not empty (`pathLen > 0`), appends a separator character to it,
 * otherwise appends just the value of the URI path option.
 *
 * The output is null-terminated unless the size of the buffer is 0.
 *
 * @param buf Destination buffer.
 * @param bufSize Buffer size.
 * @param pathLen Length of the URI path already stored in the buffer.
 * @param it Iterator pointing to a URI path CoAP option.
 * @return Length of the URI path entry plus one character for a path separator if the path in the
 *        buffer wasn't empty.
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
