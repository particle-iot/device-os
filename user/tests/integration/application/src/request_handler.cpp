/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#include "request_handler.h"

#include "test_suite.h"

#include "spark_wiring_system.h"
#include "spark_wiring_json.h"
#include "delay_hal.h"
#include "check.h"

#include "unit-test/unit-test.h"

namespace particle {

namespace {

using namespace spark; // For JSON classes

JSONValue getValue(const JSONValue& obj, const char* name) {
    JSONObjectIterator it(obj);
    while (it.next()) {
        if (it.name() == name) {
            return it.value();
        }
    }
    return JSONValue();
}

// Completion handler for Request::done()
void systemReset(int result, void* data) {
    HAL_Delay_Milliseconds(1000);
    System.reset();
}

} // namespace

// Wrapper for a control request handle
class RequestHandler::Request {
public:
    Request() :
            req_(nullptr) {
    }

    ~Request() {
        destroy();
    }

    int init(ctrl_request* req) {
        if (req->request_size > 0) {
            // Parse request
            auto d = JSONValue::parse(req->request_data, req->request_size);
            CHECK_TRUE(d.isObject(), SYSTEM_ERROR_BAD_DATA);
            data_ = std::move(d);
        }
        req_ = req;
        return 0;
    }

    void destroy() {
        data_ = JSONValue();
        if (req_) {
            // Having a pending request at this point is an internal error
            system_ctrl_set_result(req_, SYSTEM_ERROR_INTERNAL, nullptr, nullptr, nullptr);
            req_ = nullptr;
        }
    }

    template<typename EncodeFn>
    int reply(EncodeFn fn) {
        CHECK_TRUE(req_, SYSTEM_ERROR_INVALID_STATE);
        // Calculate the size of the reply data
        JSONBufferWriter writer(nullptr, 0);
        fn(writer);
        const size_t size = writer.dataSize();
        CHECK_TRUE(size > 0, SYSTEM_ERROR_INTERNAL);
        CHECK(system_ctrl_alloc_reply_data(req_, size, nullptr));
        // Serialize the reply
        writer = JSONBufferWriter(req_->reply_data, req_->reply_size);
        fn(writer);
        CHECK_TRUE(writer.dataSize() == size, SYSTEM_ERROR_INTERNAL);
        return 0;
    }

    void done(int result, ctrl_completion_handler_fn fn = nullptr, void* data = nullptr) {
        if (req_) {
            system_ctrl_set_result(req_, result, fn, data, nullptr);
            req_ = nullptr;
            destroy();
        }
    }

    JSONValue get(const char* name) const {
        return getValue(data_, name);
    }

    bool has(const char* name) const {
        return getValue(data_, name).isValid();
    }

    const JSONValue& data() const {
        return data_;
    }

