/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#include "description.h"

#include "util/coap_message_channel.h"
#include "util/protocol_stub.h"

#include <catch2/catch.hpp>
#include <fakeit.hpp>

namespace {

using namespace particle;
using namespace particle::protocol;
using namespace particle::protocol::test;

using namespace fakeit;

const size_t BLOCK_SIZE = 1024;

// Helper class for encoding Block1 and Block2 options
class BlockOption {
public:
    BlockOption() :
            index_(0),
            size_(BLOCK_SIZE),
            more_(false),
            valid_(false) {
    }

    BlockOption& index(unsigned index) {
        index_ = index;
        valid_ = true;
        return *this;
    }

    BlockOption& size(unsigned size) {
        size_ = size;
        valid_ = true;
        return *this;
    }

    BlockOption& more(bool more) {
        more_ = more;
        valid_ = true;
        return *this;
    }

    bool isValid() const {
        return valid_;
    }

    operator unsigned() const {
        if (!valid_) {
            return 0;
        }
        unsigned szx = 0;
        switch (size_) {
            case 512: szx = 5; break;
            case 1024: szx = 6; break;
            default: throw std::runtime_error("Unsupported block size");
        }
        unsigned val = (index_ << 4) | szx;
        if (more_) {
            val |= 0x08;
        }
        return val;
    }

private:
    unsigned index_;
    unsigned size_;
    bool more_, valid_;
};

class DescriptionWrapper {
public:
    explicit DescriptionWrapper() :
            proto_(&channel_),
            desc_(&proto_),
            descAckFlags_(0),
            lastMsgId_(0),
            lastMsgToken_('a' - 1) {
    }

    // Sends a Describe request to the device
    int sendRequest(int descFlags, const BlockOption& block2 = BlockOption()) {
        CoapMessage m;
        m.type(CoapType::CON);
        m.code(CoapCode::GET);
        m.option(CoapOption::URI_PATH, "d");
        m.option(CoapOption::URI_QUERY, descFlags);
        if (block2.isValid()) {
            m.option(CoapOption::BLOCK2, block2);
        }
        return sendMessage(std::move(m));
    }

    // Sends a CoAP message to the device
    int sendMessage(CoapMessage msg) {
        if (!msg.hasId()) {
            msg.id(++lastMsgId_);
        }
        if (isCoapRequest(msg.type(), msg.code()) && !msg.hasToken()) {
            ++lastMsgToken_;
            msg.token(&lastMsgToken_, 1);
        }
        channel_.sendMessage(std::move(msg));
        Message m;
        int r = channel_.receive(m);
        if (r == ProtocolError::NO_ERROR) {
            const auto type = Messages::decodeType(m.buf(), m.length());
            switch (type) {
            case CoAPMessageType::DESCRIBE: {
                r = desc_.receiveRequest(m);
                break;
            }
            case CoAPMessageType::EMPTY_ACK: {
                descAckFlags_ = 0;
                r = desc_.receiveAckOrRst(m, &descAckFlags_);
                break;
            }
            default:
                break;
            }
        }
        return r;
    }

    // Receives a CoAP message from the device
    CoapMessage receiveMessage() {
        return channel_.receiveMessage();
    }

    // Skips N messages received from the device
    void skipMessages(unsigned count) {
        channel_.skipMessages(count);
    }

    // Returns true if there's a message received from the device
    bool hasMessages() const {
        return channel_.hasMessages();
    }

    // Returns the ID of the last message sent to the device
    CoapMessageId lastMessageId() const {
        return lastMsgId_;
    }

    // Returns the token of the last message sent to the device
    std::string lastMessageToken() const {
        return std::string(1, lastMsgToken_);
    }

    int describeAckFlags() const {
        return descAckFlags_;
    }

    void tick(system_tick_t ms) {
        proto_.callbacks()->addMillis(ms);
        CHECK(desc_.processTimeouts() == ProtocolError::NO_ERROR);
    }

