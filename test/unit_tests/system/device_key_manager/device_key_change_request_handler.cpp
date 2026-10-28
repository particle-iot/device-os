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

#include <catch2/catch.hpp>

#include "device_key_manager.h"
#include "cloud/protobuf.h"
#include "system_cloud_internal.h"
#include "system_mode.h"
#include "filesystem.h"
#include "system_error.h"

#include "system/mock/coap_api_fake.h"
#include "system/mock/system_fakes.h"

#include "cloud/cloud.pb.h"

#include <pb_encode.h>
#include <pb_decode.h>

#include "mbedtls/pk.h"

#include <optional>
#include <string>
#include <cstring>

using namespace particle;
using namespace particle::system;
using namespace particle::test;

namespace {

const auto PREPARE = particle_cloud_Request_Type_CONFIG_PREPARE_DEVICE_KEY_CHANGE;
const auto FINALIZE = particle_cloud_Request_Type_CONFIG_FINALIZE_DEVICE_KEY_CHANGE;

const size_t KEY_ID_SIZE = 16;

int g_resetCount = 0;
int g_clearSessionCount = 0;

// Request sent by the device or the "cloud" (particle.cloud.Request)
struct DecodedRequest {
    int type = 0;
    std::string keyId; // FINALIZE
    std::string publicKey; // FINALIZE
};

// Response sent by the device (particle.cloud.Response)
struct DecodedResponse {
    int result = 0;
    std::string keyId; // PREPARE
};

bool decodeString(pb_istream_t* stream, const pb_field_iter_t* field, void** arg) {
    auto s = (std::string*)*arg;
    s->resize(stream->bytes_left);
    return pb_read(stream, (pb_byte_t*)s->data(), s->size());
}

template<typename T>
std::string encodeMessage(const pb_msgdesc_t* desc, const T& msg) {
    pb_ostream_t sizeStream = PB_OSTREAM_SIZING;
    REQUIRE(pb_encode(&sizeStream, desc, &msg));
    std::string s(sizeStream.bytes_written, '\0');
    auto stream = pb_ostream_from_buffer((pb_byte_t*)s.data(), s.size());
    REQUIRE(pb_encode(&stream, desc, &msg));
    return s;
}

std::string encodePrepareRequest() {
    particle_cloud_Request req = {};
    req.type = PREPARE;
    req.which_data = particle_cloud_Request_config_prepare_device_key_change_tag;
    return encodeMessage(&particle_cloud_Request_msg, req);
}

// Response sent by the cloud. An empty payload is returned if `result` is 0 and `withData` is false
std::string encodeFinalizeResponse(int result, bool withData = false) {
    particle_cloud_Response resp = {};
    resp.result = result;
    if (withData) {
        resp.which_data = particle_cloud_Response_config_finalize_device_key_change_tag;
    }
    return encodeMessage(&particle_cloud_Response_msg, resp);
}

DecodedRequest decodeFinalizeRequest(const std::string& payload) {
    DecodedRequest r;
    // nanopb doesn't support callbacks in oneof fields so the submessage is decoded separately
    auto stream = pb_istream_from_buffer((const pb_byte_t*)payload.data(), payload.size());
    std::optional<std::string> subMsg;
    for (;;) {
        auto type = pb_wire_type_t();
        uint32_t tag = 0;
        bool eof = false;
        if (!pb_decode_tag(&stream, &type, &tag, &eof)) {
            REQUIRE(eof);
            break;
        }
        if (tag == particle_cloud_Request_type_tag) {
            REQUIRE(type == PB_WT_VARINT);
            uint32_t v = 0;
            REQUIRE(pb_decode_varint32(&stream, &v));
            r.type = v;
        } else if (tag == particle_cloud_Request_config_finalize_device_key_change_tag) {
            REQUIRE(type == PB_WT_STRING);
            std::string s;
            pb_istream_t subStream = {};
            REQUIRE(pb_make_string_substream(&stream, &subStream));
            s.resize(subStream.bytes_left);
            REQUIRE(pb_read(&subStream, (pb_byte_t*)s.data(), s.size()));
            REQUIRE(pb_close_string_substream(&stream, &subStream));
            subMsg = std::move(s);
        } else {
            REQUIRE(pb_skip_field(&stream, type));
        }
    }
    REQUIRE(subMsg);
    particle_cloud_config_FinalizeDeviceKeyChangeRequest req = {};
    req.public_key.funcs.decode = decodeString;
    req.public_key.arg = &r.publicKey;
    stream = pb_istream_from_buffer((const pb_byte_t*)subMsg->data(), subMsg->size());
    REQUIRE(pb_decode(&stream, &particle_cloud_config_FinalizeDeviceKeyChangeRequest_msg, &req));
    r.keyId = std::string((const char*)req.key_id.bytes, req.key_id.size);
    return r;
}

DecodedResponse decodePrepareResponse(const std::string& payload) {
    DecodedResponse r;
    particle_cloud_Response resp = {};
    auto stream = pb_istream_from_buffer((const pb_byte_t*)payload.data(), payload.size());
    REQUIRE(pb_decode(&stream, &particle_cloud_Response_msg, &resp));
    REQUIRE(resp.which_data == particle_cloud_Response_config_prepare_device_key_change_tag);
    auto& data = resp.data.config_prepare_device_key_change;
    r.result = resp.result;
    r.keyId = std::string((const char*)data.key_id.bytes, data.key_id.size);
    return r;
}

// Parses a DER-encoded EC private key and returns its DER-encoded public key
std::string derivePublicKey(const CryptoKey& privKey) {
    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);
    REQUIRE(mbedtls_pk_parse_key(&pk, (const uint8_t*)privKey.data(), privKey.size(), nullptr, 0) == 0);
    uint8_t buf[256] = {};
    int r = mbedtls_pk_write_pubkey_der(&pk, buf, sizeof(buf));
    mbedtls_pk_free(&pk);
    REQUIRE(r > 0);
    // mbedtls_pk_write_pubkey_der() writes the key to the end of the buffer
    return std::string((const char*)buf + sizeof(buf) - r, r);
}

