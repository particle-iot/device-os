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

#include "firmware_update.h"

#if HAL_PLATFORM_OTA_PROTOCOL_V3

namespace particle::protocol {

FirmwareUpdate::FirmwareUpdate(MessageChannel* channel, FirmwareUpdateCallbacks callbacks) :
        ctx_(callbacks.userData()),
        callbacks_(std::move(callbacks)),
        channel_(channel),
        state_(State::IDLE) {
    reset();
}

FirmwareUpdate::~FirmwareUpdate() {
}

int FirmwareUpdate::receiveBegin(Message* msg) {
    return -1;
}

int FirmwareUpdate::receiveEnd(Message* msg) {
    return -1;
}

int FirmwareUpdate::receiveChunk(Message* msg) {
    return -1;
}

int FirmwareUpdate::receiveAck(Message* msg) {
    return -1;
}

int FirmwareUpdate::process() {
    return -1;
}

void FirmwareUpdate::reset() {
    if (state_ != State::IDLE) {
        finishUpdate(FirmwareUpdateFlag::CANCEL);
        state_ = State::IDLE;
    }
    ctx_.reset();
}

} // namespace particle::protocol

#endif // HAL_PLATFORM_OTA_PROTOCOL_V3
