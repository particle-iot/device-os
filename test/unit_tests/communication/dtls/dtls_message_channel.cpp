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

#include "dtls_message_channel.h"
#include "dtls_session_persist.h"
#include "protocol_defs.h"

#include "util/coap_message.h"

#include "mbedtls/ssl.h"
#include "mbedtls/ssl_internal.h"
#include "mbedtls/pk.h"
#include "mbedtls/ecp.h"

#include <catch2/catch.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

namespace particle::protocol::test {

namespace {

const size_t MAX_KEY_SIZE = 512;
const size_t MAX_DATAGRAM_SIZE = 2048;
const int MAX_HANDSHAKE_STEPS = 100;
const size_t DTLS_HANDSHAKE_HEADER_SIZE = 12;
const size_t RECORD_SEQUENCE_OFFSET = 3;
const size_t RECORD_SEQUENCE_SIZE = 8; // Epoch and sequence number

std::deque<std::string> g_toServer;
std::deque<std::string> g_toDevice;
std::deque<std::string> g_failedDatagrams;

// Result returned by the device's send callback when sending fails: -1 for a network error, 0 if
// the socket is out of memory (see system_cloud_send())
bool g_failSend = false;
int g_failSendResult = 0;

std::string g_session;
system_tick_t g_millis = 0;

int testRng(void* /* ctx */, unsigned char* data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        data[i] = rand();
    }
    return 0;
}

int deviceSend(const unsigned char* data, uint32_t size, void* /* ctx */) {
    if (g_failSend) {
        g_failedDatagrams.emplace_back((const char*)data, size);
        return g_failSendResult;
    }
    g_toServer.emplace_back((const char*)data, size);
    return size;
}

int deviceReceive(unsigned char* data, uint32_t size, void* /* ctx */) {
    if (g_toDevice.empty()) {
        return 0;
    }
    auto d = std::move(g_toDevice.front());
    g_toDevice.pop_front();
    size_t n = std::min<size_t>(size, d.size());
    std::memcpy(data, d.data(), n);
    return n;
}

system_tick_t millis() {
    return ++g_millis;
}

int saveSession(const void* data, size_t size, uint8_t /* type */, void* /* reserved */) {
    g_session.assign((const char*)data, size);
    return 0;
}

int restoreSession(void* data, size_t maxSize, uint8_t /* type */, void* /* reserved */) {
    if (g_session.empty() || g_session.size() > maxSize) {
        return 0;
    }
    std::memcpy(data, g_session.data(), g_session.size());
    return g_session.size();
}

uint64_t recordSequence(const std::string& record) {
    REQUIRE(record.size() >= RECORD_SEQUENCE_OFFSET + RECORD_SEQUENCE_SIZE);
    uint64_t seq = 0;
    for (size_t i = RECORD_SEQUENCE_OFFSET; i < RECORD_SEQUENCE_OFFSET + RECORD_SEQUENCE_SIZE; ++i) {
        seq = (seq << 8) | (uint8_t)record[i];
    }
    return seq;
}

uint32_t calculateCrc(const uint8_t* data, uint32_t size) {
    uint32_t crc = 0;
    for (uint32_t i = 0; i < size; ++i) {
        crc = crc * 31 + data[i];
    }
    return crc;
}

class EcKey {
public:
    EcKey() {
        mbedtls_pk_init(&pk_);
        REQUIRE(mbedtls_pk_setup(&pk_, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY)) == 0);
        REQUIRE(mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, mbedtls_pk_ec(pk_), testRng, nullptr) == 0);
    }

    ~EcKey() {
        mbedtls_pk_free(&pk_);
    }

    std::string privateKey() {
        unsigned char buf[MAX_KEY_SIZE];
        int n = mbedtls_pk_write_key_der(&pk_, buf, sizeof(buf));
        REQUIRE(n > 0);
        return std::string((const char*)buf + sizeof(buf) - n, n);
    }

    std::string publicKey() {
        unsigned char buf[MAX_KEY_SIZE];
        int n = mbedtls_pk_write_pubkey_der(&pk_, buf, sizeof(buf));
        REQUIRE(n > 0);
        return std::string((const char*)buf + sizeof(buf) - n, n);
    }

    mbedtls_pk_context* get() {
        return &pk_;
    }

