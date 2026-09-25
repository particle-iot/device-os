/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

#include "coap_api_fake.h"

#include "coap_message.h"
#include "coap_message_channel.h"
#include "system_fakes.h"

#include "v2/coap_channel.h"
#include "protocol.h"
#include "system_error.h"

#include <algorithm>
#include <stdexcept>
#include <cstring>

// The cloud side of the ledger tests: the "cloud" exchanges real CoAP messages with the real
// v2 CoAP channel. CoapFake keeps its recording/injection API. All messages are assumed to fit
// in a single CoAP block.

namespace particle::test {

namespace {

using protocol::test::CoapMessage;

class TestProtocol: public protocol::Protocol {
public:
    explicit TestProtocol(protocol::MessageChannel& channel) :
            Protocol(channel) {
        SparkCallbacks callbacks = {};
        callbacks.size = sizeof(callbacks);
        callbacks.millis = []() -> system_tick_t {
            return (system_tick_t)currentTime();
        };
        SparkDescriptor descriptor = {};
        descriptor.size = sizeof(descriptor);
        Protocol::init(callbacks, descriptor);
    }

    // Reimplemented from Protocol
    void init(const char* id, const SparkKeys& keys, const SparkCallbacks& cb,
            const SparkDescriptor& desc) override {
    }
    int command(ProtocolCommands::Enum cmd, uint32_t val, const void* data) override {
        return 0;
    }
    size_t build_hello(protocol::Message& msg, uint16_t flags) override {
        return 0;
    }
    int get_status(protocol_status* status) const override {
        return 0;
    }
};

class CloudChannel: public protocol::test::CoapMessageChannel {
public:
    // Reimplemented from CoapMessageChannel
    protocol::ProtocolError receive(protocol::Message& msg) override {
        msg = protocol::Message();
        return protocol::ProtocolError::NO_ERROR;
    }
};

CloudChannel& transport() {
    static CloudChannel channel;
    return channel;
}

TestProtocol& protocolInstance() {
    static TestProtocol protocol(transport());
    return protocol;
}

void sendToDevice(const CoapMessage& msg) {
    // The channel makes a shallow copy of the message and uses the spare space of its buffer to
    // allocate the empty ACK for the request
    static std::string buf;
    buf = msg.encode();
    buf.resize(buf.size() + 512);
    protocol::Message m((uint8_t*)buf.data(), buf.size(), buf.size() - 512);
    m.set_id(msg.id());
    auto channel = protocol::v2::CoapChannel::instance();
    switch (msg.type()) {
    case protocol::CoapType::CON: {
        channel->handleCon(m);
        break;
    }
    case protocol::CoapType::ACK: {
        channel->handleAck(m);
        break;
    }
    case protocol::CoapType::RST: {
        channel->handleRst(m);
        break;
    }
    default:
        throw std::runtime_error("Unsupported CoAP message type");
    }
}

} // namespace

CoapFake::CoapFake() :
        channel_(nullptr),
        lastReqId_(0),
        connected_(false) {
    // The channel caches the protocol instance on construction
    protocolInstance();
    channel_ = protocol::v2::CoapChannel::instance();
}

CoapFake& CoapFake::instance() {
    // Intentionally leaked so that the fake outlives static objects that use it in their destructors
    static auto fake = new CoapFake();
    return *fake;
}

void CoapFake::connect() {
    drain();
    channel_->open();
    connected_ = true;
}

void CoapFake::disconnect(int error) {
    drain();
    channel_->close(error);
    connected_ = false;
}

const CoapFake::Sent* CoapFake::lastRequest() const {
    const_cast<CoapFake*>(this)->drain();
    if (requests_.empty()) {
        return nullptr;
    }
    return &requests_.back();
}

void CoapFake::respond(int reqId, int status, const std::string& payload) {
    drain();
    auto it = std::find_if(requests_.begin(), requests_.end(), [reqId](const Sent& s) {
        return s.reqId == reqId && !s.completed;
    });
    if (it == requests_.end()) {
        throw std::runtime_error("Unknown request ID");
    }
    it->completed = true;
    CoapMessage resp;
    resp.type(protocol::CoapType::ACK);
    resp.code((protocol::CoapCode)status);
    resp.id(it->msg.id());
    resp.token(it->msg.token());
    if (!payload.empty()) {
        resp.payload(payload);
    }
    sendToDevice(resp);
}

void CoapFake::failRequest(int reqId, int error) {
    drain();
    auto it = std::find_if(requests_.begin(), requests_.end(), [reqId](const Sent& s) {
        return s.reqId == reqId && !s.completed;
    });
    if (it == requests_.end()) {
        throw std::runtime_error("Unknown request ID");
    }
    it->completed = true;
    CoapMessage rst;
    rst.type(protocol::CoapType::RST);
    rst.code(protocol::CoapCode::EMPTY);
    rst.id(it->msg.id());
    sendToDevice(rst);
}

int CoapFake::sendRequest(const char* uri, int method, const std::string& payload) {
    drain();
    const int fakeReqId = ++lastReqId_;
    CoapMessage req;
    req.type(protocol::CoapType::CON);
    req.code((protocol::CoapCode)method);
    req.id(0x2000 | (fakeReqId & 0xffff));
    req.token((const char*)&fakeReqId, sizeof(fakeReqId));
    req.option(protocol::CoapOption::URI_PATH, uri);
    if (!payload.empty()) {
        req.payload(payload);
    }
    sendToDevice(req);
    return fakeReqId;
}

const CoapFake::Sent* CoapFake::responseFor(int reqId) const {
    const_cast<CoapFake*>(this)->drain();
    auto it = std::find_if(responses_.begin(), responses_.end(), [reqId](const Sent& s) {
        return s.reqId == reqId;
    });
    if (it == responses_.end()) {
        return nullptr;
    }
    return &*it;
}

void CoapFake::failResponse(int reqId, int error) {
    drain();
    auto resp = responseFor(reqId);
    if (!resp) {
        throw std::runtime_error("Unknown request ID");
    }
    CoapMessage rst;
    rst.type(protocol::CoapType::RST);
    rst.code(protocol::CoapCode::EMPTY);
    rst.id(resp->msg.id());
    sendToDevice(rst);
}

void CoapFake::clearLog() {
    requests_.clear();
    responses_.clear();
}

void CoapFake::drain() {
    while (transport().hasMessages()) {
        const auto msg = transport().receiveMessage();
        Sent sent = {};
        sent.msg = msg;
        sent.payload = msg.hasPayload() ? msg.payload() : std::string();
        if (protocol::isCoapRequest(msg.type(), msg.code())) {
            sent.reqId = ++lastReqId_;
            sent.isResponse = false;
            sent.method = msg.code();
            sent.uri = "L";
            requests_.push_back(std::move(sent));
        } else if (protocol::isCoapResponse(msg.type(), msg.code()) && msg.hasToken()) {
            // The channel copies the token of the original request to the response
            int reqId = 0;
            std::memcpy(&reqId, msg.token().data(), std::min(sizeof(reqId), msg.token().size()));
            sent.reqId = reqId;
            sent.isResponse = true;
            sent.status = msg.code();
            responses_.push_back(std::move(sent));
        }
    }
}

} // namespace particle::test

extern "C" particle::protocol::Protocol* spark_protocol_instance(void) {
    return &particle::test::protocolInstance();
}