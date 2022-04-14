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

#include "protocol_callbacks.h"

namespace particle {

namespace protocol {

namespace test {

namespace {

ProtocolCallbacks* g_callbacks = nullptr;

int sendCallback(const uint8_t* buf, uint32_t bufSize, void* ctx) {
    if (g_callbacks) {
        return g_callbacks->send(buf, bufSize, ctx);
    }
    return -1;
}

int receiveCallback(uint8_t *buf, uint32_t bufSize, void* ctx) {
    if (g_callbacks) {
        return g_callbacks->receive(buf, bufSize, ctx);
    }
    return -1;
}

#if HAL_PLATFORM_OTA_PROTOCOL_V3

int startFirmwareUpdateCallback(size_t fileSize, const char* fileHash, size_t* fileOffset, unsigned flags) {
    if (g_callbacks) {
        return g_callbacks->startFirmwareUpdate(fileSize, fileHash, fileOffset, flags);
    }
    return -1;
}

int finishFirmwareUpdateCallback(unsigned flags) {
    if (g_callbacks) {
        return g_callbacks->finishFirmwareUpdate(flags);
    }
    return -1;
}

int saveFirmwareChunkCallback(const char* chunkData, size_t chunkSize, size_t chunkOffset, size_t partialSize) {
    if (g_callbacks) {
        return g_callbacks->saveFirmwareChunk(chunkData, chunkSize, chunkOffset, partialSize);
    }
    return -1;
}

#else

int prepareForFirmwareUpdateCallback(FileTransfer::Descriptor& data, uint32_t flags, void* reserved) {
    if (g_callbacks) {
        return g_callbacks->prepareForFirmwareUpdate(data, flags, reserved);
    }
    return -1;
}

int saveFirmwareChunkCallback(FileTransfer::Descriptor& descriptor, const uint8_t* chunk, void* reserved) {
    if (g_callbacks) {
        return g_callbacks->saveFirmwareChunk(descriptor, chunk, reserved);
    }
    return -1;
}

int finishFirmwareUpdateCallback(FileTransfer::Descriptor& data, uint32_t flags, void* reserved) {
    if (g_callbacks) {
        return g_callbacks->finishFirmwareUpdate(data, flags, reserved);
    }
    return -1;
}

#endif // !HAL_PLATFORM_OTA_PROTOCOL_V3

uint32_t calculateCrcCallback(const uint8_t *buf, uint32_t bufSize) {
    if (g_callbacks) {
        return g_callbacks->calculateCrc(buf, bufSize);
    }
    return 0;
}

void signalCallback(bool on, unsigned param, void* reserved) {
    if (g_callbacks) {
        return g_callbacks->signal(on, param, reserved);
    }
}

system_tick_t millisCallback() {
    if (g_callbacks) {
        return g_callbacks->millis();
    }
    return 0;
}

void setTimeCallback(uint32_t time, unsigned param, void* reserved) {
    if (g_callbacks) {
        g_callbacks->setTime(time, param, reserved);
    }
}

int saveCallback(const void* data, size_t size, uint8_t type, void* reserved) {
    if (g_callbacks) {
        return g_callbacks->save(data, size, type, reserved);
    }
    return -1;
}

int restoreCallback(void* data, size_t size, uint8_t type, void* reserved) {
    if (g_callbacks) {
        return g_callbacks->restore(data, size, type, reserved);
    }
    return -1;
}

void notifyClientMessagesProcessedCallback(void* reserved) {
    if (g_callbacks) {
        g_callbacks->notifyClientMessagesProcessed(reserved);
    }
}

} // namespace

ProtocolCallbacks::ProtocolCallbacks() :
        cb_(),
        millis_(0) {
    cb_.size = sizeof(cb_);
    cb_.protocolFactory = ProtocolFactory::PROTOCOL_DTLS;
    cb_.send = sendCallback;
    cb_.receive = receiveCallback;
#if HAL_PLATFORM_OTA_PROTOCOL_V3
    cb_.start_firmware_update = startFirmwareUpdateCallback;
    cb_.finish_firmware_update = finishFirmwareUpdateCallback;
    cb_.save_firmware_chunk = saveFirmwareChunkCallback;
#else
    cb_.prepare_for_firmware_update = prepareForFirmwareUpdateCallback;
    cb_.save_firmware_chunk = saveFirmwareChunkCallback;
    cb_.finish_firmware_update = finishFirmwareUpdateCallback;
#endif // !HAL_PLATFORM_OTA_PROTOCOL_V3
    cb_.calculate_crc = calculateCrcCallback;
    cb_.signal = signalCallback;
    cb_.millis = millisCallback;
    cb_.set_time = setTimeCallback;
    cb_.save = saveCallback;
    cb_.restore = restoreCallback;
    cb_.notify_client_messages_processed = notifyClientMessagesProcessedCallback;
    g_callbacks = this;
}

ProtocolCallbacks::~ProtocolCallbacks() {
    g_callbacks = nullptr;
}

} // namespace test

} // namespace protocol

} // namespace particle