private:
    mbedtls_pk_context pk_;
};

// The cloud side of the tests. Uses the same mbedtls build as the device
class DtlsServer {
public:
    explicit DtlsServer(EcKey& key) :
            timerFinMs_(0) {
        mbedtls_ssl_init(&ssl_);
        mbedtls_ssl_config_init(&conf_);
        mbedtls_x509_crt_init(&crt_);
        REQUIRE(mbedtls_ssl_config_defaults(&conf_, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_DATAGRAM,
                MBEDTLS_SSL_PRESET_DEFAULT) == 0);
        mbedtls_ssl_conf_rng(&conf_, testRng, nullptr);
        mbedtls_ssl_conf_min_version(&conf_, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
        REQUIRE(mbedtls_ssl_conf_own_cert(&conf_, &crt_, key.get()) == 0);
        mbedtls_ssl_conf_authmode(&conf_, MBEDTLS_SSL_VERIFY_OPTIONAL);
        static int certTypes[] = { MBEDTLS_TLS_CERT_TYPE_RAW_PUBLIC_KEY, MBEDTLS_TLS_CERT_TYPE_NONE };
        mbedtls_ssl_conf_client_certificate_types(&conf_, certTypes);
        mbedtls_ssl_conf_server_certificate_types(&conf_, certTypes);
        // The device knows the server key
        mbedtls_ssl_conf_certificate_send(&conf_, MBEDTLS_SSL_SEND_CERTIFICATE_DISABLED);
        mbedtls_ssl_conf_dtls_cookies(&conf_, nullptr, nullptr, nullptr);
        mbedtls_ssl_conf_cid(&conf_, DTLS_CID_SIZE, MBEDTLS_SSL_UNEXPECTED_CID_IGNORE);
        REQUIRE(mbedtls_ssl_setup(&ssl_, &conf_) == 0);
        mbedtls_ssl_set_timer_cb(&ssl_, this, setTimer, getTimer);
        mbedtls_ssl_set_bio(&ssl_, this, send, receive, nullptr);
        const unsigned char cid[DTLS_CID_SIZE] = { 1, 2, 3, 4, 5, 6, 7, 8 };
        REQUIRE(mbedtls_ssl_set_cid(&ssl_, MBEDTLS_SSL_CID_ENABLED, cid, sizeof(cid)) == 0);
    }

    ~DtlsServer() {
        mbedtls_ssl_free(&ssl_);
        mbedtls_ssl_config_free(&conf_);
        mbedtls_x509_crt_free(&crt_);
    }

    int handshake() {
        return mbedtls_ssl_handshake(&ssl_);
    }

    bool isConnected() const {
        return ssl_.state == MBEDTLS_SSL_HANDSHAKE_OVER;
    }

    void sendHelloRequest() {
        ssl_.out_msgtype = MBEDTLS_SSL_MSG_HANDSHAKE;
        std::memset(ssl_.out_msg, 0, DTLS_HANDSHAKE_HEADER_SIZE);
        ssl_.out_msglen = DTLS_HANDSHAKE_HEADER_SIZE;
        REQUIRE(mbedtls_ssl_write_record(&ssl_, 1 /* force_flush */) == 0);
    }

    // Returns the payloads of the CoAP messages received from the device
    std::vector<std::string> receive() {
        std::vector<std::string> payloads;
        for (;;) {
            unsigned char buf[MAX_DATAGRAM_SIZE];
            int r = mbedtls_ssl_read(&ssl_, buf, sizeof(buf));
            if (r == MBEDTLS_ERR_SSL_WANT_READ) {
                break;
            }
            REQUIRE(r > 0);
            payloads.push_back(CoapMessage::decode((const char*)buf, r).payload());
        }
        return payloads;
    }

private:
    mbedtls_ssl_context ssl_;
    mbedtls_ssl_config conf_;
    mbedtls_x509_crt crt_;
    uint32_t timerFinMs_;

    static int send(void* /* ctx */, const unsigned char* data, size_t size) {
        g_toDevice.emplace_back((const char*)data, size);
        return size;
    }

    static int receive(void* /* ctx */, unsigned char* data, size_t size) {
        if (g_toServer.empty()) {
            return MBEDTLS_ERR_SSL_WANT_READ;
        }
        auto d = std::move(g_toServer.front());
        g_toServer.pop_front();
        size_t n = std::min(size, d.size());
        std::memcpy(data, d.data(), n);
        return n;
    }

    static void setTimer(void* ctx, uint32_t /* intMs */, uint32_t finMs) {
        static_cast<DtlsServer*>(ctx)->timerFinMs_ = finMs;
    }

    static int getTimer(void* ctx) {
        return static_cast<DtlsServer*>(ctx)->timerFinMs_ ? 0 : -1;
    }
};

class DtlsFixture {
public:
    DtlsFixture() :
            server_(serverKey_),
            deviceId_(),
            coapState_(0),
            lastMsgId_(0) {
        g_toServer.clear();
        g_toDevice.clear();
        g_failedDatagrams.clear();
        g_failSend = false;
        g_session.clear();
        DTLSMessageChannel::Callbacks callbacks = {};
        callbacks.millis = millis;
        callbacks.send = deviceSend;
        callbacks.receive = deviceReceive;
        callbacks.save = saveSession;
        callbacks.restore = restoreSession;
        callbacks.calculate_crc = calculateCrc;
        callbacks.handle_seed = [](const uint8_t* /* seed */, size_t /* size */) {};
        callbacks.notify_client_messages_processed = [](void* /* reserved */) {};
        const auto devicePrivate = deviceKey_.privateKey();
        const auto devicePublic = deviceKey_.publicKey();
        const auto serverPublic = serverKey_.publicKey();
        REQUIRE(channel_.init((const uint8_t*)devicePrivate.data(), devicePrivate.size(),
                (const uint8_t*)devicePublic.data(), devicePublic.size(),
                (const uint8_t*)serverPublic.data(), serverPublic.size(),
                deviceId_, callbacks, &coapState_) == ProtocolError::NO_ERROR);
        connect();
    }

