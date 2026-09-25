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

#include "v2/coap_channel.h"
#include "coap_api.h"
#include "coap_defs.h"
#include "protocol.h"
#include "spark_protocol_functions.h"

#include "util/coap_message.h"
#include "util/coap_message_channel.h"

#include "system_error.h"

#include <catch2/catch.hpp>

#include <cstring>
#include <vector>

// Protocol instance returned by the stubbed spark_protocol_instance() (see hal_stubs.cpp)
extern particle::protocol::Protocol* g_protocolInstance;

namespace particle::protocol::test {

namespace {

class TestProtocol: public Protocol {
public:
    explicit TestProtocol(MessageChannel& channel) :
            Protocol(channel) {
        SparkCallbacks callbacks = {};
        callbacks.size = sizeof(callbacks);
        callbacks.millis = []() -> system_tick_t {
            return ++s_millis;
        };
        SparkDescriptor descriptor = {};
        descriptor.size = sizeof(descriptor);
        Protocol::init(callbacks, descriptor);
    }

    // Reimplemented from Protocol
    void init(const char* id, const SparkKeys& keys, const SparkCallbacks& callbacks,
            const SparkDescriptor& descriptor) override {
    }
    int command(ProtocolCommands::Enum cmd, uint32_t val, const void* data) override {
        return 0;
    }
    size_t build_hello(Message& msg, uint16_t flags) override {
        return 0;
    }
    int get_status(protocol_status* status) const override {
        return 0;
    }

private:
    static system_tick_t s_millis;
};

system_tick_t TestProtocol::s_millis = 0;

// The production callbacks (LedgerManager::requestErrorCallback(), CloudEvent::sendComplete())
// cast the argument back to the context type and dereference it without a null check
struct CallbackContext {
    int errorCount = 0;
    int errorResult = 0;
    int errorReqId = 0;
    int ackCount = 0;
    int ackReqId = 0;
};

void errorCallback(int error, int reqId, void* arg) {
    REQUIRE(arg);
    auto ctx = static_cast<CallbackContext*>(arg);
    ++ctx->errorCount;
    ctx->errorResult = error;
    ctx->errorReqId = reqId;
}

int ackCallback(int reqId, void* arg) {
    REQUIRE(arg);
    auto ctx = static_cast<CallbackContext*>(arg);
    ++ctx->ackCount;
    ctx->ackReqId = reqId;
    return 0;
}

class CoapChannelFixture;

struct HandlerArg {
    CoapChannelFixture* fixture;
    CallbackContext* ctx;
    coap_ack_callback ackCb;
};

// The CoAP channel is a singleton caching the protocol instance on construction, and other tests
// in this target may construct it first, so the protocol instance is installed at static
// initialization time
CoapMessageChannel& messageChannelSingleton() {
    static CoapMessageChannel channel;
    return channel;
}

TestProtocol& protocolSingleton() {
    static TestProtocol protocol(messageChannelSingleton());
    return protocol;
}

class CoapChannelFixture {
public:
    CoapChannelFixture() :
            messageChannel_(&messageChannelSingleton()),
            channel_(v2::CoapChannel::instance()) {
    }

    ~CoapChannelFixture() {
        channel_->close();
        channel_->removeRequestHandler(REQUEST_PATH, COAP_METHOD_POST);
        for (auto arg: handlerArgs_) {
            delete arg;
        }
    }

    void open() {
        channel_->open();
    }

    void dropConnection() {
        channel_->close();
    }

    int sendServerRequest() {
        CoapMessage m;
        m.type(CoapType::CON);
        m.code(CoapCode::POST);
        m.option(CoapOption::URI_PATH, REQUEST_PATH);
        m.id(++lastMsgId_);
        m.token(&lastMsgToken_, 1);
        messageChannel_->sendMessage(std::move(m));
        return dispatchMessage();
    }

    void sendAck(CoapMessageId id) {
        CoapMessage m;
        m.type(CoapType::ACK);
        m.code(CoapCode::EMPTY);
        m.id(id);
        messageChannel_->sendMessage(std::move(m));
        dispatchMessage();
    }