    Mock<DescriptorCallbacks> descriptorMock() {
        return Mock<DescriptorCallbacks>(*proto_.descriptor());
    }

    ProtocolStub* protocol() {
        return &proto_;
    }

    // Returns the underlying Description object
    Description* get() {
        return &desc_;
    }

private:
    CoapMessageChannel channel_;
    ProtocolStub proto_;
    Description desc_;
    int descAckFlags_;
    CoapMessageId lastMsgId_;
    char lastMsgToken_;
};

} // namespace

TEST_CASE("Description") {
    DescriptionWrapper d;
    auto cb = d.descriptorMock();

    SECTION("can send a regular Describe request") {
        When(Method(cb, appendSystemInfo)).Do([](appender_fn append, void* arg, void* reserved) {
            auto s = std::string(BLOCK_SIZE, 'a');
            append(arg, (const uint8_t*)s.data(), s.size());
            return true;
        });
        // Send a request to the server
        d.get()->sendRequest(DescriptionType::DESCRIBE_SYSTEM);
        // Receive the request
        auto m = d.receiveMessage();
        CHECK(m.type() == CoapType::CON);
        CHECK(m.code() == CoapCode::POST);
        CHECK(m.option(CoapOption::URI_PATH).toString() == "d");
        CHECK(m.option(CoapOption::URI_QUERY).toUInt() == DescriptionType::DESCRIBE_SYSTEM);
        CHECK((!m.hasOption(CoapOption::BLOCK1) && !m.hasOption(CoapOption::BLOCK2)));
        CHECK(m.payload() == "{" + std::string(BLOCK_SIZE, 'a') + "}");
        // Acknowledge the request
        d.sendMessage(CoapMessage().type(CoapType::ACK).code(CoapCode::EMPTY).id(m.id()));
        CHECK(d.describeAckFlags() == DescriptionType::DESCRIBE_SYSTEM);
    }

    SECTION("can send a blockwise Describe request") {
        When(Method(cb, appendSystemInfo)).Do([](appender_fn append, void* arg, void* reserved) {
            auto s = std::string(PROTOCOL_BUFFER_SIZE, 'a');
            append(arg, (const uint8_t*)s.data(), s.size());
            return true;
        });
        // Send a request to the server
        d.get()->sendRequest(DescriptionType::DESCRIBE_SYSTEM);
        // Receive the first block request
        auto m = d.receiveMessage();
        CHECK(m.type() == CoapType::CON);
        CHECK(m.code() == CoapCode::POST);
        CHECK(m.option(CoapOption::URI_PATH).toString() == "d");
        CHECK(m.option(CoapOption::URI_QUERY).toUInt() == DescriptionType::DESCRIBE_SYSTEM);
        CHECK(m.option(CoapOption::BLOCK1).toUInt() == BlockOption().index(0).more(true));
        CHECK(m.payload() == "{" + std::string(BLOCK_SIZE - 1, 'a'));
        // The next block request must not be sent until the last sent request is acknowledged
        CHECK(!d.hasMessages());
        // Acknowledge the request
        d.sendMessage(CoapMessage().type(CoapType::ACK).code(CoapCode::EMPTY).id(m.id()));
        CHECK(d.describeAckFlags() == 0);
        // Receive the next block request
        m = d.receiveMessage();
        CHECK(m.type() == CoapType::CON);
        CHECK(m.code() == CoapCode::POST);
        CHECK(m.option(CoapOption::URI_PATH).toString() == "d");
        CHECK(m.option(CoapOption::URI_QUERY).toUInt() == DescriptionType::DESCRIBE_SYSTEM);
        CHECK(m.option(CoapOption::BLOCK1).toUInt() == BlockOption().index(1).more(false));
        CHECK(m.payload() == std::string(PROTOCOL_BUFFER_SIZE - BLOCK_SIZE + 1, 'a') + '}');
        // Acknowledge the request
        d.sendMessage(CoapMessage().type(CoapType::ACK).code(CoapCode::EMPTY).id(m.id()));
        CHECK(d.describeAckFlags() == DescriptionType::DESCRIBE_SYSTEM);
    }

    SECTION("can send a regular Describe response") {
        When(Method(cb, appendSystemInfo)).Do([](appender_fn append, void* arg, void* reserved) {
            auto s = std::string(BLOCK_SIZE, 'a');
            append(arg, (const uint8_t*)s.data(), s.size());
            return true;
        });
        // Send a request to the device
        d.sendRequest(DescriptionType::DESCRIBE_SYSTEM);
        // Receive the ACK
        auto m = d.receiveMessage();
        CHECK(m.type() == CoapType::ACK);
        CHECK(m.code() == CoapCode::EMPTY);
        CHECK(m.id() == d.lastMessageId());
        // Receive the separate response
        m = d.receiveMessage();
        CHECK(m.type() == CoapType::CON);
        CHECK(m.code() == CoapCode::CONTENT);
        CHECK(m.token() == d.lastMessageToken());
        CHECK((!m.hasOption(CoapOption::BLOCK1) && !m.hasOption(CoapOption::BLOCK2)));
        CHECK(m.payload() == "{" + std::string(BLOCK_SIZE, 'a') + "}");
        // Acknowledge the response
        d.sendMessage(CoapMessage().type(CoapType::ACK).code(CoapCode::EMPTY).id(m.id()));
        CHECK(d.describeAckFlags() == DescriptionType::DESCRIBE_SYSTEM);
    }

    SECTION("can send a blockwise Describe response") {
        When(Method(cb, appendSystemInfo)).Do([](appender_fn append, void* arg, void* reserved) {
            auto s = std::string(PROTOCOL_BUFFER_SIZE, 'a');
            append(arg, (const uint8_t*)s.data(), s.size());
            return true;
        });
        // Send a request to the device
        d.sendRequest(DescriptionType::DESCRIBE_SYSTEM);
        // Receive the ACK
        auto m = d.receiveMessage();
        CHECK(m.type() == CoapType::ACK);
        CHECK(m.code() == CoapCode::EMPTY);
        CHECK(m.id() == d.lastMessageId());
        // Receive the first block response
        m = d.receiveMessage();
        CHECK(m.type() == CoapType::CON);
        CHECK(m.code() == CoapCode::CONTENT);
        CHECK(m.token() == d.lastMessageToken());
        CHECK(m.option(CoapOption::ETAG).toString() == std::string("\x01", 1));
        CHECK(m.option(CoapOption::BLOCK2).toUInt() == BlockOption().index(0).more(true));
        CHECK(m.payload() == "{" + std::string(BLOCK_SIZE - 1, 'a'));
        // Acknowledge the response
        d.sendMessage(CoapMessage().type(CoapType::ACK).code(CoapCode::EMPTY).id(m.id()));
        CHECK(d.describeAckFlags() == 0);
        // The next block must not be sent without a request from the server
        CHECK(!d.hasMessages());
        // Request the next block
        d.sendRequest(DescriptionType::DESCRIBE_SYSTEM, BlockOption().index(1));
        // Receive the ACK
        m = d.receiveMessage();
        CHECK(m.type() == CoapType::ACK);
        CHECK(m.code() == CoapCode::EMPTY);
        CHECK(m.id() == d.lastMessageId());
        // Receive the next block response
        m = d.receiveMessage();
        CHECK(m.type() == CoapType::CON);
        CHECK(m.code() == CoapCode::CONTENT);
        CHECK(m.token() == d.lastMessageToken());
        CHECK(m.option(CoapOption::ETAG).toString() == std::string("\x01", 1));
        CHECK(m.option(CoapOption::BLOCK2).toUInt() == BlockOption().index(1).more(false));
        CHECK(m.payload() == std::string(PROTOCOL_BUFFER_SIZE - BLOCK_SIZE + 1, 'a') + '}');
        // Acknowledge the response
        d.sendMessage(CoapMessage().type(CoapType::ACK).code(CoapCode::EMPTY).id(m.id()));
        CHECK(d.describeAckFlags() == DescriptionType::DESCRIBE_SYSTEM);
    }

    SECTION("queues further Describe requests when a blockwise request is in progress") {
        When(Method(cb, appendSystemInfo)).Do([](appender_fn append, void* arg, void* reserved) {
            auto s = std::string(PROTOCOL_BUFFER_SIZE, 'a');
            append(arg, (const uint8_t*)s.data(), s.size());
            return true;
        });
        When(Method(cb, appendAppInfo)).Do([](appender_fn append, void* arg, void* reserved) {
            auto s = std::string(BLOCK_SIZE, 'b');
            append(arg, (const uint8_t*)s.data(), s.size());
            return true;
        });
        // Send a blockwise request to the server
        d.get()->sendRequest(DescriptionType::DESCRIBE_SYSTEM);
        // Receive the first block
        auto m = d.receiveMessage();
        CHECK(m.type() == CoapType::CON);
        CHECK(m.code() == CoapCode::POST);
        CHECK(m.option(CoapOption::URI_PATH).toString() == "d");
        CHECK(m.option(CoapOption::URI_QUERY).toUInt() == DescriptionType::DESCRIBE_SYSTEM);
        CHECK(m.option(CoapOption::BLOCK1).toUInt() == BlockOption().index(0).more(true));
        CHECK(m.payload() == "{" + std::string(BLOCK_SIZE - 1, 'a'));
        // Send another request to the server (a regular one)
        d.get()->sendRequest(DescriptionType::DESCRIBE_APPLICATION);
        // No new messages must be sent until the first block is acknowledged
        CHECK(!d.hasMessages());
        // Acknowledge the first block
        d.sendMessage(CoapMessage().type(CoapType::ACK).code(CoapCode::EMPTY).id(m.id()));
        CHECK(d.describeAckFlags() == 0);
        // Receive the second block
        m = d.receiveMessage();
        CHECK(m.type() == CoapType::CON);
        CHECK(m.code() == CoapCode::POST);
        CHECK(m.option(CoapOption::URI_PATH).toString() == "d");
        CHECK(m.option(CoapOption::URI_QUERY).toUInt() == DescriptionType::DESCRIBE_SYSTEM);
        CHECK(m.option(CoapOption::BLOCK1).toUInt() == BlockOption().index(1).more(false));
        CHECK(m.payload() == std::string(PROTOCOL_BUFFER_SIZE - BLOCK_SIZE + 1, 'a') + '}');
        // No new messages must be sent until the second block is acknowledged
        CHECK(!d.hasMessages());
        // Acknowledge the second block
        d.sendMessage(CoapMessage().type(CoapType::ACK).code(CoapCode::EMPTY).id(m.id()));
        CHECK(d.describeAckFlags() == DescriptionType::DESCRIBE_SYSTEM);
        // Receive the second request
        m = d.receiveMessage();
        CHECK(m.type() == CoapType::CON);
        CHECK(m.code() == CoapCode::POST);
        CHECK(m.option(CoapOption::URI_PATH).toString() == "d");
        CHECK(m.option(CoapOption::URI_QUERY).toUInt() == DescriptionType::DESCRIBE_APPLICATION);
        CHECK((!m.hasOption(CoapOption::BLOCK1) && !m.hasOption(CoapOption::BLOCK2)));
        CHECK(m.payload() == "{" + std::string(BLOCK_SIZE, 'b') + "}");
        // Acknowledge the second request
        d.sendMessage(CoapMessage().type(CoapType::ACK).code(CoapCode::EMPTY).id(m.id()));
        CHECK(d.describeAckFlags() == DescriptionType::DESCRIBE_APPLICATION);
    }

    SECTION("can send multiple blockwise responses concurrently") {
        When(Method(cb, appendSystemInfo)).Do([](appender_fn append, void* arg, void* reserved) {
            auto s = std::string(PROTOCOL_BUFFER_SIZE, 'a');
            append(arg, (const uint8_t*)s.data(), s.size());
            return true;
        });
        // Send two requests to the device
        d.sendRequest(DescriptionType::DESCRIBE_SYSTEM);
        auto id1 = d.lastMessageId();
        auto token1 = d.lastMessageToken();
        d.sendRequest(DescriptionType::DESCRIBE_SYSTEM);
        auto id2 = d.lastMessageId();
        auto token2 = d.lastMessageToken();
        CHECK(((id1 != id2) && (token1 != token2)));
        // Receive the ACK and response for the first request
        auto m = d.receiveMessage();
        CHECK(m.type() == CoapType::ACK);
        CHECK(m.code() == CoapCode::EMPTY);
        CHECK(m.id() == id1);
        m = d.receiveMessage();
        CHECK(m.type() == CoapType::CON);
        CHECK(m.code() == CoapCode::CONTENT);
        CHECK(m.token() == token1);
        CHECK(m.option(CoapOption::ETAG).toString() == std::string("\x01", 1));
        CHECK(m.option(CoapOption::BLOCK2).toUInt() == BlockOption().index(0).more(true));
        CHECK(m.payload() == "{" + std::string(BLOCK_SIZE - 1, 'a'));
        id1 = m.id();
        // Receive the ACK and response for the second request
        m = d.receiveMessage();
        CHECK(m.type() == CoapType::ACK);
        CHECK(m.code() == CoapCode::EMPTY);
        CHECK(m.id() == id2);
        m = d.receiveMessage();
        CHECK(m.type() == CoapType::CON);
        CHECK(m.code() == CoapCode::CONTENT);
        CHECK(m.token() == token2);
        CHECK(m.option(CoapOption::ETAG).toString() == std::string("\x01", 1)); // ETag is the same as in the other response
        CHECK(m.option(CoapOption::BLOCK2).toUInt() == BlockOption().index(0).more(true));
        CHECK(m.payload() == "{" + std::string(BLOCK_SIZE - 1, 'a'));
        id2 = m.id();
        // Acknowledge both the responses
        d.sendMessage(CoapMessage().type(CoapType::ACK).code(CoapCode::EMPTY).id(id1));
        CHECK(d.describeAckFlags() == 0);
        d.sendMessage(CoapMessage().type(CoapType::ACK).code(CoapCode::EMPTY).id(id2));
        CHECK(d.describeAckFlags() == 0);
        // Request the next block twice
        d.sendRequest(DescriptionType::DESCRIBE_SYSTEM, BlockOption().index(1));
        id1 = d.lastMessageId();
        token1 = d.lastMessageToken();
        d.sendRequest(DescriptionType::DESCRIBE_SYSTEM, BlockOption().index(1));
        id2 = d.lastMessageId();
        token2 = d.lastMessageToken();
        CHECK(((id1 != id2) && (token1 != token2)));
        // Receive the ACK and response for the first request
        m = d.receiveMessage();
        CHECK(m.type() == CoapType::ACK);
        CHECK(m.code() == CoapCode::EMPTY);
        CHECK(m.id() == id1);
        m = d.receiveMessage();
        CHECK(m.type() == CoapType::CON);
        CHECK(m.code() == CoapCode::CONTENT);
        CHECK(m.token() == token1);
        CHECK(m.option(CoapOption::ETAG).toString() == std::string("\x01", 1));
        CHECK(m.option(CoapOption::BLOCK2).toUInt() == BlockOption().index(1).more(false));
        CHECK(m.payload() == std::string(PROTOCOL_BUFFER_SIZE - BLOCK_SIZE + 1, 'a') + '}');
        id1 = m.id();
        // Receive the ACK and response for the second request
        m = d.receiveMessage();
        CHECK(m.type() == CoapType::ACK);
        CHECK(m.code() == CoapCode::EMPTY);
        CHECK(m.id() == id2);
        m = d.receiveMessage();
        CHECK(m.type() == CoapType::CON);
        CHECK(m.code() == CoapCode::CONTENT);
        CHECK(m.token() == token2);
        CHECK(m.option(CoapOption::ETAG).toString() == std::string("\x01", 1)); // ETag is the same as in the other response
        CHECK(m.option(CoapOption::BLOCK2).toUInt() == BlockOption().index(1).more(false));
        CHECK(m.payload() == std::string(PROTOCOL_BUFFER_SIZE - BLOCK_SIZE + 1, 'a') + '}');
        id2 = m.id();
        // Acknowledge both the responses
        d.sendMessage(CoapMessage().type(CoapType::ACK).code(CoapCode::EMPTY).id(id1));
        CHECK(d.describeAckFlags() == DescriptionType::DESCRIBE_SYSTEM);
        d.sendMessage(CoapMessage().type(CoapType::ACK).code(CoapCode::EMPTY).id(id2));
        CHECK(d.describeAckFlags() == DescriptionType::DESCRIBE_SYSTEM);
        // append_system_info() should have been called once
        Verify(Method(cb, appendSystemInfo)).Once();
    }

    SECTION("uses a different ETag with each Describe response") {
        When(Method(cb, appendSystemInfo)).AlwaysDo([&](appender_fn append, void* arg, void* reserved) {
            auto s = std::string(PROTOCOL_BUFFER_SIZE, 'a');
            append(arg, (const uint8_t*)s.data(), s.size());
            return true;
        });
        // Request the first block
        d.sendRequest(DescriptionType::DESCRIBE_SYSTEM);
        d.skipMessages(1); // ACK
        auto m = d.receiveMessage();
        CHECK(m.option(CoapOption::ETAG).toString() == std::string("\x01", 1));
        CHECK(m.option(CoapOption::BLOCK2).toUInt() == BlockOption().index(0).more(true));
        d.sendMessage(CoapMessage().type(CoapType::ACK).code(CoapCode::EMPTY).id(m.id()));
        // Request the second block
        d.sendRequest(DescriptionType::DESCRIBE_SYSTEM, BlockOption().index(1));
        d.skipMessages(1); // ACK
        m = d.receiveMessage();
        CHECK(m.option(CoapOption::ETAG).toString() == std::string("\x01", 1));
        CHECK(m.option(CoapOption::BLOCK2).toUInt() == BlockOption().index(1).more(false));
        d.sendMessage(CoapMessage().type(CoapType::ACK).code(CoapCode::EMPTY).id(m.id()));
        // Request the first block (append_system_info() will be called again)
        d.sendRequest(DescriptionType::DESCRIBE_SYSTEM);
        d.skipMessages(1); // ACK
        m = d.receiveMessage();
        CHECK(m.option(CoapOption::ETAG).toString() == std::string("\x02", 1));
        CHECK(m.option(CoapOption::BLOCK2).toUInt() == BlockOption().index(0).more(true));
        d.sendMessage(CoapMessage().type(CoapType::ACK).code(CoapCode::EMPTY).id(m.id()));
        // Request the second block
        d.sendRequest(DescriptionType::DESCRIBE_SYSTEM, BlockOption().index(1));
        d.skipMessages(1); // ACK
        m = d.receiveMessage();
        CHECK(m.option(CoapOption::ETAG).toString() == std::string("\x02", 1));
        CHECK(m.option(CoapOption::BLOCK2).toUInt() == BlockOption().index(1).more(false));
        d.sendMessage(CoapMessage().type(CoapType::ACK).code(CoapCode::EMPTY).id(m.id()));
        // append_system_info() should have been called twice
        Verify(Method(cb, appendSystemInfo)).Twice();
    }

    SECTION("invalidates cached Describe data after a timeout") {
        const auto payloadSize = BLOCK_SIZE * 4 - 100;
        When(Method(cb, appendSystemInfo)).AlwaysDo([=](appender_fn append, void* arg, void* reserved) {
            auto s = std::string(payloadSize, 'a');
            append(arg, (const uint8_t*)s.data(), s.size());
            return true;
        });
        // Request the first block
        d.sendRequest(DescriptionType::DESCRIBE_SYSTEM);
        d.skipMessages(1); // ACK
        auto m = d.receiveMessage();
        CHECK(m.option(CoapOption::ETAG).toString() == std::string("\x01", 1));
        CHECK(m.option(CoapOption::BLOCK2).toUInt() == BlockOption().index(0).more(true));
        d.sendMessage(CoapMessage().type(CoapType::ACK).code(CoapCode::EMPTY).id(m.id()));
        // Request the second block after a delay
        d.tick(COAP_BLOCKWISE_RESPONSE_TIMEOUT - 1);
        d.sendRequest(DescriptionType::DESCRIBE_SYSTEM, BlockOption().index(1));
        d.skipMessages(1); // ACK
        m = d.receiveMessage();
        CHECK(m.option(CoapOption::ETAG).toString() == std::string("\x01", 1));
        CHECK(m.option(CoapOption::BLOCK2).toUInt() == BlockOption().index(1).more(true));
        d.sendMessage(CoapMessage().type(CoapType::ACK).code(CoapCode::EMPTY).id(m.id()));
        // Cause a response timeout and request the first block again
        d.tick(COAP_BLOCKWISE_RESPONSE_TIMEOUT);
        d.sendRequest(DescriptionType::DESCRIBE_SYSTEM);
        d.skipMessages(1); // ACK
        m = d.receiveMessage();
        CHECK(m.option(CoapOption::ETAG).toString() == std::string("\x02", 1));
        CHECK(m.option(CoapOption::BLOCK2).toUInt() == BlockOption().index(0).more(true));
        d.sendMessage(CoapMessage().type(CoapType::ACK).code(CoapCode::EMPTY).id(m.id()));
    }

    SECTION("notifies the protocol layer when all client requests have been processed") {
        Mock<Protocol> proto(*d.protocol());
        When(Method(proto, notify_client_messages_processed)).AlwaysReturn();
        When(Method(cb, appendSystemInfo)).Do([](appender_fn append, void* arg, void* reserved) {
            auto s = std::string(PROTOCOL_BUFFER_SIZE, 'a');
            append(arg, (const uint8_t*)s.data(), s.size());
            return true;
        });
        When(Method(cb, appendAppInfo)).Do([](appender_fn append, void* arg, void* reserved) {
            auto s = std::string(BLOCK_SIZE, 'b');
            append(arg, (const uint8_t*)s.data(), s.size());
            return true;
        });
        CHECK(!d.get()->hasPendingClientRequests());
        // Send a blockwise request to the server
        d.get()->sendRequest(DescriptionType::DESCRIBE_SYSTEM);
        CHECK(d.get()->hasPendingClientRequests());
        // Receive and acknowledge the first block
        auto m = d.receiveMessage();
        d.sendMessage(CoapMessage().type(CoapType::ACK).code(CoapCode::EMPTY).id(m.id()));
        // Send another request to the server (a regular one)
        d.get()->sendRequest(DescriptionType::DESCRIBE_APPLICATION);
        // Receive the second block of the first request
        m = d.receiveMessage();
        CHECK(d.get()->hasPendingClientRequests());
        Verify(Method(proto, notify_client_messages_processed)).Never();
        // Acknowledge the second block
        d.sendMessage(CoapMessage().type(CoapType::ACK).code(CoapCode::EMPTY).id(m.id()));
        CHECK(!d.get()->hasPendingClientRequests());
        Verify(Method(proto, notify_client_messages_processed)).Once();
        // Receive and acknowledge the second request
        m = d.receiveMessage();
        d.sendMessage(CoapMessage().type(CoapType::ACK).code(CoapCode::EMPTY).id(m.id()));
    }
}