void ensureDir(const char* path) {
    auto lfs = &filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr)->instance;
    int r = lfs_mkdir(lfs, path);
    REQUIRE((r == 0 || r == LFS_ERR_EXIST));
}

// Registers a request handler for PROTOBUF_REQUEST_URI that forwards the requests to a
// DeviceKeyChangeRequestHandler
class HandlerFixture {
public:
    HandlerFixture() :
            coap_(CoapFake::instance()) {
        g_resetCount = 0;
        g_clearSessionCount = 0;
        ensureDir("/sys");
        ensureDir("/tmp");
        REQUIRE(DeviceKeyManager::instance().clearCandidateKey() == 0);
        REQUIRE(coap_add_request_handler(PROTOBUF_REQUEST_URI, PROTOBUF_REQUEST_METHOD, 0 /* flags */,
                requestCallback, this, nullptr /* reserved */) == 0);
        coap_.connect();
    }

    ~HandlerFixture() {
        if (coap_.isConnected()) {
            coap_.disconnect();
        }
        runTimers();
        coap_remove_request_handler(PROTOBUF_REQUEST_URI, PROTOBUF_REQUEST_METHOD, nullptr /* reserved */);
        coap_.clearLog();
        DeviceKeyManager::instance().clearCandidateKey();
    }

    // Send a PrepareDeviceKeyChangeRequest to the device and return its response
    DecodedResponse prepare(const std::string& payload = encodePrepareRequest()) {
        prepareReqId_ = coap_.sendRequest(PROTOBUF_REQUEST_URI, PROTOBUF_REQUEST_METHOD, payload);
        runTimers();
        REQUIRE(handleResult_ == CloudRequestHandler::RUNNING);
        auto resp = coap_.responseFor(prepareReqId_);
        REQUIRE(resp);
        REQUIRE(resp->status == COAP_STATUS_CHANGED);
        return decodePrepareResponse(resp->payload);
    }