    void sendReset(CoapMessageId id) {
        CoapMessage m;
        m.type(CoapType::RST);
        m.code(CoapCode::EMPTY);
        m.id(id);
        messageChannel_->sendMessage(std::move(m));
        dispatchMessage();
    }

    CoapMessage receiveMessage() {
        return messageChannel_->receiveMessage();
    }

    bool hasMessages() const {
        return messageChannel_->hasMessages();
    }

    v2::CoapChannel* channel() {
        return channel_;
    }

    // Emulates LedgerManager::requestCallback(): sends a response and passes ctx to the error
    // callback, as the ledger manager does since the null-argument fix
    void setLedgerLikeRequestHandler(CallbackContext* ctx, coap_ack_callback ackCb = nullptr) {
        auto arg = new HandlerArg{this, ctx};
        arg->ackCb = ackCb;
        handlerArgs_.push_back(arg);
        REQUIRE(channel_->addRequestHandler(REQUEST_PATH, COAP_METHOD_POST, 0 /* flags */,
                [](coap_message* msg, const char* path, int method, int reqId, void* opaque) -> int {
                    auto handlerArg = static_cast<HandlerArg*>(opaque);
                    return handlerArg->fixture->respondToRequest(reqId, handlerArg->ctx, handlerArg->ackCb);
                }, arg) == 0);
    }

private:
    static constexpr const char* REQUEST_PATH = "L";

    int respondToRequest(int reqId, CallbackContext* ctx, coap_ack_callback ackCb) {
        RefCountPtr<v2::CoapMessage> resp;
        int r = channel_->beginResponse(resp, COAP_STATUS_CHANGED, reqId, 0 /* flags */);
        if (r < 0) {
            return r;
        }
        r = channel_->endResponse(std::move(resp), ackCb, errorCallback, ctx);
        if (r < 0) {
            return r;
        }
        return 0;
    }

    int dispatchMessage() {
        Message msg;
        int r = messageChannel_->receive(msg);
        REQUIRE(r == ProtocolError::NO_ERROR);
        REQUIRE(msg.length() >= MIN_COAP_MESSAGE_SIZE);
        switch (msg.get_type()) {
        case CoAPType::CON: {
            v2::CoapChannel::MessageBuffer buf = msg;
            return channel_->handleCon(buf);
        }
        case CoAPType::ACK: {
            v2::CoapChannel::MessageBuffer buf = msg;
            return channel_->handleAck(buf);
        }
        case CoAPType::RESET: {
            v2::CoapChannel::MessageBuffer buf = msg;
            return channel_->handleRst(buf);
        }
        default:
            FAIL("Unexpected CoAP message type");
            return 0;
        }
    }

