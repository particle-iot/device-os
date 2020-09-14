/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#include "firmware_update.h"
#include "messages.h"
#include "sha256.h"

#include "util/coap_message_channel.h"
#include "util/protocol_callbacks.h"

#include <catch2/catch.hpp>
#include <fakeit.hpp>

#include <random>
#include <regex>

namespace {

using namespace particle;
using namespace particle::protocol;
using namespace particle::protocol::test;

using namespace fakeit;

// Protocol-specific CoAP options
enum OtaCoapOption {
    CHUNK_INDEX = 2049,
    WINDOW_SIZE = 2053,
    FILE_SIZE = 2057,
    FILE_SHA256 = 2061,
    CHUNK_SIZE = 2065,
    DISCARD_DATA = 2069,
    CANCEL_UPDATE = 2073
};

class FirmwareUpdateWrapper: public FirmwareUpdate {
public:
    FirmwareUpdateWrapper() :
            lastMsgId_(0),
            lastMsgToken_('a' - 1) {
        REQUIRE(init(&channel_, callbacks_.get()) >= 0);
    }

    ~FirmwareUpdateWrapper() {
        destroy();
    }

    // Sends an UpdateStart message to the device
    FirmwareUpdateWrapper& sendStart(size_t fileSize, const std::string& fileHash, size_t chunkSize, bool discardData) {
        CoapMessage m;
        m.type(CoapType::CON);
        m.code(CoapCode::POST);
        m.option(CoapOption::URI_PATH, "S");
        if (!fileHash.empty()) {
            m.option(OtaCoapOption::FILE_SHA256, fileHash);
        }
        m.option(OtaCoapOption::FILE_SIZE, fileSize);
        m.option(OtaCoapOption::CHUNK_SIZE, chunkSize);
        if (discardData) {
            m.emptyOption(OtaCoapOption::DISCARD_DATA);
        }
        return sendMessage(std::move(m));
    }

    // Sends an UpdateFinish message to the device
    FirmwareUpdateWrapper& sendFinish(bool cancelUpdate, bool discardData) {
        CoapMessage m;
        m.type(CoapType::CON);
        m.code(CoapCode::POST);
        m.option(CoapOption::URI_PATH, "F");
        if (cancelUpdate) {
            m.emptyOption(OtaCoapOption::CANCEL_UPDATE);
        }
        if (discardData) {
            m.emptyOption(OtaCoapOption::DISCARD_DATA);
        }
        return sendMessage(std::move(m));
    }

    // Sends an UpdateChunk message to the device
    FirmwareUpdateWrapper& sendChunk(unsigned index, const std::string& data) {
        CoapMessage m;
        m.type(CoapType::NON);
        m.code(CoapCode::POST);
        m.token(std::string());
        m.option(CoapOption::URI_PATH, "C");
        m.option(OtaCoapOption::CHUNK_INDEX, index);
        m.payload(data);
        return sendMessage(std::move(m));
    }

    // Sends a CoAP message to the device
    FirmwareUpdateWrapper& sendMessage(CoapMessage msg) {
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
            case CoAPMessageType::UPDATE_START_V3: {
                r = startRequest(&m);
                break;
            }
            case CoAPMessageType::UPDATE_FINISH_V3: {
                r = finishRequest(&m);
                break;
            }
            case CoAPMessageType::UPDATE_CHUNK_V3: {
                r = chunkRequest(&m);
                break;
            }
            case CoAPMessageType::EMPTY_ACK: {
                r = responseAck(&m);
                break;
            }
            default:
                break;
            }
        }
        if (r != ProtocolError::NO_ERROR) {
            throw std::runtime_error("Error while receiving message");
        }
        return *this;
    }

    // Receives a CoAP message from the device
    CoapMessage receiveMessage() {
        return channel_.receiveMessage();
    }

    // Skips N messages received from the device
    FirmwareUpdateWrapper& skipMessages(unsigned count) {
        channel_.skipMessages(count);
        return *this;
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

    FirmwareUpdateWrapper& addMillis(system_tick_t ms) {
        callbacks_.addMillis(ms);
        return *this;
    }

    FirmwareUpdateWrapper& processTimeouts() {
        CHECK(FirmwareUpdate::process() == ProtocolError::NO_ERROR);
        return *this;
    }

    Mock<CoapMessageChannel> channelMock() {
        return Mock<CoapMessageChannel>(channel_);
    }

    Mock<ProtocolCallbacks> callbacksMock() {
        return Mock<ProtocolCallbacks>(callbacks_);
    }

private:
    CoapMessageChannel channel_;
    ProtocolCallbacks callbacks_;
    CoapMessageId lastMsgId_;
    char lastMsgToken_;
};

std::default_random_engine& randomGen() {
    static thread_local std::default_random_engine gen((std::random_device())());
    return gen;
}

std::string genString(size_t size) {
    static const std::string alpha = "abcdefghijklmnopqrstuvwxyz";
    std::uniform_int_distribution<unsigned> dist(0, alpha.size() - 1);
    std::string s;
    s.reserve(size);
    for (size_t i = 0; i < size; ++i) {
        s += alpha.at(dist(randomGen()));
    }
    return s;
}

