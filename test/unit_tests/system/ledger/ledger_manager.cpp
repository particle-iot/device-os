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

#include "ledger/ledger.h"
#include "ledger/ledger_manager.h"

#include "util/coap_api_fake.h"
#include "util/system_fakes.h"
#include "util/ledger_pb.h"

#include "cloud/cloud.pb.h"

#include <catch2/catch.hpp>

using namespace particle;
using namespace particle::system;
using namespace particle::test;

namespace {

const auto LEDGER_URI = "L";
const int LEDGER_METHOD = COAP_METHOD_POST;

const unsigned MIN_RETRY_DELAY = 30000; // See ledger_manager.cpp

const int GET_INFO = particle_cloud_Request_Type_LEDGER_GET_INFO;
const int SET_DATA = particle_cloud_Request_Type_LEDGER_SET_DATA;
const int GET_DATA = particle_cloud_Request_Type_LEDGER_GET_DATA;
const int SUBSCRIBE = particle_cloud_Request_Type_LEDGER_SUBSCRIBE;

int g_syncCount = 0;

void syncCallback(ledger_instance* /* ledger */, void* /* appData */) {
    ++g_syncCount;
}

// LedgerManager is a singleton that is initialized only once, so every test leaves it disconnected
// and with no ledgers
class LedgerFixture {
public:
    LedgerFixture() :
            coap_(CoapFake::instance()),
            mgr_(LedgerManager::instance()) {
        g_syncCount = 0;
    }

    ~LedgerFixture() {
        ledgers_.clear();
        if (coap_.isConnected()) {
            coap_.disconnect();
        }
        runTimers();
        mgr_->removeAllData();
        coap_.clearLog();
    }

    RefCountPtr<Ledger> getLedger(const char* name) {
        RefCountPtr<Ledger> ledger;
        REQUIRE(mgr_->getLedger(ledger, name, true /* create */) == 0);
        ledger->setSyncCallback(syncCallback);
        ledgers_.push_back(ledger);
        return ledger;
    }

    void writeLedger(Ledger& ledger, const std::string& data) {
        LedgerWriter writer;
        REQUIRE(ledger.initWriter(writer, LedgerWriteSource::USER) == 0);
        REQUIRE(writer.write(data.data(), data.size()) == (int)data.size());
        REQUIRE(writer.close() == 0);
        runTimers();
    }

    std::string readLedger(Ledger& ledger) {
        LedgerReader reader;
        REQUIRE(ledger.initReader(reader) == 0);
        std::string data;
        char buf[64];
        int r = 0;
        while ((r = reader.read(buf, sizeof(buf))) > 0) {
            data.append(buf, r);
        }
        REQUIRE(r == SYSTEM_ERROR_END_OF_STREAM);
        REQUIRE(reader.close() == 0);
        return data;
    }

    void connect() {
        coap_.connect();
        runTimers();
    }

    const CoapFake::Sent& lastRequest() {
        auto req = coap_.lastRequest();
        REQUIRE(req);
        REQUIRE(req->uri == LEDGER_URI);
        REQUIRE(req->method == LEDGER_METHOD);
        return *req;
    }

    DecodedRequest lastDecodedRequest() {
        return decodeRequest(lastRequest().payload);
    }

    void respond(int status, const std::string& payload) {
        coap_.respond(lastRequest().reqId, status, payload);
        runTimers();
    }

    // Connect to the "cloud" and complete the initial synchronization of a single ledger
    void connectAndSetUpLedger(const std::string& name, ledger_sync_direction dir) {
        connect();
        auto req = lastDecodedRequest();
        REQUIRE(req.type == GET_INFO);
        REQUIRE(req.ledgerNames == std::vector<std::string>{ name });
        respond(COAP_STATUS_CONTENT, encodeGetInfoResponse({ { name, dir } }));
        if (dir == LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE) {
            req = lastDecodedRequest();
            REQUIRE(req.type == SUBSCRIBE);
            REQUIRE(req.ledgerNames == std::vector<std::string>{ name });
            respond(COAP_STATUS_CONTENT, encodeSubscribeResponse({ { name } }));
        }
    }

    size_t requestCount() const {
        return coap_.requests().size();
    }

protected:
    CoapFake& coap_;
    LedgerManager* mgr_;
    std::vector<RefCountPtr<Ledger>> ledgers_;
};

} // namespace

TEST_CASE_METHOD(LedgerFixture, "LedgerManager requests the info of a new ledger when connected") {
    getLedger("test");
    REQUIRE(requestCount() == 0); // Nothing is sent while offline
    connect();
    REQUIRE(requestCount() == 1);
    auto req = lastDecodedRequest();
    CHECK(req.type == GET_INFO);
    CHECK(req.ledgerNames == std::vector<std::string>{ "test" });
}

