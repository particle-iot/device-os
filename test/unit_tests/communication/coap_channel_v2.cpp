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
#include "v2/coap_payload.h"
#include "v2/coap_options.h"
#include "coap_api.h"
#include "coap_defs.h"
#include "protocol.h"
#include "spark_protocol_functions.h"

#include "util/coap_message.h"
#include "util/coap_message_channel.h"

#include "system_error.h"

#include <catch2/catch.hpp>

#include <cstring>
#include <new>
#include <string>
#include <vector>

// Protocol instance returned by the stubbed spark_protocol_instance() (see hal_stubs.cpp)
extern particle::protocol::Protocol* g_protocolInstance;

namespace {

bool g_failNextNothrowAlloc = false;

} // namespace

void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    if (g_failNextNothrowAlloc) {
        g_failNextNothrowAlloc = false;
        return nullptr;
    }
    try {
        return ::operator new(size);
    } catch (...) {
        return nullptr;
    }
}

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

    static void advanceMillis(system_tick_t ms) {
        s_millis += ms;
    }

private:
    static system_tick_t s_millis;
};

system_tick_t TestProtocol::s_millis = 0;

class FaultyMessageChannel: public CoapMessageChannel {
public:
    void failSend(bool fail, bool resetSession = false, ProtocolError error = ProtocolError::IO_ERROR_SOCKET_SEND_FAILED) {
        failSend_ = fail;
        resetSession_ = resetSession;
        sendError_ = error;
    }

    ProtocolError send(Message& msg) override {
        if (failSend_) {
            if (resetSession_) {
                // Emulates DTLSMessageChannel::send() failing with a non-network error
                v2::CoapChannel::instance()->close();
                return ProtocolError::IO_ERROR_GENERIC_MBEDTLS_SSL_WRITE;
            }
            return sendError_;
        }
        return CoapMessageChannel::send(msg);
    }

    ProtocolError command(Command cmd, void* arg) override {
        if (cmd == Channel::CLOSE) {
            ++closeCommands_;
        }
        return CoapMessageChannel::command(cmd, arg);
    }

    int closeCommands() const {
        return closeCommands_;
    }

    void resetCloseCommands() {
        closeCommands_ = 0;
    }

private:
    ProtocolError sendError_ = ProtocolError::IO_ERROR_SOCKET_SEND_FAILED;
    bool failSend_ = false;
    bool resetSession_ = false;
    int closeCommands_ = 0;
};

// The production callbacks (LedgerManager::requestErrorCallback(), CloudEvent::sendComplete())
// cast the argument back to the context type and dereference it without a null check
struct CallbackContext {
    int errorCount = 0;
    int errorResult = 0;
    int errorReqId = 0;
    int ackCount = 0;
    int ackReqId = 0;
    int responseCount = 0;
    RefCountPtr<v2::CoapMessage> response;
    bool dropResponseOnError = false;
    int cancelOnError = 0;
    int cancelResult = -1;
    bool closeOnAck = false;
    bool closeOnError = false;
};

void errorCallback(int error, int reqId, void* arg) {
    REQUIRE(arg);
    auto ctx = static_cast<CallbackContext*>(arg);
    ++ctx->errorCount;
    ctx->errorResult = error;
    ctx->errorReqId = reqId;
    if (ctx->dropResponseOnError) {
        // Emulates LedgerManager::handleError() releasing the response message
        ctx->response = RefCountPtr<v2::CoapMessage>();
    }
    if (ctx->cancelOnError) {
        // Emulates an application callback cancelling another event with SYSTEM_THREAD(DISABLED)
        ctx->cancelResult = v2::CoapChannel::instance()->cancelRequest(ctx->cancelOnError);
    }
    if (ctx->closeOnError) {
        // Emulates an application callback publishing another event while the link is down with
        // SYSTEM_THREAD(DISABLED)
        v2::CoapChannel::instance()->close();
    }
}

int ackCallback(int reqId, void* arg) {
    REQUIRE(arg);
    auto ctx = static_cast<CallbackContext*>(arg);
    ++ctx->ackCount;
    ctx->ackReqId = reqId;
    if (ctx->closeOnAck) {
        v2::CoapChannel::instance()->close();
    }
    return 0;
}

