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

#include "spark_wiring_cloud_event.h"
#include "spark_wiring_cloud.h"

#include "util/coap_api_fake.h"

#include "system_task.h"
#include "concurrent_hal.h"
#include "system_error.h"

#include <catch2/catch.hpp>

#include <deque>
#include <utility>
#include <vector>

namespace {

// Tasks scheduled via application_thread_invoke() run when the test runs the application thread
std::deque<std::pair<void (*)(void*), void*>> g_appQueue;
size_t g_appQueueSize = 20;
bool g_inAppThread = false;
int g_failedInvokes = 0;

} // namespace

extern "C" uint8_t application_thread_invoke(void (*callback)(void* data), void* data, void* reserved) {
    if (g_inAppThread) {
        callback(data);
        return 0;
    }
    if (g_appQueue.size() >= g_appQueueSize) {
        ++g_failedInvokes;
        return 1;
    }
    g_appQueue.emplace_back(callback, data);
    return 0;
}

extern "C" int os_mutex_recursive_create(os_mutex_recursive_t* mutex) {
    *mutex = (os_mutex_recursive_t)1;
    return 0;
}

extern "C" int os_mutex_recursive_destroy(os_mutex_recursive_t mutex) {
    return 0;
}

extern "C" int os_mutex_recursive_lock(os_mutex_recursive_t mutex) {
    return 0;
}

extern "C" int os_mutex_recursive_unlock(os_mutex_recursive_t mutex) {
    return 0;
}

// spark_wiring_cloud.cpp is not linked as it depends on a large part of the system layer
bool CloudClass::publish(particle::CloudEvent event) {
    return event.publish() == 0;
}

CloudClass Particle;

namespace particle::test {

namespace {

template<typename F>
void inAppThread(F fn) {
    auto prev = g_inAppThread;
    g_inAppThread = true;
    fn();
    g_inAppThread = prev;
}

void runAppThread() {
    inAppThread([]() {
        while (!g_appQueue.empty()) {
            auto task = g_appQueue.front();
            g_appQueue.pop_front();
            task.first(task.second);
        }
    });
}

class CloudEventFixture {
public:
    int sent = 0;
    int failed = 0;

    CloudEventFixture() {
        g_appQueue.clear();
        g_appQueueSize = 20;
        g_failedInvokes = 0;
        CoapFake::instance().connect();
        CoapFake::instance().clearLog();
    }

    ~CloudEventFixture() {
        g_appQueueSize = 20;
        CoapFake::instance().disconnect();
        runAppThread();
        CoapFake::instance().clearLog();
    }

    CloudEvent makeEvent() {
        CloudEvent event;
        event.name("test");
        event.data("x");
        event.onStatusChange([this](CloudEvent event) {
            CHECK(g_inAppThread);
            if (event.isSent()) {
                ++sent;
            } else if (event.status() == CloudEvent::Status::FAILED) {
                ++failed;
            }
        });
        return event;
    }

    int publish(CloudEvent& event) {
        bool ok = false;
        inAppThread([&]() {
            ok = Particle.publish(event);
        });
        return ok ? 0 : event.error();
    }

    std::vector<CloudEvent> publishMany(int count) {
        std::vector<CloudEvent> events;
        for (int i = 0; i < count; ++i) {
            auto event = makeEvent();
            REQUIRE(publish(event) == 0);
            events.push_back(event);
        }
        return events;
    }

