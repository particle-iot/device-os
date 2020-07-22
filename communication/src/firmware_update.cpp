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

#include "message_channel.h"
#include "coap_message_encoder.h"
#include "coap_message_decoder.h"

#include "sha256.h"
#include "logging.h"
#include "check.h"

#include <algorithm>

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

int FirmwareUpdate::process() {
    if (!updating_) {
        return 0;
    }
    return -1;
}

int FirmwareUpdate::handleRequest(Message* msg, RequestHandlerFn handler) {
    CoapMessageDecoder d;
    int r = d.decode((const char*)msg->buf(), msg->length());
    if (r < 0) {
        LOG(ERROR, "Failed to decode message: %d", r);
        return ProtocolError::MALFORMED_MESSAGE;
    }
    r = (this->*handler)(d, msg);
    if (r < 0) {
        // Reply with a piggybacked or separate non-confirmable response
        const auto type = d.type() == CoapType::CON ? CoapType::ACK : CoapType::NON;
        r = sendErrorResponse(r, type, d.id(), d.token(), d.tokenSize());
        if (r < 0) {
            LOG(ERROR, "sendErrorAck() failed: %d", r);
            return ProtocolError::IO_ERROR_GENERIC_SEND;
        }
        reset();
    }
    return 0;
}

int FirmwareUpdate::handleBeginRequest(const CoapMessageDecoder& d, Message* msg) {
    if (!updating_) {
        LOG(INFO, "Received Begin request");
    } else {
        LOG(WARN, "Received another Begin request while performing an update");
    }
    reset();
    updateStartTime_ = millis();
    // Parse message
    const char* fileHash = nullptr;
    bool discardData = false;
    CHECK(parseBeginRequest(d, &fileHash, &fileSize_, &chunkSize_, &discardData));
    // Start the update
    FirmwareUpdateFlags flags;
    if (discardData) {
        flags |= FirmwareUpdateFlag::DISCARD_DATA;
    }
    if (!fileHash) {
        flags |= FirmwareUpdateFlag::NON_RESUMABLE;
    }
    LOG(INFO, "Starting firmware update");
    LOG(INFO, "File size: %u", (unsigned)fileSize_);
    LOG(INFO, "Chunk size: %u", (unsigned)chunkSize_);
    LOG(INFO, "Discard data: %u", (unsigned)discardData);
    if (fileHash) {
        LOG(INFO, "File checksum:");
        LOG_DUMP(INFO, fileHash, Sha256::HASH_SIZE);
        LOG_PRINT(INFO, "\r\n");
    }
    CHECK(startUpdate(fileSize_, fileHash, &fileOffset_, flags));
    transferSize_ = fileSize_ - fileOffset_;
    chunkCount_ = (transferSize_ + chunkSize_ - 1) / chunkSize_;
    LOG(INFO, "File offset: %u", (unsigned)fileOffset_);
    LOG(INFO, "Chunk count: %u", (unsigned)chunkCount_);
    windowSize_ = OTA_RECEIVE_WINDOW_SIZE / chunkSize_;
#if OTA_UPDATE_STATS
    LOG(INFO, "Window size (chunks): %u", (unsigned)windowSize_);
#endif
    updating_ = true;
    return 0;
}

int FirmwareUpdate::handleEndRequest(const CoapMessageDecoder& d, Message* msg) {
    if (!updating_) {
        LOG(WARN, "Received End request but no update is in progress");
    }
    return -1;
}

int FirmwareUpdate::handleChunkRequest(const CoapMessageDecoder& d, Message* msg) {
    return -1;
}

int FirmwareUpdate::sendErrorResponse(int error, CoapType type, CoapMessageId id, const char* token, size_t tokenSize) {
    Message msg;
    if (channel_->create(msg) != 0) {
        LOG(ERROR, "Failed to create message");
        return SYSTEM_ERROR_NO_MEMORY;
    }
    CoapMessageEncoder e((char*)msg.buf(), msg.capacity());
    e.type(type);
    e.code(coapCodeForSystemError(error));
    e.id(type == CoapType::ACK ? id : 0); // Message channel will assign an ID to this message if necessary
    e.token(token, tokenSize);
    int r = e.encode();
    if (r < 0) {
        LOG(ERROR, "Failed to encode message: %d", r);
        return r;
    }
    if (r > (int)msg.capacity()) {
        LOG(ERROR, "Too large message");
        return SYSTEM_ERROR_TOO_LARGE;
    }
    r = channel_->send(msg);
    if (r != ProtocolError::NO_ERROR) {
        LOG(ERROR, "Failed to send message: %d", (int)r);
        return SYSTEM_ERROR_IO;
    }
    return 0;
}