    // Acknowledge the device's response to the PrepareDeviceKeyChangeRequest and return the
    // FinalizeDeviceKeyChangeRequest sent by the device
    DecodedRequest ackPrepare() {
        auto count = coap_.requests().size();
        coap_.ackResponse(prepareReqId_);
        runTimers();
        REQUIRE(coap_.requests().size() == count + 1);
        auto req = coap_.lastRequest();
        REQUIRE(req->uri == PROTOBUF_REQUEST_URI);
        REQUIRE(req->method == PROTOBUF_REQUEST_METHOD);
        finalizeReqId_ = req->reqId;
        return decodeFinalizeRequest(req->payload);
    }

    void respondToFinalize(int status, const std::string& payload) {
        coap_.respond(finalizeReqId_, status, payload);
        runTimers();
    }

    std::optional<std::string> candidateKey() {
        EcpPrivateKey key;
        int r = DeviceKeyManager::instance().getCandidateKey(key);
        if (r == SYSTEM_ERROR_CRYPTO_KEY_NOT_FOUND) {
            return std::nullopt;
        }
        REQUIRE(r == 0);
        return std::string(key.data(), key.size());
    }

    CoapFake& coap_;
    DeviceKeyChangeRequestHandler handler_;
    int prepareReqId_ = 0;
    int finalizeReqId_ = 0;
    int handleResult_ = 0; // Result of the last CloudRequestHandler::handle() call
    std::optional<int> completeResult_; // Result reported via the completion callback
    int completeCount_ = 0; // Number of times the completion callback was called
    bool running_ = false; // Whether the handler is processing a request asynchronously

private:
    static int requestCallback(coap_message* msg, const char* path, int method, int reqId, void* arg) {
        auto self = static_cast<HandlerFixture*>(arg);
        CoapMessagePtr req(msg);
        CloudRequestHandler& h = self->handler_;
        int r = h.handle(reqId, req, self, [](int result, CloudRequestHandler* handler, void* arg) {
            auto self = static_cast<HandlerFixture*>(arg);
            self->completeResult_ = result;
            ++self->completeCount_;
            self->running_ = false;
        });
        if (r == CloudRequestHandler::RUNNING) {
            self->running_ = true;
        }
        self->handleResult_ = r;
        return 0;
    }
};

} // namespace

// Fakes for the system functions used by the code under test

namespace particle {

void clearSessionData() {
    ++g_clearSessionCount;
}

} // namespace particle

int system_reset(unsigned mode, unsigned reason, unsigned value, unsigned flags, void* reserved) {
    ++g_resetCount;
    return 0;
}

