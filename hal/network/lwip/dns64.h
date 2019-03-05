/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#include "ifapi.h"

#include "runnable.h"

#include "lwip/ip_addr.h"

#include <memory>

namespace particle {

namespace net {

class Dns64: public Runnable {
public:
    static const uint16_t DEFAULT_PORT = 53;

    Dns64() = default;
    ~Dns64();

    int init(if_t iface, const ip6_addr_t& prefix, uint16_t port = DEFAULT_PORT);
    void destroy();

    int run() override;

private:
    enum GetHostByNameResult {
        DONE,
        PENDING
    };

    struct Context;
    struct Query;

    std::shared_ptr<Context> ctx_;
    std::unique_ptr<char[]> buf_;

    int processQuery(char* data, size_t size, const sockaddr_in6& srcAddr);
    static int parseQuery(char* data, size_t size, Query* q, const char** name);

    static int sendResponse(const ip_addr_t& addr, const char* name, const Query& q, Context* ctx);
    static int sendErrorResponse(int error, const char* name, const Query& q, Context* ctx);

    static int getHostByName(const char* name, ip_addr_t* addr, Query* q);

    static void dnsCallback(const char* name, const ip_addr_t* addr, void* data);
};

inline Dns64::~Dns64() {
    destroy();
}

} // particle::net

} // particle