    // ID of the CoAP request sent for the N-th published event
    int requestId(size_t index) const {
        auto& reqs = CoapFake::instance().requests();
        REQUIRE(index < reqs.size());
        return reqs[index].reqId;
    }
};

// Events of 1 byte each use one 1 KB block of the 32 KB allowance
const int MAX_EVENTS_IN_FLIGHT = 32;

} // namespace

TEST_CASE("CloudEvent is completed in the application thread when the server acknowledges it", "[cloud_event]") {
    CloudEventFixture fixture;
    auto events = fixture.publishMany(2);
    CoapFake::instance().acknowledge(fixture.requestId(0));
    CoapFake::instance().acknowledge(fixture.requestId(1));
    CHECK(events[0].isSending());
    CHECK(g_appQueue.size() == 1);
    runAppThread();
    CHECK(events[0].isSent());
    CHECK(events[1].isSent());
    CHECK(fixture.sent == 2);
}

TEST_CASE("CloudEvent completions after a disconnect use one application queue slot", "[cloud_event]") {
    CloudEventFixture fixture;
    auto events = fixture.publishMany(MAX_EVENTS_IN_FLIGHT);
    CHECK_FALSE(CloudEvent::canPublish(1));
    CoapFake::instance().disconnect();
    CHECK(g_appQueue.size() == 1);
    CHECK(g_failedInvokes == 0);
    runAppThread();
    CHECK(fixture.failed == MAX_EVENTS_IN_FLIGHT);
    for (auto& event: events) {
        CHECK(event.status() == CloudEvent::Status::FAILED);
        CHECK(event.error() == SYSTEM_ERROR_COAP_CONNECTION_CLOSED);
    }
    CHECK(CloudEvent::canPublish(1));
}

TEST_CASE("CloudEvent retries scheduling the completions", "[cloud_event]") {
    SECTION("when another event is completed") {
        CloudEventFixture fixture;
        auto events = fixture.publishMany(2);
        g_appQueueSize = 0;
        CoapFake::instance().acknowledge(fixture.requestId(0));
        CHECK(g_failedInvokes == 1);
        runAppThread();
        CHECK(events[0].isSending());
        g_appQueueSize = 20;
        CoapFake::instance().acknowledge(fixture.requestId(1));
        runAppThread();
        CHECK(events[0].isSent());
        CHECK(events[1].isSent());
        CHECK(fixture.sent == 2);
    }

    SECTION("when an event is published in the application thread") {
        CloudEventFixture fixture;
        auto events = fixture.publishMany(MAX_EVENTS_IN_FLIGHT);
        g_appQueueSize = 0;
        CoapFake::instance().disconnect();
        CHECK(g_failedInvokes > 0);
        CHECK(g_appQueue.empty());
        CHECK_FALSE(CloudEvent::canPublish(1));
        CoapFake::instance().connect();
        auto event = fixture.makeEvent();
        CHECK(fixture.publish(event) == 0);
        CHECK(fixture.failed == MAX_EVENTS_IN_FLIGHT);
        CHECK(event.isSending());
    }
}

TEST_CASE("CloudEvent can be published again from its status change callback", "[cloud_event]") {
    SECTION("when publishing fails synchronously") {
        CloudEventFixture fixture;
        CoapFake::instance().disconnect();
        int callbacks = 0;
        CloudEvent event;
        event.name("test");
        event.data("x");
        event.onStatusChange([&](CloudEvent event) {
            ++callbacks;
            if (event.status() == CloudEvent::Status::FAILED && callbacks < 100) {
                Particle.publish(event);
            }
        });
        CHECK(fixture.publish(event) == SYSTEM_ERROR_COAP_CONNECTION_CLOSED);
        CHECK(callbacks == 1);
        CHECK(event.status() == CloudEvent::Status::FAILED);
        CHECK(CloudEvent::canPublish(MAX_EVENTS_IN_FLIGHT * COAP_BLOCK_SIZE));
    }

    SECTION("when the event failed after a disconnect") {
        CloudEventFixture fixture;
        int callbacks = 0;
        int callbacksWithAllowance = 0;
        std::vector<CloudEvent> events;
        for (int i = 0; i < MAX_EVENTS_IN_FLIGHT; ++i) {
            CloudEvent event;
            event.name("test");
            event.data("x");
            event.onStatusChange([&](CloudEvent event) {
                if (event.status() == CloudEvent::Status::FAILED && event.error() == SYSTEM_ERROR_COAP_CONNECTION_CLOSED) {
                    ++callbacks;
                    if (CloudEvent::canPublish(1)) {
                        ++callbacksWithAllowance;
                    }
                    Particle.publish(event);
                }
            });
            REQUIRE(fixture.publish(event) == 0);
            events.push_back(event);
        }
        CoapFake::instance().disconnect();
        runAppThread();
        CHECK(callbacks == MAX_EVENTS_IN_FLIGHT);
        CHECK(callbacksWithAllowance == MAX_EVENTS_IN_FLIGHT);
        for (auto& event: events) {
            CHECK(event.status() == CloudEvent::Status::FAILED);
            CHECK(event.error() == SYSTEM_ERROR_COAP_CONNECTION_CLOSED);
        }
        CHECK(CloudEvent::canPublish(MAX_EVENTS_IN_FLIGHT * COAP_BLOCK_SIZE));
    }
}

TEST_CASE("CloudEvent cancelled while awaiting completion returns its allowance once", "[cloud_event]") {
    CloudEventFixture fixture;
    auto events = fixture.publishMany(3);
    g_appQueueSize = 0;
    CoapFake::instance().acknowledge(fixture.requestId(0));
    inAppThread([&]() {
        events[0].cancel();
    });
    CHECK(events[0].status() == CloudEvent::Status::INVALID);
    g_appQueueSize = 20;
    CoapFake::instance().acknowledge(fixture.requestId(1));
    runAppThread();
    CHECK(events[0].status() == CloudEvent::Status::INVALID);
    CHECK(events[1].isSent());
    CHECK(fixture.sent == 1);
    // Only the third event is in flight
    fixture.publishMany(MAX_EVENTS_IN_FLIGHT - 1);
    CHECK_FALSE(CloudEvent::canPublish(1));
}

} // namespace particle::test
