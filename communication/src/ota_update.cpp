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

#if HAL_PLATFORM_OTA_PROTOCOL_V3

#include "scope_guard.h"

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
        finishUpdate(FirmwareUpdateFlag::CANCEL);
        state_ = State::IDLE;
    }
    ctx_.reset();
}

} // namespace particle::protocol

#endif // HAL_PLATFORM_OTA_PROTOCOL_V3