TEST_CASE_METHOD(LedgerFixture, "LedgerManager synchronizes a device-to-cloud ledger") {
    auto ledger = getLedger("test");
    connectAndSetUpLedger("test", LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD);
    REQUIRE(ledger->info().syncDirection() == LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD);
    auto count = requestCount();

    writeLedger(*ledger, "hello");
    REQUIRE(requestCount() == count + 1);
    auto req = lastDecodedRequest();
    CHECK(req.type == SET_DATA);
    CHECK(req.ledgerNames == std::vector<std::string>{ "test" });
    CHECK(req.data == "hello");
    CHECK(ledger->info().syncPending());

    SECTION("the ledger is marked as synchronized when the server accepts the data") {
        respond(COAP_STATUS_CHANGED, encodeResponse(particle_cloud_Response_Result_OK));
        CHECK(g_syncCount == 1);
        CHECK_FALSE(ledger->info().syncPending());
        CHECK(ledger->info().lastSynced() > 0);
        CHECK(requestCount() == count + 1);
    }

    SECTION("a server error causes a retry with an exponential backoff") {
        respond(COAP_STATUS_INTERNAL_SERVER_ERROR, encodeResponse(particle_cloud_Response_Result_ERROR));
        CHECK(g_syncCount == 0);
        CHECK(ledger->info().syncPending());
        count = requestCount();
        advanceTime(MIN_RETRY_DELAY - 1);
        REQUIRE(requestCount() == count);
        advanceTime(1);
        REQUIRE(requestCount() == count + 1);
        CHECK(lastDecodedRequest().type == SET_DATA);

        // Fail again, the delay should double
        respond(COAP_STATUS_INTERNAL_SERVER_ERROR, encodeResponse(particle_cloud_Response_Result_ERROR));
        count = requestCount();
        advanceTime(MIN_RETRY_DELAY * 2 - 1);
        REQUIRE(requestCount() == count);
        advanceTime(1);
        REQUIRE(requestCount() == count + 1);
        CHECK(lastDecodedRequest().type == SET_DATA);
    }

    SECTION("a transport error causes a retry") {
        coap_.failRequest(lastRequest().reqId, SYSTEM_ERROR_COAP_TIMEOUT);
        runTimers();
        CHECK(g_syncCount == 0);
        count = requestCount();
        advanceTime(MIN_RETRY_DELAY - 1);
        REQUIRE(requestCount() == count);
        advanceTime(1);
        REQUIRE(requestCount() == count + 1);
        CHECK(lastDecodedRequest().type == SET_DATA);
    }
}

TEST_CASE_METHOD(LedgerFixture, "LedgerManager handles an update notification for a cloud-to-device ledger") {
    auto ledger = getLedger("test");
    connectAndSetUpLedger("test", LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE);
    auto count = requestCount();

    int reqId = coap_.sendRequest(LEDGER_URI, LEDGER_METHOD, encodeNotifyUpdateRequest({ { "test", 1000 } }));
    runTimers();

    // The device acknowledges the notification
    auto resp = coap_.responseFor(reqId);
    REQUIRE(resp);
    CHECK(resp->status == COAP_STATUS_CHANGED);
    CHECK(decodeResponse(resp->payload).result == 0);

    // And requests the updated data
    REQUIRE(requestCount() == count + 1);
    auto req = lastDecodedRequest();
    CHECK(req.type == GET_DATA);
    CHECK(req.ledgerNames == std::vector<std::string>{ "test" });

    respond(COAP_STATUS_CONTENT, encodeGetDataResponse(1000, "world"));
    CHECK(g_syncCount == 1);
    CHECK(readLedger(*ledger) == "world");
    CHECK(ledger->info().lastUpdated() == 1000);
    CHECK_FALSE(ledger->info().syncPending());
}

TEST_CASE_METHOD(LedgerFixture, "LedgerManager replies with an error to an unsupported request") {
    getLedger("test");
    connectAndSetUpLedger("test", LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD);

    // GET_INFO is only ever sent by the device
    int reqId = coap_.sendRequest(LEDGER_URI, LEDGER_METHOD, encodeRequest(GET_INFO));
    runTimers();

    auto resp = coap_.responseFor(reqId);
    REQUIRE(resp);
    CHECK(resp->status == COAP_STATUS_BAD_REQUEST);
    auto r = decodeResponse(resp->payload);
    CHECK(r.result == SYSTEM_ERROR_NOT_SUPPORTED);
    CHECK_FALSE(r.message.empty());

    coap_.failResponse(reqId, SYSTEM_ERROR_COAP_TIMEOUT);
    runTimers();
}

TEST_CASE_METHOD(LedgerFixture, "LedgerManager cancels the ongoing request when disconnected") {
    getLedger("test");
    connect();
    REQUIRE(lastDecodedRequest().type == GET_INFO);

    coap_.disconnect();
    runTimers();

    // Synchronization starts over when reconnected
    auto count = requestCount();
    connect();
    REQUIRE(requestCount() == count + 1);
    CHECK(lastDecodedRequest().type == GET_INFO);
}
