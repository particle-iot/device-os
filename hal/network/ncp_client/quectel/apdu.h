/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
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

#include <cstddef>
#include <cstdint>

#include "system_error.h"

namespace particle {

// Sends a single APDU and returns 0 on success, negative on error. respSize is in/out: the size of
// resp on the way in, and the number of bytes written (data plus the two status bytes) on the way
// out.
typedef int (*SendApduFn)(void* ctx, const char* cmd, size_t cmdSize, char* resp, size_t& respSize);

class Apdu {
public:
    Apdu(SendApduFn send, void* ctx) :
            send_(send),
            ctx_(ctx),
            sw_(0) {
    }

    // Sends one command and leaves the response data (without the status bytes) in resp, with its
    // size in respSize. The status word is available via sw().
    int sendCmd(const char* cmd, size_t cmdSize, char* resp, size_t& respSize) {
        size_t size = respSize;
        int r = send_(ctx_, cmd, cmdSize, resp, size);
        if (r < 0) {
            return r;
        }
        if (size < 2) {
            return SYSTEM_ERROR_BAD_DATA;
        }
        sw_ = (((unsigned char)resp[size - 2]) << 8) | (unsigned char)resp[size - 1];
        respSize = size - 2;
        return 0;
    }

    // Fetches the SW2 bytes a 61 XX status says are waiting
    int getResponse(uint8_t cla, char* resp, size_t& respSize) {
        const char cmd[] = { (char)cla, (char)0xc0, 0x00, 0x00, (char)sw2() };
        return sendCmd(cmd, sizeof(cmd), resp, respSize);
    }

    unsigned sw() const {
        return sw_;
    }

    unsigned sw1() const {
        return sw_ >> 8;
    }

    unsigned sw2() const {
        return sw_ & 0xff;
    }

private:
    SendApduFn send_;
    void* ctx_;
    unsigned sw_;
};

} // particle