    CoapMessageChannel* messageChannel_;
    v2::CoapChannel* channel_;
    std::vector<HandlerArg*> handlerArgs_;
    CoapMessageId lastMsgId_ = 0;
    char lastMsgToken_ = 'a';
};

// The old CoAP implementation forwards RST messages to the new one, so the channel singleton
// may be constructed before any test in this file runs
const bool g_protocolInstanceInstalled = []() {
    g_protocolInstance = &protocolSingleton();
    return true;
}();

} // namespace

TEST_CASE("v2 CoAP channel preserves the callback argument for error callbacks", "[coap]") {
    SECTION("connection drop while a response is awaiting an ACK") {
        CallbackContext ctx;
        CoapChannelFixture fixture;
        fixture.setLedgerLikeRequestHandler(&ctx);
        fixture.open();
        REQUIRE(fixture.sendServerRequest() >= 0);
        REQUIRE(fixture.hasMessages());
        const auto ack = fixture.receiveMessage();
        CHECK(ack.type() == CoapType::ACK);
        CHECK(ack.code() == (unsigned)CoapCode::EMPTY);
        REQUIRE(fixture.hasMessages());
        const auto resp = fixture.receiveMessage();
        CHECK(resp.type() == CoapType::CON);
        CHECK(resp.code() == (unsigned)CoapCode::CHANGED);
        fixture.dropConnection();
        CHECK(ctx.errorCount == 1);
        CHECK(ctx.errorResult == SYSTEM_ERROR_COAP_CONNECTION_CLOSED);
    }

    SECTION("connection drop while a request is awaiting an ACK") {
        CallbackContext ctx;
        CoapChannelFixture fixture;
        fixture.open();
        RefCountPtr<v2::CoapMessage> msg;
        const int reqId = fixture.channel()->beginRequest(msg, "L", COAP_METHOD_POST, 0 /* timeout */,
                0 /* flags */);
        REQUIRE(reqId > 0);
        REQUIRE(fixture.channel()->endRequest(std::move(msg), nullptr /* resp_cb */,
                nullptr /* ack_cb */, errorCallback, &ctx) == 0);
        REQUIRE(fixture.hasMessages());
        fixture.receiveMessage();
        fixture.dropConnection();
        CHECK(ctx.errorCount == 1);
        CHECK(ctx.errorResult == SYSTEM_ERROR_COAP_CONNECTION_CLOSED);
        CHECK(ctx.errorReqId == reqId);
    }

    SECTION("RST for a response awaiting an ACK") {
        CallbackContext ctx;
        CoapChannelFixture fixture;
        fixture.setLedgerLikeRequestHandler(&ctx);
        fixture.open();
        REQUIRE(fixture.sendServerRequest() >= 0);
        REQUIRE(fixture.hasMessages());
        (void)fixture.receiveMessage();
        const auto resp = fixture.receiveMessage();
        fixture.sendReset(resp.id());
        CHECK(ctx.errorCount == 1);
        CHECK(ctx.errorResult == SYSTEM_ERROR_COAP_MESSAGE_RESET);
    }

    SECTION("RST for a request awaiting an ACK") {
        CallbackContext ctx;
        CoapChannelFixture fixture;
        fixture.open();
        RefCountPtr<v2::CoapMessage> msg;
        const int reqId = fixture.channel()->beginRequest(msg, "L", COAP_METHOD_POST, 0 /* timeout */,
                0 /* flags */);
        REQUIRE(reqId > 0);
        REQUIRE(fixture.channel()->endRequest(std::move(msg), nullptr /* resp_cb */,
                nullptr /* ack_cb */, errorCallback, &ctx) == 0);
        REQUIRE(fixture.hasMessages());
        const auto req = fixture.receiveMessage();
        fixture.sendReset(req.id());
        CHECK(ctx.errorCount == 1);
        CHECK(ctx.errorResult == SYSTEM_ERROR_COAP_MESSAGE_RESET);
        CHECK(ctx.errorReqId == reqId);
    }

    SECTION("ACK callback is invoked with the argument passed to endResponse()") {
        CallbackContext ctx;
        CoapChannelFixture fixture;
        fixture.setLedgerLikeRequestHandler(&ctx, ackCallback);
        fixture.open();
        REQUIRE(fixture.sendServerRequest() >= 0);
        REQUIRE(fixture.hasMessages());
        (void)fixture.receiveMessage();
        const auto resp = fixture.receiveMessage();
        fixture.sendAck(resp.id());
        CHECK(ctx.ackCount == 1);
        CHECK(ctx.errorCount == 0);
    }

    SECTION("ACK callback of a request is invoked with the passed argument") {
        CallbackContext ctx;
        CoapChannelFixture fixture;
        fixture.open();
        RefCountPtr<v2::CoapMessage> msg;
        const int reqId = fixture.channel()->beginRequest(msg, "L", COAP_METHOD_POST, 0 /* timeout */,
                0 /* flags */);
        REQUIRE(reqId > 0);
        REQUIRE(fixture.channel()->endRequest(std::move(msg), nullptr /* resp_cb */,
                ackCallback, errorCallback, &ctx) == 0);
        REQUIRE(fixture.hasMessages());
        const auto req = fixture.receiveMessage();
        fixture.sendAck(req.id());
        CHECK(ctx.ackCount == 1);
        CHECK(ctx.ackReqId == reqId);
    }
}

} // namespace particle::protocol::test