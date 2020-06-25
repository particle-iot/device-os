/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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
#include <memory>
#include "socket_hal.h"
#include "system_tick_hal.h"

namespace particle {

// Forward declaration
class UdpSocket;

class SimpleNtpClient {
public:
    SimpleNtpClient();
    explicit SimpleNtpClient(sock_handle_t sock);
    ~SimpleNtpClient();

    int ntpDate(uint64_t* timestamp, const char* hostname = nullptr, system_tick_t timeout = 10000);

private:
    std::unique_ptr<UdpSocket> sock_;
};

} // particle
