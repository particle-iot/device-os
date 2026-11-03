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

#include "system_cloud_user_requests.h"
#include "system_task.h"
#include "system_error.h"

#include <catch2/catch.hpp>

#include <deque>
#include <new>
#include <string>
#include <utility>
#include <cstdlib>
#include <cstring>

namespace {

// Tasks scheduled via application_thread_invoke() run when the test runs the application thread
std::deque<std::pair<void (*)(void*), void*>> g_appQueue;
size_t g_appQueueSize = 20;

// Blocks allocated with new(std::nothrow) that haven't been freed yet
const size_t MAX_TRACKED_ALLOCS = 64;
void* g_allocs[MAX_TRACKED_ALLOCS] = {};

} // namespace

extern "C" uint8_t application_thread_invoke(void (*callback)(void* data), void* data, void* reserved) {
    if (g_appQueue.size() >= g_appQueueSize) {
        return 1;
    }
    g_appQueue.emplace_back(callback, data);
    return 0;
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    auto ptr = std::malloc(size ? size : 1);
    for (auto& p: g_allocs) {
        if (!p) {
            p = ptr;
            break;
        }
    }
    return ptr;
}

void operator delete(void* ptr) noexcept {
    for (auto& p: g_allocs) {
        if (p == ptr) {
            p = nullptr;
            break;
        }
    }
    std::free(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept {
    operator delete(ptr);
}

namespace {

using particle::protocol::ProtocolError;

size_t liveAllocs() {
    size_t n = 0;
    for (auto p: g_allocs) {
        if (p) {
            ++n;
        }
    }
    return n;
}

void runAppThread() {
    while (!g_appQueue.empty()) {
        auto task = g_appQueue.front();
        g_appQueue.pop_front();
        task.first(task.second);
    }
}

struct VariableResult {
    int calls = 0;
    int error = 0;
    int type = 0;
    std::string data;
};

// Mirrors protocol::Variables::get_variable_callback(), which takes the ownership over the data
void variableCallback(int error, int type, void* data, size_t size, void* context) {
    auto result = static_cast<VariableResult*>(context);
    ++result->calls;
    result->error = error;
    result->type = type;
    if (data) {
        result->data.assign((const char*)data, size);
    }
    std::free(data);
}

struct FunctionCall {
    int calls = 0;
    std::string arg;
};

int userFunction(void* data, const char* arg, void* reserved) {
    auto call = static_cast<FunctionCall*>(data);
    ++call->calls;
    call->arg = arg;
    return 7;
}

class UserRequestFixture {
public:
    UserRequestFixture() {
        g_appQueue.clear();
        g_appQueueSize = 20;
    }

    ~UserRequestFixture() {
        runAppThread();
    }
};

} // namespace

TEST_CASE_METHOD(UserRequestFixture, "Cloud variable requests are handled in the application thread", "[cloud]") {
    int value = 42;
    User_Var_Lookup_Table_t item = {};
    item.userVar = &value;
    item.userVarType = CLOUD_VAR_INT;
    std::strcpy(item.userVarKey, "var");
    VariableResult result;
    const auto allocs = liveAllocs();

    SECTION("the value is read in the application thread") {
        getUserVarValue(&item, variableCallback, &result);
        CHECK(result.calls == 0);
        CHECK(g_appQueue.size() == 1);
        runAppThread();
        CHECK(result.calls == 1);
        CHECK(result.error == ProtocolError::NO_ERROR);
        CHECK(result.type == SparkReturnType::INT);
        CHECK(result.data == std::string((const char*)&value, sizeof(value)));
        CHECK(liveAllocs() == allocs);
    }

    SECTION("the request fails if the application thread is busy") {
        g_appQueueSize = 0;
        getUserVarValue(&item, variableCallback, &result);
        CHECK(result.calls == 1);
        CHECK(result.error == ProtocolError::NO_MEMORY);
        CHECK(result.data.empty());
        CHECK(liveAllocs() == allocs);
    }
}

TEST_CASE_METHOD(UserRequestFixture, "Cloud functions are called in the application thread", "[cloud]") {
    FunctionCall call;
    User_Func_Lookup_Table_t item = {};
    item.pUserFuncData = &call;
    item.pUserFunc = userFunction;
    std::strcpy(item.userFuncKey, "fn");
    int results = 0;
    long resultValue = 0;
    auto callback = [&](const void* result, SparkReturnType::Enum type) {
        ++results;
        resultValue = (long)result;
        return true;
    };
    const auto allocs = liveAllocs();

    SECTION("the function gets a copy of the argument") {
        char arg[] = "hello";
        CHECK(callUserFunction(&item, arg, callback) == 0);
        arg[0] = 'X';
        CHECK(call.calls == 0);
        runAppThread();
        CHECK(call.calls == 1);
        CHECK(call.arg == "hello");
        CHECK(results == 1);
        CHECK(resultValue == 7);
        CHECK(liveAllocs() == allocs);
    }

    SECTION("the call fails if the application thread is busy") {
        g_appQueueSize = 0;
        CHECK(callUserFunction(&item, "hello", callback) == SYSTEM_ERROR_NO_MEMORY);
        CHECK(call.calls == 0);
        CHECK(results == 0);
        CHECK(liveAllocs() == allocs);
    }
}
