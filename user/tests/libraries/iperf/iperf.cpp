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

#include "iperf/iperf.h"

#include <cstdarg>
#include <csetjmp>

#include "iperf_api.h"
#include "logging.h"
#include "delay_hal.h"

extern "C" void iperf_particle_interrupt(struct iperf_test* test);

namespace particle {

namespace {

const auto IPERF_THREAD_NAME = "iperf";
const size_t IPERF_THREAD_STACK_SIZE = 8 * 1024;
const unsigned IPERF_ERROR_RETRY_DELAY_MS = 1000;

int outputCallback(struct iperf_test* test, void* ctx, const char* fmt, va_list ap) {
    auto self = static_cast<IperfServer*>(ctx);
    if (self && self->isQuiet()) {
        return 0;
    }
    LogAttributes attr = {};
    log_message_v(1, "iperf", &attr, nullptr /* reserved */, fmt, ap);
    return 0;
}

} // namespace

IperfServer::IperfServer() {
    os_mutex_create(&mutex_);
}

IperfServer::~IperfServer() {
    stop();
    if (mutex_) {
        os_mutex_destroy(mutex_);
        mutex_ = nullptr;
    }
}

int IperfServer::start(uint16_t port) {
    if (running_) {
        return -1;
    }
    port_ = port;
    stop_ = false;
    running_ = true;
    auto entry = [](void* arg) -> os_thread_return_t {
        static_cast<IperfServer*>(arg)->run();
        os_thread_exit(nullptr);
    };
    if (os_thread_create(&thread_, IPERF_THREAD_NAME, priority_, entry,
            this, IPERF_THREAD_STACK_SIZE) != 0) {
        running_ = false;
        thread_ = nullptr;
        return -1;
    }
    return 0;
}

void IperfServer::stop() {
    if (!thread_) {
        return;
    }
    stop_ = true;
    os_mutex_lock(mutex_);
    iperf_particle_interrupt(test_);
    os_mutex_unlock(mutex_);
    os_thread_join(thread_);
    os_thread_cleanup(thread_);
    thread_ = nullptr;
}

bool IperfServer::isRunning() const {
    return running_;
}

IperfServer& IperfServer::jsonOutput(bool enabled) {
    jsonOutput_ = enabled;
    return *this;
}

IperfServer& IperfServer::onResults(std::function<void(const char* json)> callback) {
    resultsCallback_ = std::move(callback);
    return *this;
}

IperfServer& IperfServer::quiet(bool enabled) {
    quiet_ = enabled;
    return *this;
}

IperfServer& IperfServer::priority(os_thread_prio_t prio) {
    priority_ = prio;
    return *this;
}

void IperfServer::run() {
    auto test = iperf_new_test();
    if (!test) {
        LOG(ERROR, "iperf_new_test() failed");
        running_ = false;
        return;
    }
    iperf_defaults(test);
    iperf_set_test_role(test, 's');
    iperf_set_test_server_port(test, port_);
    iperf_set_test_output_callback(test, outputCallback, this);
    if (jsonOutput_) {
        iperf_set_test_json_output(test, 1);
        // The results are passed to resultsCallback_ after each test, prevent
        // the JSON document from also being written to the regular output
        iperf_set_test_json_callback(test, [](struct iperf_test*, char*) {});
    }
    os_mutex_lock(mutex_);
    test_ = test;
    os_mutex_unlock(mutex_);
    // Same pattern as the upstream iperf3 server: the test object is reused
    // for consecutive tests, retaining the listening socket
    for (unsigned round = 1; !stop_; round++) {
        LOG(INFO, "Starting iperf server on port %u (test #%u)", (unsigned)port_, round);
        int r = 0;
        jmp_buf exitJmp;
        if (setjmp(exitJmp) == 0) {
            iperf_set_test_exit_jmp_buf(test, &exitJmp);
            r = iperf_run_server(test);
        } else {
            // libiperf attempted to terminate the process
            r = -1;
        }
        iperf_set_test_exit_jmp_buf(test, nullptr);
        if (r < 0 && !stop_) {
            LOG(ERROR, "iperf server error: %s", iperf_strerror(i_errno));
            HAL_Delay_Milliseconds(IPERF_ERROR_RETRY_DELAY_MS);
        }
        if (jsonOutput_ && resultsCallback_) {
            // Only valid until iperf_reset_test()
            auto json = iperf_get_test_json_output_string(test);
            if (json) {
                resultsCallback_(json);
            }
        }
        iperf_reset_test(test);
    }
    os_mutex_lock(mutex_);
    test_ = nullptr;
    os_mutex_unlock(mutex_);
    iperf_free_test(test);
    running_ = false;
}

} // namespace particle