TEST_CASE_METHOD(HandlerFixture, "DeviceKeyChangeRequestHandler changes the device key") {
    auto prepResp = prepare();
    CHECK(prepResp.result == 0);
    CHECK(prepResp.keyId.size() == KEY_ID_SIZE);
    CHECK(running_);
    CHECK(!handler_.needResponse());

    SECTION("doesn't send the finalize request until the prepare response is acknowledged") {
        CHECK(coap_.requests().empty());
    }

    SECTION("sends the public key of the new device key with the same key ID") {
        auto req = ackPrepare();
        CHECK(req.type == FINALIZE);
        CHECK(req.keyId == prepResp.keyId);
        CHECK(!req.publicKey.empty());
        CHECK(!candidateKey()); // The key is not saved until the server confirms the change
        CHECK(!completeResult_);
    }

    SECTION("saves the candidate key and resets the device when the server confirms the change") {
        SECTION("with an empty response payload") {
            auto req = ackPrepare();
            respondToFinalize(COAP_STATUS_CHANGED, encodeFinalizeResponse(0));
            auto key = candidateKey();
            REQUIRE(key);
            EcpPrivateKey privKey;
            REQUIRE(DeviceKeyManager::instance().getCandidateKey(privKey) == 0);
            CHECK(derivePublicKey(privKey) == req.publicKey);
        }

        SECTION("with a non-empty response payload") {
            ackPrepare();
            auto payload = encodeFinalizeResponse(0, true /* withData */);
            REQUIRE(!payload.empty());
            respondToFinalize(COAP_STATUS_CHANGED, payload);
            CHECK(candidateKey());
        }

        CHECK(completeResult_ == 0);
        CHECK(!running_);
        CHECK(completeCount_ == 1);
        CHECK(g_clearSessionCount == 1);
        CHECK(g_resetCount == 1);
    }

    SECTION("discards the new key if the server rejects the change") {
        ackPrepare();

        SECTION("with an error CoAP code") {
            respondToFinalize(COAP_STATUS_BAD_REQUEST, encodeFinalizeResponse(0));
        }

        SECTION("with an error result") {
            respondToFinalize(COAP_STATUS_CHANGED, encodeFinalizeResponse(particle_cloud_Response_Result_ERROR));
        }

        SECTION("with a negative error result") {
            respondToFinalize(COAP_STATUS_CHANGED, encodeFinalizeResponse(-5));
        }

        CHECK(completeResult_ == SYSTEM_ERROR_SERVER_REQUEST_FAILED);
        CHECK(!running_);
        CHECK(completeCount_ == 1);
        CHECK(!candidateKey());
        CHECK(g_resetCount == 0);
    }

    SECTION("fails if the prepare response can't be delivered") {
        coap_.failResponse(prepareReqId_, SYSTEM_ERROR_COAP_MESSAGE_RESET);
        runTimers();
        CHECK(completeResult_ == SYSTEM_ERROR_COAP_MESSAGE_RESET);
        CHECK(!running_);
        CHECK(completeCount_ == 1);
        CHECK(coap_.requests().empty());
    }

    SECTION("fails if the finalize request can't be delivered") {
        ackPrepare();
        coap_.failRequest(finalizeReqId_, SYSTEM_ERROR_COAP_MESSAGE_RESET);
        runTimers();
        CHECK(completeResult_ == SYSTEM_ERROR_COAP_MESSAGE_RESET);
        CHECK(!running_);
        CHECK(completeCount_ == 1);
        CHECK(!candidateKey());
        CHECK(g_resetCount == 0);
    }

    SECTION("fails if the device disconnects before the change is finalized") {
        SECTION("while waiting for the prepare response to be acknowledged") {
        }

        SECTION("while waiting for the finalize response") {
            ackPrepare();
        }

        coap_.disconnect(SYSTEM_ERROR_COAP_REQUEST_CANCELLED);
        runTimers();
        REQUIRE(completeResult_);
        CHECK(*completeResult_ < 0);
        CHECK(!running_);
        CHECK(completeCount_ == 1);
        CHECK(!candidateKey());
        CHECK(g_resetCount == 0);

        SECTION("and accepts a new request after reconnecting") {
            coap_.connect();
            completeResult_.reset();
            auto resp = prepare();
            CHECK(resp.keyId.size() == KEY_ID_SIZE);
            CHECK(resp.keyId != prepResp.keyId);
            CHECK(running_);
        }
    }

    SECTION("rejects another request while the key change is in progress") {
        coap_.sendRequest(PROTOBUF_REQUEST_URI, PROTOBUF_REQUEST_METHOD, encodePrepareRequest());
        runTimers();
        CHECK(handleResult_ == SYSTEM_ERROR_INVALID_STATE);
        // The ongoing key change is not affected
        CHECK(running_);
        auto req = ackPrepare();
        CHECK(req.keyId == prepResp.keyId);
    }
}

TEST_CASE_METHOD(HandlerFixture, "getProtobufRequestType() finds the request type after other fields") {
    // An unknown field that doesn't fit in the buffer used for the initial lookup, followed by the
    // request type
    std::string payload;
    payload += "\xa2\x06"; // Field 100, length-delimited
    payload += "\xc8\x01"; // Length: 200
    payload += std::string(200, 'x');
    payload += '\x08'; // Field 1 (type), varint
    payload += (char)PREPARE;

    int result = 0; // Request type or an error code
    REQUIRE(coap_add_request_handler("t", COAP_METHOD_POST, 0 /* flags */, [](coap_message* msg, const char* path,
            int method, int reqId, void* arg) {
        CoapMessagePtr req(msg);
        int type = 0;
        int r = getProtobufRequestType(req.get(), type);
        *(int*)arg = (r == 0) ? type : r;
        return 0;
    }, &result, nullptr /* reserved */) == 0);
    coap_.sendRequest("t", COAP_METHOD_POST, payload);
    runTimers();
    coap_remove_request_handler("t", COAP_METHOD_POST, nullptr /* reserved */);
    CHECK(result == PREPARE);
}
