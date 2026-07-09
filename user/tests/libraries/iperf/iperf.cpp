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
#include "iperf_time.h"
#include "logging.h"
#include "delay_hal.h"
#include "ifapi.h"
#include "system_defs.h"

extern "C" void iperf_particle_interrupt(struct iperf_test* test);
extern "C" void iperf_particle_force_ipv4(struct iperf_test* test);

// These setters exist in iperf_api.c but aren't declared in iperf_api.h
extern "C" void iperf_set_test_idle_timeout(struct iperf_test* ipt, int to);
extern "C" void iperf_set_test_rcv_timeout(struct iperf_test* ipt, struct iperf_time* to);

namespace particle {

namespace {

const auto IPERF_THREAD_NAME = "iperf";
const size_t IPERF_THREAD_STACK_SIZE = IPERF_SERVER_THREAD_STACK_SIZE;
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
    // Create a fresh test object for each iteration. Reusing the test object
    // leaves the listening socket in a bad state when a client crashes
    // mid-test (lwIP select() doesn't honor timeouts, so the server can't
    // recover from a dead client connection).
    for (unsigned round = 1; !stop_; round++) {
        auto test = iperf_new_test();
        if (!test) {
            LOG(ERROR, "iperf_new_test() failed");
            HAL_Delay_Milliseconds(IPERF_ERROR_RETRY_DELAY_MS);
            continue;
        }
        iperf_defaults(test);
        // Force IPv4 - platforms without LWIP_IPV6 (e.g. Gen 3) can't create the
        // AF_INET6 wildcard listener netannounce() defaults to, which otherwise
        // fails with IELISTEN forever.
        iperf_particle_force_ipv4(test);
        iperf_set_test_role(test, 's');
        iperf_set_test_server_port(test, port_);
        iperf_set_test_output_callback(test, outputCallback, this);
        iperf_set_test_idle_timeout(test, 10);
        struct iperf_time rcv_to = { .secs = 10, .usecs = 0 };
        iperf_set_test_rcv_timeout(test, &rcv_to);
        if (jsonOutput_) {
            iperf_set_test_json_output(test, 1);
            iperf_set_test_json_callback(test, [](struct iperf_test*, char*) {});
        }
        os_mutex_lock(mutex_);
        test_ = test;
        os_mutex_unlock(mutex_);

        LOG(INFO, "iperf server: waiting for client (test #%u)", round);
        int r = 0;
        jmp_buf exitJmp;
        if (setjmp(exitJmp) == 0) {
            iperf_set_test_exit_jmp_buf(test, &exitJmp);
            r = iperf_run_server(test);
        } else {
            r = -1;
        }
        iperf_set_test_exit_jmp_buf(test, nullptr);
        LOG(INFO, "iperf server: test #%u finished, r=%d, i_errno=%d", round, r, i_errno);
        if (r < 0 && !stop_) {
            LOG(ERROR, "iperf server error: %s", iperf_strerror(i_errno));
            // Back off before retrying so a persistent setup failure (e.g. the
            // listener socket can't be created) doesn't tight-loop and peg the CPU.
            HAL_Delay_Milliseconds(IPERF_ERROR_RETRY_DELAY_MS);
        }
        if (jsonOutput_ && resultsCallback_ && r >= 0) {
            auto json = iperf_get_test_json_output_string(test);
            if (json) {
                LOG(INFO, "iperf server: sending JSON results (%u bytes)", (unsigned)strlen(json));
                resultsCallback_(json);
            } else {
                LOG(WARN, "iperf server: no JSON output string available");
            }
        }
        os_mutex_lock(mutex_);
        test_ = nullptr;
        os_mutex_unlock(mutex_);
        iperf_free_test(test);
    }
    running_ = false;
}

IperfClient::IperfClient() {
}

IperfClient::~IperfClient() {
}

IperfClient& IperfClient::udp(bool enabled) {
    udp_ = enabled;
    return *this;
}

IperfClient& IperfClient::reverse(bool enabled) {
    reverse_ = enabled;
    return *this;
}

IperfClient& IperfClient::bitrate(int64_t bps) {
    bitrate_ = bps;
    return *this;
}

IperfClient& IperfClient::time(int seconds) {
    time_ = seconds;
    return *this;
}

IperfClient& IperfClient::jsonOutput(bool enabled) {
    jsonOutput_ = enabled;
    return *this;
}

IperfClient& IperfClient::onResults(std::function<void(const char* json)> callback) {
    resultsCallback_ = std::move(callback);
    return *this;
}

IperfClient& IperfClient::quiet(bool enabled) {
    quiet_ = enabled;
    return *this;
}

IperfClient& IperfClient::network(network_interface_t iface) {
    network_ = iface;
    return *this;
}

namespace {

struct ClientRunCtx {
    IperfClient* self;
    const char* host;
    uint16_t port;
    int result;
    os_semaphore_t done;
};

} // namespace

int IperfClient::run(const char* host, uint16_t port) {
    ClientRunCtx ctx = {};
    ctx.self = this;
    ctx.host = host;
    ctx.port = port;
    ctx.result = -1;
    if (os_semaphore_create(&ctx.done, 1, 0) != 0) {
        return -1;
    }
    os_thread_t thread = nullptr;
    if (os_thread_create(&thread, "iperfc", OS_THREAD_PRIORITY_DEFAULT + 1, [](void* p) {
                auto c = (ClientRunCtx*)p;
                c->result = c->self->runImpl(c->host, c->port);
                os_semaphore_give(c->done, false);
                os_thread_exit(nullptr);
            }, &ctx, IPERF_SERVER_THREAD_STACK_SIZE) != 0) {
        os_semaphore_destroy(ctx.done);
        return -1;
    }
    os_semaphore_take(ctx.done, CONCURRENT_WAIT_FOREVER, false);
    os_thread_join(thread);
    os_semaphore_destroy(ctx.done);
    return ctx.result;
}

int IperfClient::runImpl(const char* host, uint16_t port) {
    if (!host) {
        return -1;
    }
    LOG(INFO, "iperf client: connecting to %s:%u (udp=%d, reverse=%d, time=%d)",
        host, (unsigned)port, udp_, reverse_, time_);
    auto test = iperf_new_test();
    if (!test) {
        LOG(ERROR, "iperf_new_test() failed");
        return -1;
    }
    iperf_defaults(test);
    iperf_set_test_role(test, 'c');
    iperf_set_test_server_hostname(test, (char*)host);
    iperf_set_test_server_port(test, port);
    iperf_set_test_duration(test, time_);
    iperf_set_test_connect_timeout(test, 10000);

    iperf_particle_force_ipv4(test);
    if (udp_) {
        set_protocol(test, Pudp);
        if (bitrate_ > 0) {
            iperf_set_test_rate(test, bitrate_);
        }
        // UDP blksize: MTU - 100 (IP + UDP + large safety margin) just in case
        int blksize = 512; // Fallback
        unsigned int mtu = 0;
        if (network_ != NETWORK_INTERFACE_ALL) {
            if_t iface = nullptr;
            if (if_get_by_index(network_, &iface) == 0 && iface) {
                if (if_get_mtu(iface, &mtu) == 0 && mtu > 100) {
                    blksize = mtu - 100;
                }
            }
        } else {
            // Find lowest MTU among UP interfaces
            struct if_list* ifs = nullptr;
            if (if_get_list(&ifs) == 0 && ifs) {
                unsigned int minMtu = 0;
            for (auto cur = ifs; cur; cur = cur->next) {
                    unsigned int flags = 0;
                    if (if_get_flags(cur->iface, &flags) == 0 && (flags & IFF_UP)) {
                        unsigned int curMtu = 0;
                        if (if_get_mtu(cur->iface, &curMtu) == 0 && curMtu > 100) {
                            if (minMtu == 0 || curMtu < minMtu) {
                                minMtu = curMtu;
                            }
                        }
                    }
                }
                if_free_list(ifs);
                if (minMtu > 100) {
                    blksize = minMtu - 100;
                }
            }
        }
        // Cap the datagram at 1240 bytes on the wire (payload 1212 + 28 for
        // IP/UDP headers).
        constexpr int MAX_UDP_BLKSIZE = 1240 - 28;
        if (blksize > MAX_UDP_BLKSIZE) {
            blksize = MAX_UDP_BLKSIZE;
        }
        LOG(INFO, "iperf client: UDP blksize=%d (mtu=%u)", blksize, mtu);
        iperf_set_test_blksize(test, blksize);
    } else {
        // TCP blksize: use MTU - 40 (IP + TCP headers) as a reasonable block
        // size. This is small enough that the worker checks test->done
        // frequently, and matches what the TCP stack can send in one segment.
        int blksize = 1460; // Fallback
        unsigned int mtu = 0;
        if (network_ != NETWORK_INTERFACE_ALL) {
            if_t iface = nullptr;
            if (if_get_by_index(network_, &iface) == 0 && iface) {
                if (if_get_mtu(iface, &mtu) == 0 && mtu > 40) {
                    blksize = mtu - 40;
                }
            }
        } else {
            struct if_list* ifs = nullptr;
            if (if_get_list(&ifs) == 0 && ifs) {
                unsigned int minMtu = 0;
                for (auto cur = ifs; cur; cur = cur->next) {
                    unsigned int flags = 0;
                    if (if_get_flags(cur->iface, &flags) == 0 && (flags & IFF_UP)) {
                        unsigned int curMtu = 0;
                        if (if_get_mtu(cur->iface, &curMtu) == 0 && curMtu > 40) {
                            if (minMtu == 0 || curMtu < minMtu) {
                                minMtu = curMtu;
                            }
                        }
                    }
                }
                if_free_list(ifs);
                if (minMtu > 40) {
                    blksize = minMtu - 40;
                }
            }
        }
        LOG(INFO, "iperf client: TCP blksize=%d (mtu=%u)", blksize, mtu);
        iperf_set_test_blksize(test, blksize);
    }
    if (reverse_) {
        iperf_set_test_reverse(test, 1);
    }
    if (jsonOutput_) {
        iperf_set_test_json_output(test, 1);
        iperf_set_test_get_server_output(test, 1);
    }
    if (quiet_) {
        iperf_set_test_output_callback(test, [](struct iperf_test*, void*, const char*, va_list) -> int {
            return 0;
        }, nullptr);
    } else {
        iperf_set_test_output_callback(test, [](struct iperf_test*, void* ctx, const char* fmt, va_list ap) -> int {
            LogAttributes attr = {};
            log_message_v(1, "iperf", &attr, nullptr, fmt, ap);
            return 0;
        }, nullptr);
    }
    int r = 0;
    jmp_buf exitJmp;
    if (setjmp(exitJmp) == 0) {
        iperf_set_test_exit_jmp_buf(test, &exitJmp);
        LOG(INFO, "iperf client: starting test");
        r = iperf_run_client(test);
    } else {
        r = -1;
    }
    iperf_set_test_exit_jmp_buf(test, nullptr);
    LOG(INFO, "iperf client: test finished, r=%d, i_errno=%d", r, i_errno);
    if (r < 0) {
        LOG(ERROR, "iperf client error: %s", iperf_strerror(i_errno));
    }
    if (jsonOutput_ && resultsCallback_) {
        auto json = iperf_get_test_json_output_string(test);
        if (json) {
            LOG(INFO, "iperf client: got JSON output (%u bytes)", (unsigned)strlen(json));
            LOG_PRINT_C(TRACE, "iperf", json);
            resultsCallback_(json);
        } else {
            LOG(WARN, "iperf client: no JSON output string available");
        }
    }
    iperf_free_test(test);
    return r;
}

} // namespace particle
