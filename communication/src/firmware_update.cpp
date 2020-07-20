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

#include "coap_message_decoder.h"

#include "sha256.h"
#include "logging.h"
#include "check.h"

LOG_SOURCE_CATEGORY("comm.ota")

namespace particle::protocol {

namespace {

// Protocol-specific CoAP options
enum MessageOption {
    CHUNK_INDEX = 2049,
    WINDOW_SIZE = 2053,
    FILE_SIZE = 2057,
    FILE_SHA_256 = 2061,
    CHUNK_SIZE = 2065,
    DISCARD_DATA = 2069,
    CANCEL_UPDATE = 2073
};

} // namespace

FirmwareUpdate::FirmwareUpdate(MessageChannel* channel, FirmwareUpdateCallbacks callbacks) :
        ctx_(callbacks.userData()),
        callbacks_(std::move(callbacks)),
        channel_(channel),
        updating_(false) {
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

int FirmwareUpdate::handleBeginRequest(const Message* msg) {
    updateStartTime_ = millis();
    if (updating_) {
        LOG(WARN, "Received another Begin message while performing an update");
    } else {
        LOG(INFO, "Received Begin message");
    }
    reset();
    // Parse message
    const char* fileHash = nullptr;
    size_t fileSize = 0;
    size_t chunkSize = 0;
    bool discardData = false;
    CHECK(decodeBeginMessage(msg, &fileHash, &fileSize, &chunkSize, &discardData));
    // Start the update
    FirmwareUpdateFlags flags;
    if (discardData) {
        flags |= FirmwareUpdateFlag::DISCARD_DATA;
    }
    if (!fileHash) {
        flags |= FirmwareUpdateFlag::NON_RESUMABLE;
    }
    size_t fileOffset = 0;
    LOG(INFO, "Starting firmware update");
    LOG(INFO, "File size: %u", (unsigned)fileSize);
    LOG(INFO, "Chunk size: %u", (unsigned)chunkSize);
    LOG(INFO, "Discard data: %u", (unsigned)discardData);
    if (fileHash) {
        LOG(INFO, "File checksum:");
        LOG_DUMP(INFO, fileHash, Sha256::HASH_SIZE);
        LOG_PRINT(INFO, "\r\n");
    }
    CHECK(startUpdate(fileSize, fileHash, &fileOffset, flags));
    LOG(INFO, "File offset: %u", (unsigned)fileOffset);
    LOG(INFO, "Chunk count: %u", (unsigned)((fileSize - fileOffset + chunkSize - 1) / chunkSize));
    updating_ = true;
    return 0;
}

int FirmwareUpdate::decodeBeginRequest(const Message* msg, const char** fileHash, size_t* fileSize, size_t* chunkSize,
        bool* discardData, token_t* token, message_id_t* id) {
    CoapMessageDecoder d;
    CHECK(d.decode(msg->buf(), msg->length()));
    if (d.tokenSize() != sizeof(token_t)) {
        LOG(ERROR, "Invalid token size");
        return SYSTEM_ERROR_COAP;
    }
    bool hasFileSize = false;
    bool hasFileHash = false;
    bool hasChunkSize = false;
    bool hasDiscardData = false;
    auto it = d.options();
    while (it.next()) {
        switch (it.option) {
        case MessageOption::FILE_SIZE: {
            const size_t size = it.toUInt();
            if (!size) {
                LOG(ERROR, "Invalid file size: %u", (unsigned)size);
                return SYSTEM_ERROR_COAP;
            }
            *fileSize = size;
            hasFileSize = true;
            break;
        }
        case MessageOption::FILE_SHA_256: {
            if (it.size() != Sha256::HASH_SIZE) {
                LOG(ERROR, "Invalid option size");
                return SYSTEM_ERROR_COAP;
            }
            *fileHash = it.data();
            hasFileHash = true;
            break;
        }
        case MessageOption::CHUNK_SIZE: {
            const size_t size = it.toUInt();
            if (!size || size < MIN_CHUNK_SIZE || size > MAX_CHUNK_SIZE) {
                LOG(ERROR, "Invalid chunk size: %u", (unsigned)size);
                return SYSTEM_ERROR_COAP;
            }
            *chunkSize = size;
            hasChunkSize = true;
            break;
        }
        case MessageOption::DISCARD_DATA: {
            if (it.size() != 0) {
                LOG(ERROR, "Invalid option size");
                return SYSTEM_ERROR_COAP;
            }
            *discardData = true;
            hasDiscardData = true;
            break;
        }
        default:
            break;
        }
    }
    if (!hasFileSize || !hasChunkSize) {
        LOG(ERROR, "Mandatory option is missing");
        return SYSTEM_ERROR_COAP;
    }
    if (!hasFileHash) {
        *fileHash = nullptr;
    }
    if (!hasDiscardData) {
        *discardData = false;
    }
    memcpy(token, d.token(), sizeof(token_t));
    *id = d.id();
    return 0;
}

int FirmwareUpdate::decodeEndRequest(const Message* msg, bool* discardData, bool* cancelUpdate) {
    CoapMessageDecoder d;
    CHECK(d.decode(msg->buf(), msg->length()));
    bool hasDiscardData = false;
    bool hasCancelUpdate = false;
    auto it = d.options();
    while (it.next()) {
        switch (it.option) {
        case MessageOption::DISCARD_DATA: {
            if (it.size() != 0) {
                LOG(ERROR, "Invalid option size");
                return SYSTEM_ERROR_COAP;
            }
            *discardData = true;
            hasDiscardData = true;
            break;
        }
        case MessageOption::CANCEL_UPDATE: {
            if (it.size() != 0) {
                LOG(ERROR, "Invalid option size");
                return SYSTEM_ERROR_COAP;
            }
            *cancelUpdate = true;
            hasCancelUpdate = true;
            break;
        }
        default:
            break;
        }
    }
    if (!hasDiscardData) {
        *discardData = false;
    }
    if (!hasDiscardData) {
        *cancelUpdate = false;
    }
    return 0;
}

int FirmwareUpdate::decodeChunkRequest(const Message* msg, const char** chunkData, size_t* chunkSize, size_t* chunkIndex) {
    CoapMessageDecoder d;
    CHECK(d.decode(msg->buf(), msg->length()));
    if (d.payloadSize() == 0) {
        LOG(ERROR, "Invalid payload size");
        return SYSTEM_ERROR_COAP;
    }
    auto it = d.findOption(MessageOption::CHUNK_INDEX);
    if (!it) {
        LOG(ERROR, "Mandatory option is missing");
        return SYSTEM_ERROR_COAP;
    }
    *chunkIndex = it.toUInt();
    *chunkData = d.payload();
    *chunkSize = d.payloadSize();
    return 0;
}

void FirmwareUpdate::reset() {
    if (state_ != State::IDLE) {
        const int r = finishUpdate(FirmwareUpdateFlag::CANCEL);
        if (r < 0) {
            LOG(ERROR, "finishUpdate() failed: %d", r);
        }
        state_ = State::IDLE;
    }
    ctx_.reset();
}

} // namespace particle::protocol

#endif // HAL_PLATFORM_OTA_PROTOCOL_V3
