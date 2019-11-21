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

#include "suite.h"

#include "spark_wiring_system.h"
#include "spark_wiring_json.h"
#include "check.h"

#include "unit-test/unit-test.h"

namespace particle {

namespace test {

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

void systemReset() {
    System.reset();
}

} // namespace

// Wrapper for a control request handle
class RequestHandler::Request {
public:
    // Completion callback for Request::done()
    typedef void(*CompleteFn)();

    Request() :
            req_(nullptr) {
    }

    ~Request() {
        destroy();
    }

    int init(ctrl_request* req) {
        // Parse request
        auto d = JSONValue::parse(req->request_data, req->request_size);
        CHECK_TRUE(d.isObject(), SYSTEM_ERROR_BAD_DATA);
        data_ = std::move(d);
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
        CHECK(fn(writer));
        const size_t size = writer.dataSize();
        CHECK_TRUE(size > 0, SYSTEM_ERROR_INTERNAL);
        CHECK(system_ctrl_alloc_reply_data(req_, size, nullptr));
        // Serialize the reply
        writer = JSONBufferWriter(req_->reply_data, req_->reply_size);
        CHECK(fn(writer));
        CHECK_TRUE(writer.dataSize() == size, SYSTEM_ERROR_INTERNAL);
        return 0;
    }

    void done(CompleteFn fn = nullptr) {
        if (!req_) {
            return;
        }
        if (fn) {
            system_ctrl_set_result(req_, SYSTEM_ERROR_NONE, [](int result, void* data) {
                const auto fn = (CompleteFn)data;
                fn();
            }, (void*)fn, nullptr);
        } else {
            system_ctrl_set_result(req_, SYSTEM_ERROR_NONE, nullptr, nullptr, nullptr);
        }
        req_ = nullptr;
        destroy();
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

int RequestHandler::request(ctrl_request* ctrlReq) {
    CHECK_TRUE(inited_, SYSTEM_ERROR_INVALID_STATE);
    Request req;
    CHECK(req.init(ctrlReq));
    const int r = CHECK(request(&req));
    req.done((r == Result::RESET_PENDING) ? systemReset : nullptr);
    return 0;
}

int RequestHandler::request(Request* req) {
    const auto cmd = req->get("c").toString(); // Command
    if (cmd == "i") { // Init suite
        CHECK(initSuite(req));
    } else if (cmd == "l") { // List tests
        CHECK(listTests(req));
    } else if (cmd == "t") { // Start test
        CHECK(startTest(req));
    } else if (cmd == "s") { // Get status
        CHECK(getStatus(req));
    } else if (cmd == "L") { // Get log
        CHECK(getLog(req));
    } else {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    return 0;
}

int RequestHandler::initSuite(Request* req) {
    SuiteConfig conf;
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
        const auto val = req->get("t").toInt();
        conf.systemThreadEnabled((bool)val);
    }
    return Suite::instance()->config(conf);
}

int RequestHandler::listTests(Request* req) {
    CHECK(req->reply([](JSONWriter& w) {
        w.beginArray();
        Test::for_each([&w](const Test& t) {
            w.value((const char*)t.name.data);
        });
        w.endArray();
        return 0;
    }));
    return 0;
}

int RequestHandler::startTest(Request* req) {
    const auto name = req->get("t").toString(); // Test name
    CHECK_TRUE(!name.isEmpty(), SYSTEM_ERROR_INVALID_ARGUMENT);
    const auto runner = TestRunner::instance();
    runner->reset();
    const auto count = Test::include(name.data());
    CHECK_TRUE(count != 0, SYSTEM_ERROR_NOT_FOUND);
    runner->start();
    return 0;
}

int RequestHandler::getStatus(Request* req) {
    const char* status = "w"; // Waiting
    const auto runner = TestRunner::instance();
    if (runner->isComplete()) {
        if (Test::getCurrentPassed() > 0) {
            status = "p"; // Passed
        } else if (Test::getCurrentFailed() > 0) {
            status = "f"; // Failed
        } else {
            status = "s"; // Skipped
        }
    } else if (runner->isStarted()) {
        status = "r"; // Running
    }
    CHECK(req->reply([status](JSONWriter& w) {
        w.value(status);
        return 0;
    }));
    return 0;
}

int RequestHandler::getLog(Request* req) {
    CHECK(req->reply([](JSONWriter& w) {
        const auto runner = TestRunner::instance();
        const auto data = runner->logBuffer();
        const auto size = runner->logSize();
        w.value(data, size);
        return 0;
    }));
    return 0;
}

RequestHandler* RequestHandler::instance() {
    static RequestHandler handler;
    return &handler;
}

} // namespace test

} // namespace particle

void ctrl_request_custom_handler(ctrl_request* req) {
    const int r = particle::test::RequestHandler::instance()->request(req);
    if (r < 0) {
        // Handle an early processing error
        system_ctrl_set_result(req, r, nullptr, nullptr, nullptr);
    }
}
