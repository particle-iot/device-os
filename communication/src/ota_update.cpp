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

#include "ota_update.h"

#include "scope_guard.h"

#include <cstdio>
#include <cstdarg>

namespace particle::protocol {

OtaUpdate::OtaUpdate(MessageChannel* channel, OtaUpdateCallbacks callbacks) :
        ctx_(callbacks.userData()),
        callbacks_(std::move(callbacks)),
        channel_(channel),
        state_(State::IDLE) {
    reset();
}

OtaUpdate::~OtaUpdate() {
}

int OtaUpdate::receiveBegin(Message* msg) {
    return -1;
}

int OtaUpdate::receiveEnd(Message* msg) {
    return -1;
}

int OtaUpdate::receiveChunk(Message* msg) {
    return -1;
}

int OtaUpdate::receiveAck(Message* msg) {
    return -1;
}

int OtaUpdate::process() {
    return -1;
}

void OtaUpdate::reset() {
    if (state_ != State::IDLE) {
        command(OtaUpdateCommand::CANCEL_UPDATE);
        state_ = State::IDLE;
    }
    ctx_.reset();
    memset(chunkMap_, 0, sizeof(chunkMap_));
}

void OtaUpdateContext::formatErrorMessage(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    va_list args2;
    va_copy(args2, args);
    SCOPE_GUARD({
        va_end(args2);
        va_end(args);
    });
    const auto n = vsnprintf(nullptr, 0, fmt, args);
    if (n < 0) {
        return;
    }
    const auto buf = (char*)malloc(n + 1);
    if (!buf) {
        return;
    }
    vsnprintf(buf, n + 1, fmt, args2);
    errMsg_ = CString::wrap(buf);
}

} // namespace particle::protocol