    ProtocolError send(const std::string& payload) {
        CoapMessage m;
        m.type(CoapType::CON);
        m.code(CoapCode::POST);
        m.id(++lastMsgId_);
        m.token(std::string("t"));
        m.payload(payload);
        const auto data = m.encode();
        Message msg;
        REQUIRE(channel_.create(msg) == ProtocolError::NO_ERROR);
        REQUIRE(data.size() <= msg.capacity());
        std::memcpy(msg.buf(), data.data(), data.size());
        msg.set_length(data.size());
        return channel_.send(msg);
    }

    ProtocolError receive() {
        Message msg;
        return channel_.receive(msg);
    }

    std::vector<std::string> serverReceive() {
        return server_.receive();
    }

    void serverSendHelloRequest() {
        server_.sendHelloRequest();
    }

    void persistSession() {
        REQUIRE(channel_.notify_established() == ProtocolError::NO_ERROR);
    }

    // Emulates a reconnection to the server after the protocol is reset (see Protocol::reset())
    ProtocolError resume() {
        channel_.reset();
        return channel_.establish();
    }

    const std::string& lastFailedDatagram() const {
        REQUIRE(!g_failedDatagrams.empty());
        return g_failedDatagrams.back();
    }

    const std::string& lastDatagramToServer() const {
        REQUIRE(!g_toServer.empty());
        return g_toServer.back();
    }

    void failSends(int result) {
        g_failSend = true;
        g_failSendResult = result;
    }

    void stopFailingSends() {
        g_failSend = false;
    }

private:
    EcKey deviceKey_;
    EcKey serverKey_;
    DtlsServer server_;
    DTLSMessageChannel channel_;
    uint8_t deviceId_[DEVICE_ID_LEN];
    message_id_t coapState_;
    CoapMessageId lastMsgId_;

    void connect() {
        for (int i = 0; i < MAX_HANDSHAKE_STEPS; ++i) {
            const auto r = channel_.establish();
            REQUIRE((r == ProtocolError::NO_ERROR || r == ProtocolError::IN_PROGRESS));
            const int serverResult = server_.handshake();
            REQUIRE((serverResult == 0 || serverResult == MBEDTLS_ERR_SSL_WANT_READ));
            if (r == ProtocolError::NO_ERROR && server_.isConnected()) {
                return;
            }
        }
        FAIL("Handshake didn't complete");
    }
};

} // namespace

