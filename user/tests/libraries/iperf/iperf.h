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

#pragma once

#include <cstdint>
#include <functional>

#include "concurrent_hal.h"

struct iperf_test;

namespace particle {

class IperfServer {
public:
    static constexpr uint16_t DEFAULT_PORT = 5201;

    IperfServer();
    ~IperfServer();

    IperfServer(const IperfServer&) = delete;
    IperfServer& operator=(const IperfServer&) = delete;

    /**
     * Starts the iperf server in a separate thread, accepting tests on all
     * interfaces. The server output is forwarded to the logging system.
     * Handles one test at a time and keeps serving until stop() is called.
     */
    int start(uint16_t port = DEFAULT_PORT);

    /**
     * Stops the server. Interrupts an actively running test at the next
     * reporting interval; if the server is idling waiting for a client,
     * the stop takes effect when the next test starts.
     */
    void stop();

    bool isRunning() const;

    /**
     * Enables machine readable output: instead of logging the human readable
     * reports, the results of each test are collected into a JSON document
     * and passed to the callback set with onResults(). Call before start().
     */
    IperfServer& jsonOutput(bool enabled);

    /**
     * Sets the callback receiving the JSON results document of each test
     * (see jsonOutput()). The callback is invoked from the server thread and
     * the document is only valid for the duration of the call.
     */
    IperfServer& onResults(std::function<void(const char* json)> callback);

    IperfServer& quiet(bool enabled = true);

    bool isQuiet() const { return quiet_; }

    IperfServer& priority(os_thread_prio_t prio);

private:
    void run();

    os_thread_t thread_ = nullptr;
    os_mutex_t mutex_ = nullptr;
    struct iperf_test* test_ = nullptr;
    std::function<void(const char*)> resultsCallback_;
    bool jsonOutput_ = false;
    bool quiet_ = false;
    os_thread_prio_t priority_ = OS_THREAD_PRIORITY_DEFAULT + 1;
    volatile bool running_ = false;
    volatile bool stop_ = false;
    uint16_t port_ = DEFAULT_PORT;
};

} // namespace particle