int FirmwareUpdate::sendEmptyAck(Message* origMsg, CoapMessageId id) {
    Message ackMsg;
    if (channel_->response(*origMsg, ackMsg, 0) != 0) {
        LOG(ERROR, "Failed to create message");
        return SYSTEM_ERROR_NO_MEMORY;
    }
    CoapMessageEncoder e((char*)ackMsg.buf(), ackMsg.capacity());
    e.type(CoapType::ACK);
    e.code(CoapCode::EMPTY);
    e.id(id);
    int r = e.encode();
    if (r < 0) {
        LOG(ERROR, "Failed to encode message: %d", r);
        return r;
    }
    if (r > (int)ackMsg.capacity()) {
        LOG(ERROR, "Too large message");
        return SYSTEM_ERROR_TOO_LARGE;
    }
    r = channel_->send(ackMsg);
    if (r != ProtocolError::NO_ERROR) {
        LOG(ERROR, "Failed to send message: %d", (int)r);
        return SYSTEM_ERROR_IO;
    }
    return 0;
}

int FirmwareUpdate::parseBeginRequest(const CoapMessageDecoder& d, const char** fileHash, size_t* fileSize,
        size_t* chunkSize, bool* discardData) {
    if (d.type() != CoapType::CON) {
        LOG(ERROR, "Invalid message type");
        return SYSTEM_ERROR_COAP;
    }
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
        switch (it.option()) {
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
            if (!size || size < MIN_OTA_CHUNK_SIZE || size > MAX_OTA_CHUNK_SIZE) {
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
    return 0;
}

int FirmwareUpdate::parseEndRequest(const CoapMessageDecoder& d, bool* discardData, bool* cancelUpdate) {
    if (d.type() != CoapType::CON) {
        LOG(ERROR, "Invalid message type");
        return SYSTEM_ERROR_COAP;
    }
    if (d.tokenSize() != sizeof(token_t)) {
        LOG(ERROR, "Invalid token size");
        return SYSTEM_ERROR_COAP;
    }
    bool hasDiscardData = false;
    bool hasCancelUpdate = false;
    auto it = d.options();
    while (it.next()) {
        switch (it.option()) {
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
    if (!hasCancelUpdate) {
        *cancelUpdate = false;
    }
    return 0;
}

int FirmwareUpdate::parseChunkRequest(const CoapMessageDecoder& d, const char** chunkData, size_t* chunkSize,
        size_t* chunkIndex) {
    if (d.type() != CoapType::NON) {
        LOG(ERROR, "Invalid message type");
        return SYSTEM_ERROR_COAP;
    }
    if (d.tokenSize() != 0) {
        LOG(ERROR, "Invalid token size");
        return SYSTEM_ERROR_COAP;
    }
    if (d.payloadSize() == 0) {
        LOG(ERROR, "Invalid payload size");
        return SYSTEM_ERROR_COAP;
    }
    const auto it = d.findOption(MessageOption::CHUNK_INDEX);
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
    if (updating_) {
        const int r = finishUpdate(FirmwareUpdateFlag::CANCEL);
        if (r < 0) {
            LOG(ERROR, "finishUpdate() failed: %d", r);
        }
        updating_ = false;
    }
    ctx_.reset();
    memset(chunks_, 0, sizeof(chunks_));
    updateStartTime_ = 0;
    lastChunkTime_ = 0;
    fileSize_ = 0;
    fileOffset_ = 0;
    transferSize_ = 0;
    chunkSize_ = 0;
    chunkCount_ = 0;
    windowSize_ = 0;
    chunkIndex_ = 0;
    unackChunks_ = 0;
    hasChunkGaps_ = false;
#if OTA_UPDATE_STATS
    processTime_ = 0;
    recvChunks_ = 0;
    sentAcks_ = 0;
    outOrderChunks_ = 0;
    outWindowChunks_ = 0;
    dupChunks_ = 0;
#endif // OTA_UPDATE_STATS
}

} // namespace particle::protocol

#endif // HAL_PLATFORM_OTA_PROTOCOL_V3