TEST_CASE("DTLSMessageChannel sends messages to the server", "[dtls]") {
    DtlsFixture fixture;
    CHECK(fixture.send("a") == ProtocolError::NO_ERROR);
    CHECK(fixture.send("b") == ProtocolError::NO_ERROR);
    CHECK(fixture.serverReceive() == std::vector<std::string>{ "a", "b" });
}

TEST_CASE("DTLSMessageChannel sends a message after a failed send", "[dtls]") {
    SECTION("network error") {
        DtlsFixture fixture;
        fixture.failSends(-1);
        CHECK(fixture.send("a") == ProtocolError::IO_ERROR_SOCKET_SEND_FAILED);
        fixture.stopFailingSends();
        CHECK(fixture.send("b") == ProtocolError::NO_ERROR);
        CHECK(fixture.serverReceive() == std::vector<std::string>{ "b" });
    }

    SECTION("socket is out of memory") {
        DtlsFixture fixture;
        fixture.failSends(0);
        CHECK(fixture.send("a") == ProtocolError::NO_ERROR);
        fixture.stopFailingSends();
        CHECK(fixture.send("b") == ProtocolError::NO_ERROR);
        CHECK(fixture.serverReceive() == std::vector<std::string>{ "b" });
    }
}

TEST_CASE("DTLSMessageChannel resumes the session", "[dtls]") {
    DtlsFixture fixture;
    fixture.persistSession();
    CHECK(fixture.send("a") == ProtocolError::NO_ERROR);
    REQUIRE(fixture.resume() == ProtocolError::SESSION_RESUMED);
    CHECK(fixture.send("b") == ProtocolError::NO_ERROR);
    CHECK(fixture.serverReceive() == std::vector<std::string>{ "a", "b" });
}

TEST_CASE("DTLSMessageChannel does not reuse the sequence number of a record that failed to send", "[dtls]") {
    DtlsFixture fixture;
    fixture.persistSession();
    fixture.failSends(-1);
    CHECK(fixture.send("a") == ProtocolError::IO_ERROR_SOCKET_SEND_FAILED);
    fixture.stopFailingSends();
    const auto failedSeq = recordSequence(fixture.lastFailedDatagram());
    REQUIRE(fixture.resume() == ProtocolError::SESSION_RESUMED);
    CHECK(fixture.send("b") == ProtocolError::NO_ERROR);
    CHECK(recordSequence(fixture.lastDatagramToServer()) > failedSeq);
    CHECK(fixture.serverReceive() == std::vector<std::string>{ "b" });
}

TEST_CASE("DTLSMessageChannel keeps the session if it can't reply to a HelloRequest", "[dtls]") {
    SECTION("socket is out of memory") {
        DtlsFixture fixture;
        fixture.persistSession();
        fixture.serverSendHelloRequest();
        fixture.failSends(0);
        CHECK(fixture.receive() == ProtocolError::NO_ERROR);
        fixture.stopFailingSends();
        CHECK(fixture.send("a") == ProtocolError::NO_ERROR);
        CHECK(fixture.serverReceive() == std::vector<std::string>{ "a" });
        CHECK(fixture.resume() == ProtocolError::SESSION_RESUMED);
    }

    SECTION("network error") {
        DtlsFixture fixture;
        fixture.persistSession();
        fixture.serverSendHelloRequest();
        fixture.failSends(-1);
        CHECK(fixture.receive() == ProtocolError::IO_ERROR_SOCKET_RECV_FAILED);
        fixture.stopFailingSends();
        CHECK(fixture.send("a") == ProtocolError::NO_ERROR);
        CHECK(fixture.serverReceive() == std::vector<std::string>{ "a" });
        CHECK(fixture.resume() == ProtocolError::SESSION_RESUMED);
    }
}

TEST_CASE("DTLSMessageChannel does not send a message that failed to send later", "[dtls]") {
    DtlsFixture fixture;
    fixture.failSends(-1);
    CHECK(fixture.send("a") == ProtocolError::IO_ERROR_SOCKET_SEND_FAILED);
    fixture.stopFailingSends();
    CHECK(fixture.receive() == ProtocolError::NO_ERROR);
    CHECK(fixture.serverReceive().empty());
}

} // namespace particle::protocol::test