std::vector<unsigned> parseChunkAckPayload(const CoapMessage& msg) {
    const unsigned ackIndex = msg.option(OtaCoapOption::CHUNK_INDEX).toUInt();
    std::vector<unsigned> sackIndices;
    const auto& payload = msg.payload();
    size_t offs = 0;
    while (offs < payload.size()) {
        if (payload.size() - offs < 4) {
            throw std::runtime_error("Unexpected size of the payload data");
        }
        uint32_t w = 0;
        memcpy(&w, payload.data() + offs, 4);
        unsigned i = 1;
        while (w) {
            if (w & 1) {
                sackIndices.push_back(ackIndex + offs * 8 + i);
            }
            w >>= 1;
            ++i;
        }
        offs += 4;
    }
    return sackIndices;
}

bool hasDiagnosticPayload(const CoapMessage& msg) {
    static const std::regex rx1("^\\{\"code\":-?\\d+,\"message\":\".+\"\\}$");
    static const std::regex rx2("^\\{\"message\":\".+\",\"code\":-?\\d+\\}$");
    return msg.hasPayload() && (std::regex_match(msg.payload(), rx1) || std::regex_match(msg.payload(), rx2));
}

} // namespace

TEST_CASE("FirmwareUpdate") {
    FirmwareUpdateWrapper w;
    SECTION("starts an update when it receives an UpdateStart request") {
        CHECK(!w.isRunning());
        auto cb = w.callbacksMock();
        Spy(Method(cb, startFirmwareUpdate));
        SECTION("non-resumable update") {
            w.sendStart(1000 /* fileSize */, std::string() /* fileHash */, 512 /* chunkSize */, false /* discardData */);
            Verify(Method(cb, startFirmwareUpdate).Matching([=](size_t fileSize, const char* fileHash,
                    size_t* partialSize, unsigned flags) {
                return fileSize == 1000 && fileHash == nullptr && partialSize != nullptr &&
                        FirmwareUpdateFlags::fromUnderlying(flags) == FirmwareUpdateFlag::NON_RESUMABLE;
            })).Once();
            CHECK(w.isRunning());
        }
        SECTION("resumable update") {
            auto h = genString(Sha256::HASH_SIZE);
            w.sendStart(1000 /* fileSize */, h /* fileHash */, 512 /* chunkSize */, false /* discardData */);
            Verify(Method(cb, startFirmwareUpdate).Matching([=](size_t fileSize, const char* fileHash,
                    size_t* partialSize, unsigned flags) {
                return fileSize == 1000 && std::string(fileHash, Sha256::HASH_SIZE) == h && partialSize != nullptr &&
                        flags == 0;
            })).Once();
            CHECK(w.isRunning());
        }
        SECTION("discarding previously received data") {
            auto h = genString(Sha256::HASH_SIZE);
            w.sendStart(1000 /* fileSize */, h /* fileHash */, 512 /* chunkSize */, true /* discardData */);
            Verify(Method(cb, startFirmwareUpdate).Matching([=](size_t fileSize, const char* fileHash,
                    size_t* partialSize, unsigned flags) {
                return fileSize == 1000 && std::string(fileHash, Sha256::HASH_SIZE) == h && partialSize != nullptr &&
                        FirmwareUpdateFlags::fromUnderlying(flags) == FirmwareUpdateFlag::DISCARD_DATA;
            })).Once();
            CHECK(w.isRunning());
        }
    }
    SECTION("replies to the server with an UpdateStart response") {
        SECTION("non-resumable update") {
            w.sendStart(1000 /* fileSize */, std::string() /* fileHash */, 512 /* chunkSize */, false /* discardData */);
            // Empty ACK
            auto m = w.receiveMessage();
            CHECK(m.type() == CoapType::ACK);
            CHECK(m.code() == CoapCode::EMPTY);
            CHECK(m.id() == w.lastMessageId());
            // Separate response
            m = w.receiveMessage();
            CHECK(m.type() == CoapType::CON);
            CHECK(m.code() == CoapCode::CREATED);
            CHECK(m.token() == w.lastMessageToken());
            CHECK(m.option(OtaCoapOption::WINDOW_SIZE).toUInt() == OTA_RECEIVE_WINDOW_SIZE / 512 /* Chunk size */);
            CHECK((!m.hasOption(OtaCoapOption::FILE_SIZE) || m.option(OtaCoapOption::FILE_SIZE).toUInt() == 0));
            CHECK(!m.hasPayload());
        }
        SECTION("resumable update") {
            auto cb = w.callbacksMock();
            When(Method(cb, startFirmwareUpdate)).Do([](size_t fileSize, const char* fileHash, size_t* partialSize,
                    unsigned flags) {
                *partialSize = 200;
                return 0;
            });
            w.sendStart(1000 /* fileSize */, genString(Sha256::HASH_SIZE) /* fileHash */, 512 /* chunkSize */, false /* discardData */);
            w.skipMessages(1); // Skip the ACK
            auto m = w.receiveMessage();
            CHECK(m.type() == CoapType::CON);
            CHECK(m.code() == CoapCode::CREATED);
            CHECK(m.token() == w.lastMessageToken());
            CHECK(m.option(OtaCoapOption::WINDOW_SIZE).toUInt() == OTA_RECEIVE_WINDOW_SIZE / 512 /* Chunk size */);
            CHECK(m.option(OtaCoapOption::FILE_SIZE).toUInt() == 200);
            CHECK(!m.hasPayload());
        }
        SECTION("error white starting the update") {
            auto cb = w.callbacksMock();
            When(Method(cb, startFirmwareUpdate)).Do([](size_t fileSize, const char* fileHash, size_t* partialSize,
                    unsigned flags) {
                ERROR_MESSAGE("YOU SHALL NOT PASS!");
                return SYSTEM_ERROR_NOT_ALLOWED;
            });
            w.sendStart(1000 /* fileSize */, std::string() /* fileHash */, 512 /* chunkSize */, false /* discardData */);
            w.skipMessages(1); // Skip the ACK
            auto m = w.receiveMessage();
            CHECK(m.type() == CoapType::CON);
            CHECK((isCoapResponseCode(m.code()) && !isCoapSuccessCode(m.code())));
            CHECK(m.token() == w.lastMessageToken());
            CHECK(hasDiagnosticPayload(m));
            CHECK(m.payload().find("YOU SHALL NOT PASS!") != std::string::npos);
            CHECK(m.payload().find(std::to_string(SYSTEM_ERROR_NOT_ALLOWED)) != std::string::npos);
            CHECK(!w.isRunning());
        }
        SECTION("invalid request") {
            CoapMessage req;
            SECTION("unexpected message type") {
                req.type(CoapType::NON);
                req.code(CoapCode::POST);
                req.option(CoapOption::URI_PATH, "S");
                req.option(OtaCoapOption::FILE_SIZE, 1000);
                req.option(OtaCoapOption::CHUNK_SIZE, 512);
                w.sendMessage(req);
                auto resp = w.receiveMessage();
                CHECK(resp.type() == CoapType::NON);
                CHECK((isCoapResponseCode(resp.code()) && !isCoapSuccessCode(resp.code())));
                CHECK(resp.token() == w.lastMessageToken());
                CHECK(hasDiagnosticPayload(resp));
                CHECK(!w.isRunning());
            }
            SECTION("unexpected request method") {
                req.type(CoapType::CON);
                req.code(CoapCode::PUT);
                req.option(CoapOption::URI_PATH, "S");
                req.option(OtaCoapOption::FILE_SIZE, 1000);
                req.option(OtaCoapOption::CHUNK_SIZE, 512);
                w.sendMessage(req);
                // FIXME: The Protocol class silently ignores messages with an unexpected request method
                CHECK(!w.hasMessages());
                CHECK(!w.isRunning());
            }
            SECTION("missing token") {
                req.type(CoapType::CON);
                req.code(CoapCode::POST);
                req.token(std::string());
                req.option(CoapOption::URI_PATH, "S");
                req.option(OtaCoapOption::FILE_SIZE, 1000);
                req.option(OtaCoapOption::CHUNK_SIZE, 512);
                w.sendMessage(req);
                auto resp = w.receiveMessage();
                CHECK(resp.type() == CoapType::ACK);
                CHECK((isCoapResponseCode(resp.code()) && !isCoapSuccessCode(resp.code())));
                CHECK(hasDiagnosticPayload(resp));
                CHECK(!w.isRunning());
            }
            SECTION("invalid options") {
                req.type(CoapType::CON);
                req.code(CoapCode::POST);
                req.option(CoapOption::URI_PATH, "S");
                SECTION("missing File-Size") {
                    req.option(OtaCoapOption::CHUNK_SIZE, 512);
                    w.sendMessage(req);
                    auto resp = w.receiveMessage();
                    CHECK(resp.type() == CoapType::ACK);
                    CHECK((isCoapResponseCode(resp.code()) && !isCoapSuccessCode(resp.code())));
                    CHECK(resp.token() == w.lastMessageToken());
                    CHECK(hasDiagnosticPayload(resp));
                    CHECK(!w.isRunning());
                }
                SECTION("File-Size is zero") {
                    req.option(OtaCoapOption::FILE_SIZE, 0);
                    w.sendMessage(req);
                    auto resp = w.receiveMessage();
                    CHECK(resp.type() == CoapType::ACK);
                    CHECK((isCoapResponseCode(resp.code()) && !isCoapSuccessCode(resp.code())));
                    CHECK(resp.token() == w.lastMessageToken());
                    CHECK(hasDiagnosticPayload(resp));
                    CHECK(!w.isRunning());
                }
                SECTION("missing Chunk-Size") {
                    req.option(OtaCoapOption::FILE_SIZE, 1000);
                    w.sendMessage(req);
                    auto resp = w.receiveMessage();
                    CHECK(resp.type() == CoapType::ACK);
                    CHECK((isCoapResponseCode(resp.code()) && !isCoapSuccessCode(resp.code())));
                    CHECK(resp.token() == w.lastMessageToken());
                    CHECK(hasDiagnosticPayload(resp));
                    CHECK(!w.isRunning());
                }
                SECTION("too small Chunk-Size") {
                    req.option(OtaCoapOption::FILE_SIZE, 1000);
                    req.option(OtaCoapOption::CHUNK_SIZE, MIN_OTA_CHUNK_SIZE - 1);
                    w.sendMessage(req);
                    auto resp = w.receiveMessage();
                    CHECK(resp.type() == CoapType::ACK);
                    CHECK((isCoapResponseCode(resp.code()) && !isCoapSuccessCode(resp.code())));
                    CHECK(resp.token() == w.lastMessageToken());
                    CHECK(hasDiagnosticPayload(resp));
                    CHECK(!w.isRunning());
                }
                SECTION("too large Chunk-Size") {
                    req.option(OtaCoapOption::FILE_SIZE, 1000);
                    req.option(OtaCoapOption::CHUNK_SIZE, MAX_OTA_CHUNK_SIZE + 1);
                    w.sendMessage(req);
                    auto resp = w.receiveMessage();
                    CHECK(resp.type() == CoapType::ACK);
                    CHECK((isCoapResponseCode(resp.code()) && !isCoapSuccessCode(resp.code())));
                    CHECK(resp.token() == w.lastMessageToken());
                    CHECK(hasDiagnosticPayload(resp));
                    CHECK(!w.isRunning());
                }
                SECTION("invalid size of File-Sha-256") {
                    req.option(OtaCoapOption::FILE_SIZE, 1000);
                    req.option(OtaCoapOption::FILE_SHA256, genString(Sha256::HASH_SIZE + 1));
                    req.option(OtaCoapOption::CHUNK_SIZE, 512);
                    w.sendMessage(req);
                    auto resp = w.receiveMessage();
                    CHECK(resp.type() == CoapType::ACK);
                    CHECK((isCoapResponseCode(resp.code()) && !isCoapSuccessCode(resp.code())));
                    CHECK(resp.token() == w.lastMessageToken());
                    CHECK(hasDiagnosticPayload(resp));
                    CHECK(!w.isRunning());
                }
                SECTION("non-empty Discard-Data") {
                    req.option(OtaCoapOption::FILE_SIZE, 1000);
                    req.option(OtaCoapOption::CHUNK_SIZE, 512);
                    req.option(OtaCoapOption::DISCARD_DATA, 1);
                    w.sendMessage(req);
                    auto resp = w.receiveMessage();
                    CHECK(resp.type() == CoapType::ACK);
                    CHECK((isCoapResponseCode(resp.code()) && !isCoapSuccessCode(resp.code())));
                    CHECK(resp.token() == w.lastMessageToken());
                    CHECK(hasDiagnosticPayload(resp));
                    CHECK(!w.isRunning());
                }
            }
        }
    }
    SECTION("saves the data received in an UpdateChunk request") {
        auto cb = w.callbacksMock();
        std::vector<std::string> recvChunks;
        When(Method(cb, saveFirmwareChunk)).AlwaysDo([&](const char* chunkData, size_t chunkSize, size_t chunkOffset,
                size_t partialSize) {
            recvChunks.push_back(std::string(chunkData, chunkSize));
            return 0;
        });
        w.sendStart(2000 /* fileSize */, std::string() /* fileHash */, 512 /* chunkSize */, false /* discardData */);
        auto chunk1 = genString(512);
        w.sendChunk(1 /* index */, chunk1 /* data */);
        auto chunk2 = genString(512);
        w.sendChunk(2 /* index */, chunk2 /* data */);
        auto chunk3 = genString(512);
        w.sendChunk(3 /* index */, chunk3 /* data */);
        auto chunk4 = genString(464);
        w.sendChunk(4 /* index */, chunk4 /* data */);
        Verify(Method(cb, saveFirmwareChunk).Matching([=](const char* chunkData, size_t chunkSize, size_t chunkOffset,
                size_t partialSize) {
            return recvChunks.at(0) == chunk1 && chunkOffset == 0 && partialSize == 512;
        }) + Method(cb, saveFirmwareChunk).Matching([=](const char* chunkData, size_t chunkSize, size_t chunkOffset,
                size_t partialSize) {
            return recvChunks.at(1) == chunk2 && chunkOffset == 512 && partialSize == 1024;
        }) + Method(cb, saveFirmwareChunk).Matching([=](const char* chunkData, size_t chunkSize, size_t chunkOffset,
                size_t partialSize) {
            return recvChunks.at(2) == chunk3 && chunkOffset == 1024 && partialSize == 1536;
        }) + Method(cb, saveFirmwareChunk).Matching([=](const char* chunkData, size_t chunkSize, size_t chunkOffset,
                size_t partialSize) {
            return recvChunks.at(3) == chunk4 && chunkOffset == 1536 && partialSize == 2000;
        })).Once();
    }
    SECTION("doesn't attempt to save the data received in a duplicate UpdateChunk request") {
        auto cb = w.callbacksMock();
        Spy(Method(cb, saveFirmwareChunk));
        w.sendStart(1024 /* fileSize */, std::string() /* fileHash */, 512 /* chunkSize */, false /* discardData */);
        auto chunk1 = genString(512);
        w.sendChunk(1 /* index */, chunk1 /* data */);
        CHECK(w.stats().duplicateChunks == 0);
        w.sendChunk(1 /* index */, chunk1 /* data */);
        CHECK(w.stats().duplicateChunks == 1);
        Verify(Method(cb, saveFirmwareChunk).Matching([=](const char* chunkData, size_t chunkSize, size_t chunkOffset,
                size_t partialSize) {
            return chunkOffset == 0 && partialSize == 512;
        })).Once();
        VerifyNoOtherInvocations(Method(cb, saveFirmwareChunk));
    }
    SECTION("acknowledges every second chunk normally") {
        w.sendStart(2560 /* fileSize */, std::string() /* fileHash */, 512 /* chunkSize */, false /* discardData */);
        w.skipMessages(2); // Skip the ACK and response
        // Chunk 1
        w.sendChunk(1 /* index */, genString(512) /* data */);
        CHECK(!w.hasMessages()); // ACK delayed
        // Chunk 2
        w.sendChunk(2 /* index */, genString(512) /* data */);
        auto m = w.receiveMessage();
        CHECK(m.type() == CoapType::NON);
        CHECK(m.code() == CoapCode::POST);
        CHECK(!m.hasToken());
        CHECK(m.option(CoapOption::URI_PATH).toString() == "A");
        CHECK(m.option(OtaCoapOption::CHUNK_INDEX).toUInt() == 2);
        CHECK(!m.hasPayload()); // No gaps
        // Chunk 3
        w.sendChunk(3 /* index */, genString(512) /* data */);
        CHECK(!w.hasMessages()); // ACK delayed
        // Chunk 4
        w.sendChunk(4 /* index */, genString(512) /* data */);
        m = w.receiveMessage();
        CHECK(m.type() == CoapType::NON);
        CHECK(m.code() == CoapCode::POST);
        CHECK(!m.hasToken());
        CHECK(m.option(CoapOption::URI_PATH).toString() == "A");
        CHECK(m.option(OtaCoapOption::CHUNK_INDEX).toUInt() == 4);
        CHECK(!m.hasPayload()); // No gaps
    }
    SECTION("acknowledges the last received chunk after a delay") {
        w.sendStart(1024 /* fileSize */, std::string() /* fileHash */, 512 /* chunkSize */, false /* discardData */);
        w.skipMessages(2); // Skip the ACK and response
        w.sendChunk(1 /* index */, genString(512) /* data */);
        w.processTimeouts();
        CHECK(!w.hasMessages());
        w.addMillis(OTA_CHUNK_ACK_DELAY);
        w.processTimeouts();
        auto m = w.receiveMessage();
        CHECK(m.type() == CoapType::NON);
        CHECK(m.code() == CoapCode::POST);
        CHECK(!m.hasToken());
        CHECK(m.option(CoapOption::URI_PATH).toString() == "A");
        CHECK(m.option(OtaCoapOption::CHUNK_INDEX).toUInt() == 1);
        CHECK(!m.hasPayload()); // No gaps
    }
    SECTION("always acknowledges the last chunk of the file data") {
        w.sendStart(1536 /* fileSize */, std::string() /* fileHash */, 512 /* chunkSize */, false /* discardData */);
        w.skipMessages(2); // Skip the ACK and response
        // Chunk 1
        w.sendChunk(1 /* index */, genString(512) /* data */);
        CHECK(!w.hasMessages()); // ACK delayed
        // Chunk 2
        w.sendChunk(2 /* index */, genString(512) /* data */);
        auto m = w.receiveMessage();
        CHECK(m.option(OtaCoapOption::CHUNK_INDEX).toUInt() == 2);
        CHECK(!m.hasPayload()); // No gaps
        // Chunk 3
        w.sendChunk(3 /* index */, genString(512) /* data */);
        m = w.receiveMessage();
        CHECK(m.option(OtaCoapOption::CHUNK_INDEX).toUInt() == 3);
        CHECK(!m.hasPayload()); // No gaps
    }
    SECTION("acknowledges chunks selectively if there's a gap in the sequence of received chunks") {
        w.sendStart(2048 /* fileSize */, std::string() /* fileHash */, 512 /* chunkSize */, false /* discardData */);
        w.skipMessages(2); // Skip the ACK and response
        // Chunk 2
        w.sendChunk(2 /* index */, genString(512) /* data */);
        auto m = w.receiveMessage();
        CHECK(m.option(OtaCoapOption::CHUNK_INDEX).toUInt() == 0);
        CHECK(parseChunkAckPayload(m) == std::vector<unsigned>{ 2 });
        // Chunk 4
        w.sendChunk(4 /* index */, genString(512) /* data */);
        m = w.receiveMessage();
        CHECK(m.option(OtaCoapOption::CHUNK_INDEX).toUInt() == 0);
        CHECK(parseChunkAckPayload(m) == std::vector<unsigned>{ 2, 4 });
    }
    SECTION("acknowledges every received chunk until a gap in the sequence of received chunks is fixed") {
        w.sendStart(4096 /* fileSize */, std::string() /* fileHash */, 512 /* chunkSize */, false /* discardData */);
        w.skipMessages(2); // Skip the ACK and response
        // Chunk 2
        w.sendChunk(2 /* index */, genString(512) /* data */);
        auto m = w.receiveMessage();
        CHECK(m.option(OtaCoapOption::CHUNK_INDEX).toUInt() == 0);
        CHECK(parseChunkAckPayload(m) == std::vector<unsigned>{ 2 });
        // Chunk 3
        w.sendChunk(3 /* index */, genString(512) /* data */);
        m = w.receiveMessage();
        CHECK(m.option(OtaCoapOption::CHUNK_INDEX).toUInt() == 0);
        CHECK(parseChunkAckPayload(m) == std::vector<unsigned>{ 2, 3 });
        // Chunk 1
        w.sendChunk(1 /* index */, genString(512) /* data */);
        m = w.receiveMessage();
        CHECK(m.option(OtaCoapOption::CHUNK_INDEX).toUInt() == 3);
        CHECK(!m.hasPayload()); // No gaps
        // Chunk 4
        w.sendChunk(4 /* index */, genString(512) /* data */);
        CHECK(!w.hasMessages()); // ACK delayed
    }
    SECTION("always acknowledges a duplicate chunk") {
        w.sendStart(4096 /* fileSize */, std::string() /* fileHash */, 512 /* chunkSize */, false /* discardData */);
        w.skipMessages(2); // Skip the ACK and response
        // Chunk 1
        w.sendChunk(1 /* index */, genString(512) /* data */);
        CHECK(!w.hasMessages()); // ACK delayed
        // Chunk 2
        auto d = genString(512);
        w.sendChunk(2 /* index */, d /* data */);
        auto m = w.receiveMessage();
        CHECK(m.option(OtaCoapOption::CHUNK_INDEX).toUInt() == 2);
        // Chunk 2
        w.sendChunk(2 /* index */, d /* data */);
        m = w.receiveMessage();
        CHECK(m.option(OtaCoapOption::CHUNK_INDEX).toUInt() == 2);
    }
    SECTION("replies to the server with an error response if an UpdateChunk request cannot be processed") {
        SECTION("no update is in progress") {
            w.sendChunk(1 /* index */, genString(512) /* data */);
            auto resp = w.receiveMessage();
            CHECK(resp.type() == CoapType::RST);
            CHECK(resp.id() == w.lastMessageId());
        }
        SECTION("error while saving chunk") {
            auto cb = w.callbacksMock();
            When(Method(cb, saveFirmwareChunk)).Do([&](const char* chunkData, size_t chunkSize, size_t chunkOffset,
                    size_t partialSize) {
                return SYSTEM_ERROR_FLASH_IO;
            });
            w.sendStart(1024 /* fileSize */, std::string() /* fileHash */, 512 /* chunkSize */, false /* discardData */);
            w.skipMessages(2); // Skip the ACK and response
            w.sendChunk(1 /* index */, genString(512) /* data */);
            auto resp = w.receiveMessage();
            CHECK(resp.type() == CoapType::RST);
            CHECK(resp.id() == w.lastMessageId());
            CHECK(!w.isRunning());
        }
        SECTION("invalid request") {
            w.sendStart(1024 /* fileSize */, std::string() /* fileHash */, 512 /* chunkSize */, false /* discardData */);
            w.skipMessages(2); // Skip the ACK and response
            CoapMessage req;
            SECTION("unexpected message type") {
                req.type(CoapType::CON);
                req.code(CoapCode::POST);
                req.token(std::string());
                req.option(CoapOption::URI_PATH, "C");
                req.option(OtaCoapOption::CHUNK_INDEX, 1);
                req.payload(genString(512));
                w.sendMessage(req);
                auto resp = w.receiveMessage();
                CHECK(resp.type() == CoapType::ACK);
                CHECK((isCoapResponseCode(resp.code()) && !isCoapSuccessCode(resp.code())));
                CHECK(!resp.hasToken());
                CHECK(hasDiagnosticPayload(resp));
            }
            SECTION("unexpected request method") {
                req.type(CoapType::NON);
                req.code(CoapCode::PUT);
                req.token(std::string());
                req.option(CoapOption::URI_PATH, "C");
                req.option(OtaCoapOption::CHUNK_INDEX, 1);
                req.payload(genString(512));
                w.sendMessage(req);
                // FIXME: The Protocol class silently ignores messages with an unexpected request method
                CHECK(!w.hasMessages());
            }
            SECTION("non-empty token") {
                req.type(CoapType::NON);
                req.code(CoapCode::POST);
                req.token("a");
                req.option(CoapOption::URI_PATH, "C");
                req.option(OtaCoapOption::CHUNK_INDEX, 1);
                req.payload(genString(512));
                w.sendMessage(req);
                auto resp = w.receiveMessage();
                CHECK(resp.type() == CoapType::NON);
                CHECK((isCoapResponseCode(resp.code()) && !isCoapSuccessCode(resp.code())));
                CHECK(resp.token() == w.lastMessageToken());
                CHECK(hasDiagnosticPayload(resp));
            }
            SECTION("Chunk-Index is missing") {
                req.type(CoapType::NON);
                req.code(CoapCode::POST);
                req.token(std::string());
                req.option(CoapOption::URI_PATH, "C");
                req.payload(genString(512));
                w.sendMessage(req);
                auto resp = w.receiveMessage();
                CHECK(resp.type() == CoapType::RST);
                CHECK(resp.id() == w.lastMessageId());
            }
            SECTION("Chunk-Index is zero") {
                w.sendChunk(0 /* index */, genString(512) /* data */);
                auto resp = w.receiveMessage();
                CHECK(resp.type() == CoapType::RST);
                CHECK(resp.id() == w.lastMessageId());
            }
            SECTION("Chunk-Index is greater than the total number of chunks") {
                w.sendChunk(3 /* index */, genString(512) /* data */);
                auto resp = w.receiveMessage();
                CHECK(resp.type() == CoapType::RST);
                CHECK(resp.id() == w.lastMessageId());
            }
            SECTION("invalid payload size") {
                w.sendChunk(1 /* index */, genString(511) /* data */);
                auto resp = w.receiveMessage();
                CHECK(resp.type() == CoapType::RST);
                CHECK(resp.id() == w.lastMessageId());
            }
        }
    }
    SECTION("fails if the first chunk is not received within the timeout") {
        auto cb = w.callbacksMock();
        Spy(Method(cb, finishFirmwareUpdate));
        w.sendStart(1024 /* fileSize */, std::string() /* fileHash */, 512 /* chunkSize */, false /* discardData */);
        w.skipMessages(2); // Skip the ACK and response
        w.addMillis(OTA_TRANSFER_TIMEOUT);
        CHECK(w.process() == ProtocolError::MESSAGE_TIMEOUT);
        Verify(Method(cb, finishFirmwareUpdate).Matching([=](unsigned flags) {
            return FirmwareUpdateFlags::fromUnderlying(flags) == FirmwareUpdateFlag::CANCEL;
        })).Once();
        CHECK(!w.isRunning());
    }
    SECTION("fails if the next chunk is not received within the timeout") {
        auto cb = w.callbacksMock();
        Spy(Method(cb, finishFirmwareUpdate));
        w.sendStart(1024 /* fileSize */, std::string() /* fileHash */, 512 /* chunkSize */, false /* discardData */);
        w.skipMessages(2); // Skip the ACK and response
        // Chunk 1
        w.sendChunk(1 /* index */, genString(512) /* data */);
        CHECK(!w.hasMessages()); // ACK delayed
        w.addMillis(OTA_TRANSFER_TIMEOUT);
        CHECK(w.process() == ProtocolError::MESSAGE_TIMEOUT);
        Verify(Method(cb, finishFirmwareUpdate).Matching([=](unsigned flags) {
            return FirmwareUpdateFlags::fromUnderlying(flags) == FirmwareUpdateFlag::CANCEL;
        })).Once();
        CHECK(!w.isRunning());
    }
    SECTION("finishes the update when it receives an UpdateFinish request") {
        auto cb = w.callbacksMock();
        Spy(Method(cb, finishFirmwareUpdate));
        w.sendStart(512 /* fileSize */, std::string() /* fileHash */, 512 /* chunkSize */, false /* discardData */);
        w.skipMessages(2); // Skip the ACK and response
        SECTION("successfully finishing an update") {
            w.sendChunk(1 /* index */, genString(512) /* data */);
            w.skipMessages(1); // Skip the UpdateAck
            w.sendFinish(false /* cancelUpdate */, false /* discardData */);
            // The handler should validate the update and send an UpdateFinish response
            Verify(Method(cb, finishFirmwareUpdate).Matching([=](unsigned flags) {
                return FirmwareUpdateFlags::fromUnderlying(flags) == FirmwareUpdateFlag::VALIDATE_ONLY;
            })).Once();
            VerifyNoOtherInvocations(Method(cb, finishFirmwareUpdate));
            cb.ClearInvocationHistory();
            CHECK(w.isRunning());
            // Empty ACK
            auto m = w.receiveMessage();
            CHECK(m.type() == CoapType::ACK);
            CHECK(m.code() == CoapCode::EMPTY);
            CHECK(m.id() == w.lastMessageId());
            // Separate response
            m = w.receiveMessage();
            CHECK(m.type() == CoapType::CON);
            CHECK((isCoapResponseCode(m.code()) && isCoapSuccessCode(m.code())));
            CHECK(m.token() == w.lastMessageToken());
            // Acknowledge the response
            w.sendMessage(CoapMessage().type(CoapType::ACK).code(CoapCode::EMPTY).id(m.id()));
            // The handler should apply the update
            Verify(Method(cb, finishFirmwareUpdate).Matching([=](unsigned flags) {
                return flags == 0;
            }));
            VerifyNoOtherInvocations(Method(cb, finishFirmwareUpdate));
            CHECK(!w.isRunning());
        }
        SECTION("finishing an incomplete update") {
            w.sendFinish(false /* cancelUpdate */, false /* discardData */);
            Verify(Method(cb, finishFirmwareUpdate).Matching([=](unsigned flags) {
                return FirmwareUpdateFlags::fromUnderlying(flags) == FirmwareUpdateFlag::CANCEL;
            })).Once();
            VerifyNoOtherInvocations(Method(cb, finishFirmwareUpdate));
            CHECK(!w.isRunning());
            // Empty ACK
            auto m = w.receiveMessage();
            CHECK(m.type() == CoapType::ACK);
            CHECK(m.code() == CoapCode::EMPTY);
            CHECK(m.id() == w.lastMessageId());
            // Separate response
            m = w.receiveMessage();
            CHECK(m.type() == CoapType::CON);
            CHECK((isCoapResponseCode(m.code()) && !isCoapSuccessCode(m.code())));
            CHECK(m.token() == w.lastMessageToken());
            CHECK(hasDiagnosticPayload(m));
        }
        SECTION("failed validation") {
            When(Method(cb, finishFirmwareUpdate)).AlwaysDo([=](unsigned flags) {
                ERROR_MESSAGE("YOU SHALL NOT PASS!");
                return SYSTEM_ERROR_NOT_ALLOWED;
            });
            w.sendChunk(1 /* index */, genString(512) /* data */);
            w.skipMessages(1); // Skip the UpdateAck
            w.sendFinish(false /* cancelUpdate */, false /* discardData */);
            w.skipMessages(1); // Skip the ACK
            auto m = w.receiveMessage();
            CHECK(m.type() == CoapType::CON);
            CHECK((isCoapResponseCode(m.code()) && !isCoapSuccessCode(m.code())));
            CHECK(m.token() == w.lastMessageToken());
            CHECK(hasDiagnosticPayload(m));
            CHECK(m.payload().find("YOU SHALL NOT PASS!") != std::string::npos);
            CHECK(m.payload().find(std::to_string(SYSTEM_ERROR_NOT_ALLOWED)) != std::string::npos);
            CHECK(!w.isRunning());
        }
        SECTION("cancelling an update") {
            w.sendFinish(true /* cancelUpdate */, false /* discardData */);
            Verify(Method(cb, finishFirmwareUpdate).Matching([=](unsigned flags) {
                return FirmwareUpdateFlags::fromUnderlying(flags) == FirmwareUpdateFlag::CANCEL;
            })).Once();
            VerifyNoOtherInvocations(Method(cb, finishFirmwareUpdate));
            CHECK(!w.isRunning());
        }
        SECTION("cancelling an update and discarding transferred data") {
            w.sendFinish(true /* cancelUpdate */, true /* discardData */);
            Verify(Method(cb, finishFirmwareUpdate).Matching([=](unsigned flags) {
                return FirmwareUpdateFlags::fromUnderlying(flags) == (FirmwareUpdateFlag::CANCEL | FirmwareUpdateFlag::DISCARD_DATA);
            })).Once();
            VerifyNoOtherInvocations(Method(cb, finishFirmwareUpdate));
            CHECK(!w.isRunning());
        }
        SECTION("invalid request") {
            w.sendChunk(1 /* index */, genString(512) /* data */);
            w.skipMessages(1); // Skip the UpdateAck
            CoapMessage req;
            SECTION("unexpected message type") {
                req.type(CoapType::NON);
                req.code(CoapCode::POST);
                req.option(CoapOption::URI_PATH, "F");
                w.sendMessage(req);
                auto resp = w.receiveMessage();
                CHECK(resp.type() == CoapType::NON);
                CHECK((isCoapResponseCode(resp.code()) && !isCoapSuccessCode(resp.code())));
                CHECK(resp.token() == w.lastMessageToken());
                CHECK(hasDiagnosticPayload(resp));
            }
            SECTION("unexpected request method") {
                req.type(CoapType::CON);
                req.code(CoapCode::PUT);
                req.option(CoapOption::URI_PATH, "F");
                w.sendMessage(req);
                // FIXME: The Protocol class silently ignores messages with an unexpected request method
                CHECK(!w.hasMessages());
            }
            SECTION("missing token") {
                req.type(CoapType::CON);
                req.code(CoapCode::POST);
                req.token(std::string());
                req.option(CoapOption::URI_PATH, "F");
                w.sendMessage(req);
                auto resp = w.receiveMessage();
                CHECK(resp.type() == CoapType::ACK);
                CHECK((isCoapResponseCode(resp.code()) && !isCoapSuccessCode(resp.code())));
                CHECK(hasDiagnosticPayload(resp));
            }
            SECTION("invalid options") {
                req.type(CoapType::CON);
                req.code(CoapCode::POST);
                req.option(CoapOption::URI_PATH, "F");
                SECTION("non-empty Cancel-Update") {
                    req.option(OtaCoapOption::CANCEL_UPDATE, 1);
                    w.sendMessage(req);
                    auto resp = w.receiveMessage();
                    CHECK(resp.type() == CoapType::ACK);
                    CHECK((isCoapResponseCode(resp.code()) && !isCoapSuccessCode(resp.code())));
                    CHECK(resp.token() == w.lastMessageToken());
                    CHECK(hasDiagnosticPayload(resp));
                }
                SECTION("non-empty Discard-Data") {
                    req.option(OtaCoapOption::DISCARD_DATA, 1);
                    w.sendMessage(req);
                    auto resp = w.receiveMessage();
                    CHECK(resp.type() == CoapType::ACK);
                    CHECK((isCoapResponseCode(resp.code()) && !isCoapSuccessCode(resp.code())));
                    CHECK(resp.token() == w.lastMessageToken());
                    CHECK(hasDiagnosticPayload(resp));
                }
            }
        }
    }
    SECTION("fails if UpdateFinish is not received within the timeout") {
        auto cb = w.callbacksMock();
        Spy(Method(cb, finishFirmwareUpdate));
        w.sendStart(512 /* fileSize */, std::string() /* fileHash */, 512 /* chunkSize */, false /* discardData */);
        w.skipMessages(2); // Skip the ACK and response
        w.sendChunk(1 /* index */, genString(512) /* data */);
        w.skipMessages(1); // Skip the UpdateAck
        w.addMillis(OTA_TRANSFER_TIMEOUT);
        CHECK(w.process() == ProtocolError::MESSAGE_TIMEOUT);
        Verify(Method(cb, finishFirmwareUpdate).Matching([=](unsigned flags) {
            return FirmwareUpdateFlags::fromUnderlying(flags) == FirmwareUpdateFlag::CANCEL;
        })).Once();
        CHECK(!w.isRunning());
    }
}