int responseCallback(coap_message* msg, int status, int reqId, void* arg) {
    REQUIRE(arg);
    auto ctx = static_cast<CallbackContext*>(arg);
    ++ctx->responseCount;
    ctx->response = RefCountPtr<v2::CoapMessage>::wrap(reinterpret_cast<v2::CoapMessage*>(msg));
    return 0;
}

int blockCallback(coap_message* msg, int reqId, void* arg) {
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
FaultyMessageChannel& messageChannelSingleton() {
    static FaultyMessageChannel channel;
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
        messageChannel_->resetCloseCommands();
    }

    ~CoapChannelFixture() {
        g_failNextNothrowAlloc = false;
        messageChannel_->failSend(false);
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

    int sendPiggybackedResponse(CoapMessageId id, CoapCode code, const std::string& token) {
        CoapMessage m;
        m.type(CoapType::ACK);
        m.code(code);
        m.id(id);
        m.token(token);
        messageChannel_->sendMessage(std::move(m));
        return dispatchMessage();
    }

    // Emulates CloudEvent::send()
    int sendRequestWithPayload(size_t payloadSize, CallbackContext* ctx) {
        auto payload = makeRefCountPtr<v2::CoapPayload>(payloadSize);
        REQUIRE(payload);
        std::string data(payloadSize, 'x');
        REQUIRE(payload->write(data.data(), data.size(), 0 /* pos */) == (int)data.size());
        RefCountPtr<v2::CoapMessage> msg;
        const int reqId = channel_->beginRequest(msg, "E/test", COAP_METHOD_POST, 0 /* timeout */, 0 /* flags */);
        REQUIRE(reqId > 0);
        REQUIRE(channel_->setPayload(msg, std::move(payload)) == 0);
        REQUIRE(channel_->addOption(msg, COAP_OPTION_NO_RESPONSE, 26) == 0);
        REQUIRE(channel_->endRequest(std::move(msg), nullptr /* resp_cb */, ackCallback, errorCallback, ctx) == 0);
        return reqId;
    }

    int sendToDevice(CoapMessage msg) {
        messageChannel_->sendMessage(std::move(msg));
        return dispatchMessage();
    }

    int sendRequest(CallbackContext* ctx, int timeout = 0) {
        RefCountPtr<v2::CoapMessage> msg;
        const int reqId = channel_->beginRequest(msg, "L", COAP_METHOD_GET, timeout, 0 /* flags */);
        REQUIRE(reqId > 0);
        REQUIRE(channel_->endRequest(std::move(msg), responseCallback, nullptr /* ack_cb */, errorCallback, ctx) == 0);
        return reqId;
    }

    // Receives the first block of a blockwise response and returns the request for the next block
    CoapMessage startBlockwiseResponse(CallbackContext* ctx, int timeout = 0) {
        sendRequest(ctx, timeout);
        REQUIRE(hasMessages());
        const auto req = receiveMessage();
        CoapMessage m;
        m.type(CoapType::ACK);
        m.code(CoapCode::CONTENT);
        m.id(req.id());
        m.token(req.token());
        m.option(CoapOption::ETAG, std::string("etag"));
        m.option(CoapOption::BLOCK2, 0x0eu); // NUM=0, M=1, SZX=6
        m.payload(std::string("abc"));
        REQUIRE(sendToDevice(std::move(m)) >= 0);
        REQUIRE(ctx->responseCount == 1);
        char buf[16] = {};
        size_t size = sizeof(buf);
        REQUIRE(channel_->readBlock(ctx->response, buf, size, blockCallback, errorCallback, ctx) == COAP_RESULT_WAIT_BLOCK);
        REQUIRE(hasMessages());
        return receiveMessage();
    }

    void failSend(bool fail, bool resetSession = false) {
        messageChannel_->failSend(fail, resetSession);
    }

    void failSendWith(ProtocolError error) {
        messageChannel_->failSend(true, false /* resetSession */, error);
    }

    int sendRequestWithAckCallback(CallbackContext* ctx) {
        RefCountPtr<v2::CoapMessage> msg;
        const int reqId = channel_->beginRequest(msg, "L", COAP_METHOD_POST, 0 /* timeout */, 0 /* flags */);
        REQUIRE(reqId > 0);
        REQUIRE(channel_->endRequest(std::move(msg), nullptr /* resp_cb */, ackCallback, errorCallback, ctx) == 0);
        return reqId;
    }

    void failNextAllocation() {
        g_failNextNothrowAlloc = true;
    }

    int closeCommands() const {
        return messageChannel_->closeCommands();
    }

    void advanceTime(system_tick_t ms) {
        TestProtocol::advanceMillis(ms);
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

    FaultyMessageChannel* messageChannel_;
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

TEST_CASE("v2 CoAP channel completes a request with a payload object if sending a block fails", "[coap]") {
    SECTION("retransmission of a single-block request fails") {
        CallbackContext ctx;
        CoapChannelFixture fixture;
        fixture.open();
        const int reqId = fixture.sendRequestWithPayload(100, &ctx);
        REQUIRE(fixture.hasMessages());
        (void)fixture.receiveMessage();
        fixture.failSend(true);
        fixture.advanceTime(10000);
        CHECK(fixture.channel()->run() < 0);
        CHECK(ctx.errorCount == 1);
        CHECK(ctx.errorResult == SYSTEM_ERROR_IO);
        CHECK(ctx.errorReqId == reqId);
        CHECK(ctx.ackCount == 0);
        CHECK(fixture.channel()->cancelRequest(reqId) == 0);
        fixture.dropConnection();
        CHECK(ctx.errorCount == 1);
    }

    SECTION("sending the next block after 2.31 Continue fails") {
        CallbackContext ctx;
        CoapChannelFixture fixture;
        fixture.open();
        const int reqId = fixture.sendRequestWithPayload(COAP_BLOCK_SIZE + 100, &ctx);
        REQUIRE(fixture.hasMessages());
        const auto block = fixture.receiveMessage();
        REQUIRE(block.hasOption(CoapOption::BLOCK1));
        fixture.failSend(true);
        CHECK(fixture.sendPiggybackedResponse(block.id(), CoapCode::CONTINUE, block.token()) < 0);
        CHECK(ctx.errorCount == 1);
        CHECK(ctx.errorResult == SYSTEM_ERROR_IO);
        CHECK(ctx.errorReqId == reqId);
        CHECK(ctx.ackCount == 0);
        CHECK(fixture.channel()->cancelRequest(reqId) == 0);
        fixture.dropConnection();
        CHECK(ctx.errorCount == 1);
    }
}

TEST_CASE("v2 CoAP channel keeps the session on a network error", "[coap]") {
    SECTION("retransmission fails") {
        CallbackContext ctx1;
        CallbackContext ctx2;
        CoapChannelFixture fixture;
        fixture.open();
        fixture.sendRequestWithPayload(100, &ctx1);
        fixture.sendRequestWithAckCallback(&ctx2);
        REQUIRE(fixture.hasMessages());
        (void)fixture.receiveMessage();
        REQUIRE(fixture.hasMessages());
        (void)fixture.receiveMessage();
        fixture.failSend(true);
        fixture.advanceTime(10000);
        CHECK(fixture.channel()->run() == SYSTEM_ERROR_IO);
        CHECK(ctx1.errorCount == 1);
        CHECK(ctx1.errorResult == SYSTEM_ERROR_IO);
        CHECK(ctx2.errorCount == 1);
        CHECK(ctx2.errorResult == SYSTEM_ERROR_IO);
        CHECK(fixture.closeCommands() == 0);
    }

    SECTION("sending a request fails") {
        CallbackContext ctx;
        CoapChannelFixture fixture;
        fixture.open();
        fixture.failSend(true);
        RefCountPtr<v2::CoapMessage> msg;
        REQUIRE(fixture.channel()->beginRequest(msg, "L", COAP_METHOD_POST, 0 /* timeout */, 0 /* flags */) > 0);
        CHECK(fixture.channel()->endRequest(msg, nullptr /* resp_cb */, ackCallback, errorCallback, &ctx) == SYSTEM_ERROR_IO);
        CHECK(fixture.closeCommands() == 0);
        // The error is reported to the protocol layer so that it reconnects
        CHECK(fixture.channel()->run() == SYSTEM_ERROR_IO);
        CHECK(fixture.channel()->run() == 0);
        fixture.failSend(false);
        fixture.open();
        RefCountPtr<v2::CoapMessage> msg2;
        CHECK(fixture.channel()->beginRequest(msg2, "L", COAP_METHOD_POST, 0 /* timeout */, 0 /* flags */) > 0);
        CHECK(fixture.channel()->endRequest(msg2, nullptr /* resp_cb */, ackCallback, errorCallback, &ctx) == 0);
        REQUIRE(fixture.hasMessages());
        (void)fixture.receiveMessage();
    }

    SECTION("sending a request fails due to a lack of memory") {
        CallbackContext ctx;
        CoapChannelFixture fixture;
        fixture.open();
        fixture.failSendWith(ProtocolError::INSUFFICIENT_STORAGE);
        RefCountPtr<v2::CoapMessage> msg;
        REQUIRE(fixture.channel()->beginRequest(msg, "L", COAP_METHOD_POST, 0 /* timeout */, 0 /* flags */) > 0);
        CHECK(fixture.channel()->endRequest(msg, nullptr /* resp_cb */, ackCallback, errorCallback, &ctx) == SYSTEM_ERROR_TOO_LARGE);
        CHECK(fixture.closeCommands() == 0);
        CHECK(fixture.channel()->run() == SYSTEM_ERROR_TOO_LARGE);
    }

    SECTION("the error is not reported once the channel is closed") {
        CallbackContext ctx;
        CoapChannelFixture fixture;
        fixture.open();
        fixture.failSend(true);
        RefCountPtr<v2::CoapMessage> msg;
        REQUIRE(fixture.channel()->beginRequest(msg, "L", COAP_METHOD_POST, 0 /* timeout */, 0 /* flags */) > 0);
        CHECK(fixture.channel()->endRequest(msg, nullptr /* resp_cb */, ackCallback, errorCallback, &ctx) == SYSTEM_ERROR_IO);
        // The connection was closed for another reason, e.g. a receive error
        fixture.dropConnection();
        CHECK(fixture.channel()->run() == 0);
    }

    SECTION("the error of a failed retransmission is not reported again") {
        CallbackContext ctx;
        CoapChannelFixture fixture;
        fixture.open();
        fixture.sendRequestWithPayload(100, &ctx);
        REQUIRE(fixture.hasMessages());
        (void)fixture.receiveMessage();
        fixture.failSend(true);
        fixture.advanceTime(10000);
        CHECK(fixture.channel()->run() == SYSTEM_ERROR_IO);
        // The protocol layer closes the connection when run() fails
        fixture.dropConnection();
        CHECK(fixture.channel()->run() == 0);
    }

    SECTION("other errors discard the session") {
        CallbackContext ctx;
        CoapChannelFixture fixture;
        fixture.open();
        fixture.failSend(true, true /* resetSession */);
        RefCountPtr<v2::CoapMessage> msg;
        REQUIRE(fixture.channel()->beginRequest(msg, "L", COAP_METHOD_POST, 0 /* timeout */, 0 /* flags */) > 0);
        CHECK(fixture.channel()->endRequest(msg, nullptr /* resp_cb */, ackCallback, errorCallback, &ctx) < 0);
        CHECK(fixture.closeCommands() == 1);
        CHECK(fixture.channel()->run() == 0);
    }
}

TEST_CASE("v2 CoAP channel completes a request if handling its response fails", "[coap]") {
    SECTION("acknowledging a separate response fails") {
        CallbackContext ctx;
        CoapChannelFixture fixture;
        fixture.open();
        const int reqId = fixture.sendRequest(&ctx);
        REQUIRE(fixture.hasMessages());
        const auto req = fixture.receiveMessage();
        fixture.sendAck(req.id());
        fixture.failSend(true);
        CoapMessage resp;
        resp.type(CoapType::CON);
        resp.code(CoapCode::CONTENT);
        resp.id(1000);
        resp.token(req.token());
        CHECK(fixture.sendToDevice(std::move(resp)) < 0);
        CHECK(ctx.errorCount == 0);
        CHECK(ctx.responseCount == 0);
        fixture.dropConnection();
        CHECK(ctx.errorCount == 1);
        CHECK(ctx.errorResult == SYSTEM_ERROR_COAP_CONNECTION_CLOSED);
        CHECK(ctx.errorReqId == reqId);
    }

    SECTION("acknowledging a separate response resets the session") {
        CallbackContext ctx;
        CoapChannelFixture fixture;
        fixture.open();
        const int reqId = fixture.sendRequest(&ctx);
        REQUIRE(fixture.hasMessages());
        const auto req = fixture.receiveMessage();
        fixture.sendAck(req.id());
        fixture.failSend(true, true /* resetSession */);
        CoapMessage resp;
        resp.type(CoapType::CON);
        resp.code(CoapCode::CONTENT);
        resp.id(1000);
        resp.token(req.token());
        CHECK(fixture.sendToDevice(std::move(resp)) < 0);
        CHECK(ctx.errorCount == 1);
        CHECK(ctx.errorResult == SYSTEM_ERROR_COAP_CONNECTION_CLOSED);
        CHECK(ctx.errorReqId == reqId);
        CHECK(ctx.responseCount == 0);
        fixture.dropConnection();
        CHECK(ctx.errorCount == 1);
    }

    SECTION("acknowledging the next block of a blockwise response resets the session") {
        CallbackContext ctx;
        ctx.dropResponseOnError = true;
        CoapChannelFixture fixture;
        fixture.open();
        const auto blockReq = fixture.startBlockwiseResponse(&ctx);
        fixture.sendAck(blockReq.id());
        fixture.failSend(true, true /* resetSession */);
        CoapMessage block;
        block.type(CoapType::CON);
        block.code(CoapCode::CONTENT);
        block.id(1000);
        block.token(blockReq.token());
        block.option(CoapOption::ETAG, std::string("etag"));
        block.option(CoapOption::BLOCK2, 0x16u); // NUM=1, M=0, SZX=6
        block.payload(std::string("def"));
        CHECK(fixture.sendToDevice(std::move(block)) < 0);
        CHECK(ctx.errorCount == 1);
        CHECK(ctx.errorResult == SYSTEM_ERROR_COAP_CONNECTION_CLOSED);
        fixture.dropConnection();
        CHECK(ctx.errorCount == 1);
    }

    SECTION("response has malformed options") {
        CallbackContext ctx;
        CoapChannelFixture fixture;
        fixture.open();
        const int reqId = fixture.sendRequest(&ctx);
        REQUIRE(fixture.hasMessages());
        const auto req = fixture.receiveMessage();
        CoapMessage resp;
        resp.type(CoapType::ACK);
        resp.code(CoapCode::CONTENT);
        resp.id(req.id());
        resp.token(req.token());
        resp.option(CoapOption::BLOCK2, 0x05u); // SZX=5 is not supported
        CHECK(fixture.sendToDevice(std::move(resp)) >= 0);
        CHECK(ctx.errorCount == 1);
        CHECK(ctx.errorResult == SYSTEM_ERROR_COAP);
        CHECK(ctx.errorReqId == reqId);
        CHECK(ctx.responseCount == 0);
        fixture.dropConnection();
        CHECK(ctx.errorCount == 1);
    }

    SECTION("no memory for the response message") {
        CallbackContext ctx;
        CoapChannelFixture fixture;
        fixture.open();
        const int reqId = fixture.sendRequest(&ctx);
        REQUIRE(fixture.hasMessages());
        const auto req = fixture.receiveMessage();
        fixture.failNextAllocation();
        CHECK(fixture.sendPiggybackedResponse(req.id(), CoapCode::CONTENT, req.token()) == SYSTEM_ERROR_NO_MEMORY);
        CHECK(ctx.errorCount == 1);
        CHECK(ctx.errorResult == SYSTEM_ERROR_NO_MEMORY);
        CHECK(ctx.errorReqId == reqId);
        CHECK(ctx.responseCount == 0);
        fixture.dropConnection();
        CHECK(ctx.errorCount == 1);
    }
}

TEST_CASE("v2 CoAP channel enforces the request timeout", "[coap]") {
    SECTION("custom timeout") {
        CallbackContext ctx;
        CoapChannelFixture fixture;
        fixture.open();
        const int reqId = fixture.sendRequest(&ctx, 5000);
        REQUIRE(fixture.hasMessages());
        fixture.sendAck(fixture.receiveMessage().id());
        fixture.advanceTime(4000);
        CHECK(fixture.channel()->run() == 0);
        CHECK(ctx.errorCount == 0);
        fixture.advanceTime(2000);
        CHECK(fixture.channel()->run() == 0);
        CHECK(ctx.errorCount == 1);
        CHECK(ctx.errorResult == SYSTEM_ERROR_COAP_TIMEOUT);
        CHECK(ctx.errorReqId == reqId);
        fixture.dropConnection();
        CHECK(ctx.errorCount == 1);
    }

    SECTION("default timeout") {
        CallbackContext ctx;
        CoapChannelFixture fixture;
        fixture.open();
        const int reqId = fixture.sendRequest(&ctx);
        REQUIRE(fixture.hasMessages());
        fixture.sendAck(fixture.receiveMessage().id());
        fixture.advanceTime(59000);
        CHECK(fixture.channel()->run() == 0);
        CHECK(ctx.errorCount == 0);
        fixture.advanceTime(2000);
        CHECK(fixture.channel()->run() == 0);
        CHECK(ctx.errorCount == 1);
        CHECK(ctx.errorResult == SYSTEM_ERROR_COAP_TIMEOUT);
        CHECK(ctx.errorReqId == reqId);
    }

    SECTION("request without a response callback") {
        CallbackContext ctx;
        CoapChannelFixture fixture;
        fixture.open();
        fixture.sendRequestWithAckCallback(&ctx);
        REQUIRE(fixture.hasMessages());
        fixture.sendAck(fixture.receiveMessage().id());
        CHECK(ctx.ackCount == 1);
        fixture.advanceTime(61000);
        CHECK(fixture.channel()->run() == 0);
        CHECK(ctx.errorCount == 0);
        fixture.dropConnection();
        CHECK(ctx.errorCount == 0);
    }

    SECTION("non-final block of a request with a payload object") {
        CallbackContext ctx;
        CoapChannelFixture fixture;
        fixture.open();
        const int reqId = fixture.sendRequestWithPayload(COAP_BLOCK_SIZE + 100, &ctx);
        REQUIRE(fixture.hasMessages());
        fixture.sendAck(fixture.receiveMessage().id());
        fixture.advanceTime(61000);
        CHECK(fixture.channel()->run() == 0);
        CHECK(ctx.errorCount == 1);
        CHECK(ctx.errorResult == SYSTEM_ERROR_COAP_TIMEOUT);
        CHECK(ctx.errorReqId == reqId);
        CHECK(ctx.ackCount == 0);
        fixture.dropConnection();
        CHECK(ctx.errorCount == 1);
    }

    SECTION("request for the next block of a blockwise response") {
        CallbackContext ctx;
        CoapChannelFixture fixture;
        fixture.open();
        const auto blockReq = fixture.startBlockwiseResponse(&ctx, 5000);
        fixture.sendAck(blockReq.id());
        fixture.advanceTime(6000);
        CHECK(fixture.channel()->run() == 0);
        CHECK(ctx.errorCount == 1);
        CHECK(ctx.errorResult == SYSTEM_ERROR_COAP_TIMEOUT);
        fixture.dropConnection();
        CHECK(ctx.errorCount == 1);
    }
}

TEST_CASE("v2 CoAP channel matches a separate response received before the ACK by token", "[coap]") {
    SECTION("response for a request awaiting an ACK") {
        CallbackContext ctx;
        CoapChannelFixture fixture;
        fixture.open();
        fixture.sendRequest(&ctx);
        REQUIRE(fixture.hasMessages());
        const auto req = fixture.receiveMessage();
        CoapMessage resp;
        resp.type(CoapType::CON);
        resp.code(CoapCode::CONTENT);
        resp.id(1000);
        resp.token(req.token());
        CHECK(fixture.sendToDevice(std::move(resp)) >= 0);
        CHECK(ctx.responseCount == 1);
        REQUIRE(fixture.hasMessages());
        const auto ack = fixture.receiveMessage();
        CHECK(ack.type() == CoapType::ACK);
        CHECK(ack.id() == 1000);
        fixture.dropConnection();
        CHECK(ctx.errorCount == 0);
    }

    SECTION("response with an unknown token and a colliding message ID") {
        CallbackContext ctx;
        CoapChannelFixture fixture;
        fixture.open();
        const int reqId = fixture.sendRequestWithAckCallback(&ctx);
        REQUIRE(fixture.hasMessages());
        const auto req = fixture.receiveMessage();
        CoapMessage resp;
        resp.type(CoapType::CON);
        resp.code(CoapCode::CONTENT);
        resp.id(req.id());
        resp.token(std::string("zz"));
        CHECK(fixture.sendToDevice(std::move(resp)) >= 0);
        CHECK(ctx.ackCount == 0);
        fixture.dropConnection();
        CHECK(ctx.errorCount == 1);
        CHECK(ctx.errorReqId == reqId);
    }
}

TEST_CASE("v2 CoAP channel completes a message once if its callback closes the channel", "[coap]") {
    SECTION("ACK callback of a request with the No-Response option") {
        CallbackContext ctx;
        ctx.closeOnAck = true;
        CoapChannelFixture fixture;
        fixture.open();
        fixture.sendRequestWithPayload(100, &ctx);
        REQUIRE(fixture.hasMessages());
        fixture.sendAck(fixture.receiveMessage().id());
        CHECK(ctx.ackCount == 1);
        CHECK(ctx.errorCount == 0);
    }

    SECTION("error callback invoked on RST") {
        CallbackContext ctx;
        ctx.closeOnError = true;
        CoapChannelFixture fixture;
        fixture.open();
        fixture.sendRequestWithAckCallback(&ctx);
        REQUIRE(fixture.hasMessages());
        fixture.sendReset(fixture.receiveMessage().id());
        CHECK(ctx.errorCount == 1);
        CHECK(ctx.errorResult == SYSTEM_ERROR_COAP_MESSAGE_RESET);
    }
}

TEST_CASE("v2 CoAP channel completes a request with the No-Response option when it's acknowledged", "[coap]") {
    CallbackContext ctx;
    CoapChannelFixture fixture;
    fixture.open();
    fixture.sendRequestWithPayload(100, &ctx);
    REQUIRE(fixture.hasMessages());
    fixture.sendAck(fixture.receiveMessage().id());
    CHECK(ctx.ackCount == 1);
    fixture.dropConnection();
    CHECK(ctx.errorCount == 0);
}

TEST_CASE("v2 CoAP options store the option data", "[coap]") {
    v2::CoapOptions opts;
    REQUIRE(opts.add(COAP_OPTION_NO_RESPONSE, 26) == 0);
    REQUIRE(opts.add(COAP_OPTION_URI_PATH, "abc", 3) == 0);
    auto opt = opts.findFirst(COAP_OPTION_NO_RESPONSE);
    REQUIRE(opt);
    CHECK(opt->toUint() == 26);
    opt = opts.findFirst(COAP_OPTION_URI_PATH);
    REQUIRE(opt);
    CHECK(std::string(opt->data(), opt->size()) == "abc");
}

TEST_CASE("v2 CoAP channel does not cancel a request that is being completed by close()", "[coap]") {
    CallbackContext ctx1;
    CallbackContext ctx2;
    CoapChannelFixture fixture;
    fixture.open();
    const int reqId1 = fixture.sendRequestWithAckCallback(&ctx1);
    fixture.sendRequestWithAckCallback(&ctx2);
    REQUIRE(fixture.hasMessages());
    (void)fixture.receiveMessage();
    REQUIRE(fixture.hasMessages());
    (void)fixture.receiveMessage();
    ctx2.cancelOnError = reqId1;
    fixture.dropConnection();
    CHECK(ctx2.errorCount == 1);
    CHECK(ctx2.cancelResult == 0);
    CHECK(ctx1.errorCount == 1);
    CHECK(ctx1.errorResult == SYSTEM_ERROR_COAP_CONNECTION_CLOSED);
}

TEST_CASE("v2 CoAP channel reports an error for a blockwise response once", "[coap]") {
    SECTION("RST for a request for the next block") {
        CallbackContext ctx;
        CoapChannelFixture fixture;
        fixture.open();
        const auto blockReq = fixture.startBlockwiseResponse(&ctx);
        fixture.sendReset(blockReq.id());
        CHECK(ctx.errorCount == 1);
        CHECK(ctx.errorResult == SYSTEM_ERROR_COAP_MESSAGE_RESET);
        fixture.dropConnection();
        CHECK(ctx.errorCount == 1);
    }
}

} // namespace particle::protocol::test