    bool isEmpty() const {
        return !data_.isValid();
    }

private:
    JSONValue data_;
    ctrl_request* req_;
};

RequestHandler::RequestHandler() :
        inited_(false) {
}

RequestHandler::~RequestHandler() {
    destroy();
}

int RequestHandler::init() {
    inited_ = true;
    return 0;
}

void RequestHandler::destroy() {
    inited_ = false;
}

void RequestHandler::process(ctrl_request* ctrlReq) {
    const int r = request(ctrlReq);
    if (r < 0) {
        system_ctrl_set_result(ctrlReq, r, nullptr, nullptr, nullptr);
    }
}

int RequestHandler::request(ctrl_request* ctrlReq) {
    CHECK_TRUE(inited_, SYSTEM_ERROR_INVALID_STATE);
    Request req;
    CHECK(req.init(ctrlReq));
    const int r = CHECK(request(&req));
    req.done(r, (r == Result::RESET_PENDING) ? systemReset : nullptr);
    return 0;
}

int RequestHandler::request(Request* req) {
    const auto cmd = req->get("c").toString(); // Command
    if (cmd == "i") { // Init suite
        return initSuite(req);
    } else if (cmd == "l") { // List tests
        return listTests(req);
    } else if (cmd == "t") { // Start test
        return startTest(req);
    } else if (cmd == "s") { // Get status
        return getStatus(req);
    } else if (cmd == "L") { // Get log
        return getLog(req);
    } else if (cmd == "M") { // Read device->host mailbox
        return readMailbox(req);
    } else if (cmd == "r") { // Reset
        return reset(req);
    } else if (req->isEmpty()) { // Ping request
        return SYSTEM_ERROR_NONE;
    } else {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
}

int RequestHandler::initSuite(Request* req) {
    TestSuiteConfig conf;
    if (req->has("m")) { // System mode
        const auto val = req->get("m").toString();
        if (val == "d") { // Default
            conf.systemMode(DEFAULT);
        } else if (val == "a") { // Automatic
            conf.systemMode(AUTOMATIC);
        } else if (val == "s") { // Semi-automatic
            conf.systemMode(SEMI_AUTOMATIC);
        } else if (val == "m") { // Manual
            conf.systemMode(MANUAL);
        } else if (val == "S") { // Safe mode
            conf.systemMode(SAFE_MODE);
        } else {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
    }
    if (req->has("t")) { // System thread
        const bool val = req->get("t").toInt();
        conf.systemThreadEnabled(val);
    }
    if (req->has("b")) { // Clear backup memory
        const bool val = req->get("b").toInt();
        conf.clearBackupMemory(val);
    }
    return TestSuite::instance()->config(conf);
}

int RequestHandler::listTests(Request* req) {
    CHECK(req->reply([](JSONWriter& w) {
        w.beginArray();
        Test::for_each([&w](const Test& t) {
            w.value((const char*)t.name.data);
        });
        w.endArray();
    }));
    return 0;
}

int RequestHandler::startTest(Request* req) {
    const auto name = req->get("t").toString(); // Test name
    CHECK_TRUE(!name.isEmpty(), SYSTEM_ERROR_INVALID_ARGUMENT);
    const auto runner = TestRunner::instance();
    runner->reset();
    Test::exclude("*");
    const auto count = Test::include(name.data());
    CHECK_TRUE(count > 0, SYSTEM_ERROR_NOT_FOUND);
    runner->start();
    return 0;
}

int RequestHandler::getStatus(Request* req) {
    const auto runner = TestRunner::instance();
    if (runner->isComplete()) {
        if (Test::getCurrentFailed() > 0) {
            return Result::STATUS_FAILED;
        } if (Test::getCurrentPassed() > 0) {
            return Result::STATUS_PASSED;
        } else {
            return Result::STATUS_SKIPPED;
        }
    } else if (runner->isStarted()) {
        return Result::STATUS_RUNNING;
    } else {
        return Result::STATUS_WAITING;
    }
}

int RequestHandler::getLog(Request* req) {
    CHECK(req->reply([](JSONWriter& w) {
        const auto runner = TestRunner::instance();
        w.value(runner->logBuffer(), runner->logSize());
    }));
    return 0;
}

int RequestHandler::readMailbox(Request* req) {
    const auto runner = TestRunner::instance();
    auto m = runner->peekOutboundMailbox();
    if (!m) {
        return 0;
    }

    CHECK(req->reply([&](JSONWriter& w) {
        w.beginObject();
        w.name("t").value((int)m->type());
        const auto data = m->data();
        if (data.size() > 0) {
            w.name("d").value(data.data(), data.size());
        }
        w.endObject();
    }));
    if (m->waitedOn()) {
        req->done(0, [](int, void* data) -> void {
            auto m = static_cast<TestRunner::MailboxEntry*>(data);
            if (m->waitedOn()) {
                m->completed();
            } else {
                const auto runner = TestRunner::instance();
                runner->popOutboundMailbox();
            }
        }, m);
    } else {
        CHECK(runner->popOutboundMailbox());
    }
    return 0;
}

int RequestHandler::reset(Request* req) {
    TestSuite::instance()->destroy();
    return Result::RESET_PENDING;
}

RequestHandler* RequestHandler::instance() {
    static RequestHandler handler;
    return &handler;
}

} // namespace particle
