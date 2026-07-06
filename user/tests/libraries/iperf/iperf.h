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
#include "inet_hal_compat.h"
#include "system_defs.h"
#include "hal_platform.h"

#if HAL_PLATFORM_NRF52840
#define IPERF_SERVER_THREAD_STACK_SIZE (4 * 1024)
#else
#define IPERF_SERVER_THREAD_STACK_SIZE (8 * 1024)
#endif

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


class IperfClient {
public:
    IperfClient();
    ~IperfClient();

    IperfClient(const IperfClient&) = delete;
    IperfClient& operator=(const IperfClient&) = delete;

    /**
     * Runs one iperf3 client test against the given server. Blocks until the
     * test completes or an error occurs.
     *
     * @param host Server hostname or IP.
     * @param port Server port (default 5201).
     * @return 0 on success, negative on error.
     */
    int run(const char* host, uint16_t port = IperfServer::DEFAULT_PORT);

    IperfClient& udp(bool enabled = true);
    IperfClient& reverse(bool enabled = true);
    IperfClient& bitrate(int64_t bps);
    IperfClient& time(int seconds);
    IperfClient& jsonOutput(bool enabled);

    /**
     * Sets the network interface to use for determining the MTU for blksize
     * calculation. If NETWORK_INTERFACE_ALL (default), the lowest MTU among
     * all UP interfaces is used.
     */
    IperfClient& network(network_interface_t iface);

    /**
     * Sets the callback receiving the JSON results document after the test.
     * The document is only valid for the duration of the call.
     */
    IperfClient& onResults(std::function<void(const char* json)> callback);

    IperfClient& quiet(bool enabled = true);

private:
    int runImpl(const char* host, uint16_t port);

    std::function<void(const char*)> resultsCallback_;
    bool udp_ = false;
    bool reverse_ = false;
    int64_t bitrate_ = 0;
    int time_ = 10;
    bool jsonOutput_ = false;
    bool quiet_ = false;
    network_interface_t network_ = NETWORK_INTERFACE_ALL;
};

} // namespace particle
