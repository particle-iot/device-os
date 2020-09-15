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

#undef LOG_COMPILE_TIME_LEVEL

#include "logging.h"

#include "firmware_update.h"

#if HAL_PLATFORM_OTA_PROTOCOL_V3

#include "message_channel.h"
#include "coap_message_encoder.h"
#include "coap_message_decoder.h"
#include "protocol_util.h"

#include "sha256.h"
#include "endian_util.h"
#include "logging.h"
#include "check.h"

LOG_SOURCE_CATEGORY("comm.ota")

namespace particle {

namespace protocol {

namespace {

const system_tick_t TRANSFER_STATE_LOG_INTERVAL = 3000;

static_assert(PARTICLE_LITTLE_ENDIAN, "This code is optimized for little-endian architectures");

// Protocol-specific CoAP options
enum OtaCoapOption {
    CHUNK_INDEX = 2049,
    WINDOW_SIZE = 2053,
    FILE_SIZE = 2057,
    FILE_SHA256 = 2061,
    CHUNK_SIZE = 2065,
    DISCARD_DATA = 2069,
    CANCEL_UPDATE = 2073
};

inline unsigned trailingOneBits(uint32_t v) {
    v = ~v;
    if (!v) {
        return 32;
    }
    return __builtin_ctz(v);
}

} // namespace

int FirmwareUpdate::init(MessageChannel* channel, const SparkCallbacks& callbacks) {
    channel_ = channel;
    callbacks_ = &callbacks;
    return 0;
}

void FirmwareUpdate::destroy() {
    reset();
}

ProtocolError FirmwareUpdate::responseAck(Message* msg) {
    if (!updating_) {
        return ProtocolError::INVALID_STATE;
    }
    if (!finishRespId_) {
        return ProtocolError::NO_ERROR;
    }
    CoapMessageDecoder d;
    int r = d.decode((const char*)msg->buf(), msg->length());
    if (r < 0) {
        LOG(ERROR, "Failed to decode message: %d", r);
        return ProtocolError::MALFORMED_MESSAGE;
    }
    if (d.type() != CoapType::ACK) {
        LOG(ERROR, "Invalid message type");
        return ProtocolError::INTERNAL;
    }
    if (d.id() == finishRespId_) {
        stats_.updateFinishTime = millis();
        LOG(INFO, "Update time: %u", (unsigned)(stats_.updateFinishTime - stats_.updateStartTime));
        system_tick_t transferTime = 0;
        if (stats_.transferStartTime && stats_.transferFinishTime) {
            transferTime = stats_.transferFinishTime - stats_.transferStartTime;
        }
        LOG(INFO, "Transfer time: %u", (unsigned)transferTime);
        LOG(INFO, "Processing time: %u", (unsigned)stats_.processingTime);
        LOG(INFO, "Chunks received: %u", stats_.receivedChunks);
        LOG(INFO, "Chunk ACKs sent: %u", stats_.sentChunkAcks);
        LOG(INFO, "Duplicate chunks: %u", stats_.duplicateChunks);
        LOG(INFO, "Out-of-order chunks: %u", stats_.outOfOrderChunks);
        LOG(INFO, "Applying firmware update");
        r = callbacks_->finish_firmware_update(0);
        if (r < 0) {
            LOG(ERROR, "Failed to apply firmware update: %d", r);
            cancelUpdate();
        } else {
            // finish_firmware_update() doesn't normally return on success, but it does so in unit tests
            updating_ = false;
        }
    }
    return ProtocolError::NO_ERROR;
}

ProtocolError FirmwareUpdate::process() {
    if (!updating_) {
        return ProtocolError::NO_ERROR;
    }
    if (unackChunks_ > 0 && lastChunkTime_ + OTA_CHUNK_ACK_DELAY <= millis()) {
        // Send an UpdateAck
        Message msg;
        int r = channel_->create(msg);
        if (r != ProtocolError::NO_ERROR) {
            LOG(ERROR, "Failed to create message");
            return (ProtocolError)r;
        }
        CoapMessageEncoder e((char*)msg.buf(), msg.capacity());
        initChunkAck(&e);
        r = e.encode();
        if (r < 0) {
            LOG(ERROR, "Failed to encode message: %d", r);
            return ProtocolError::INTERNAL;
        }
        if (r > (int)msg.capacity()) {
            LOG(ERROR, "Too large message");
            return ProtocolError::INSUFFICIENT_STORAGE;
        }
        msg.set_length(r);
        r = channel_->send(msg);
        if (r != ProtocolError::NO_ERROR) {
            LOG(ERROR, "Failed to send message: %d", (int)r);
            return (ProtocolError)r;
        }
        unackChunks_ = 0;
        ++stats_.sentChunkAcks;
    }
    if (chunkIndex_ < chunkCount_ && stateLogTime_ + TRANSFER_STATE_LOG_INTERVAL <= millis()) {
        const size_t bytesLeft = fileSize_ - fileOffset_;
        LOG(TRACE, "Received %u of %u bytes", (unsigned)(transferSize_ - bytesLeft), (unsigned)transferSize_);
        stateLogTime_ = millis();
    }
    const auto now = millis();
    if ((lastChunkTime_ && now - lastChunkTime_ >= OTA_TRANSFER_TIMEOUT) ||
            (!lastChunkTime_ && now - stats_.updateStartTime >= OTA_TRANSFER_TIMEOUT)) {
        LOG(ERROR, "Transfer timeout");
        cancelUpdate();
        return ProtocolError::MESSAGE_TIMEOUT;
    }
    return ProtocolError::NO_ERROR;
}

ProtocolError FirmwareUpdate::handleRequest(Message* msg, RequestHandlerFn handler) {
    reset_error_message();
    CoapMessageDecoder d;
    int r = d.decode((const char*)msg->buf(), msg->length());
    if (r < 0) {
        LOG(ERROR, "Failed to decode message: %d", r);
        return ProtocolError::MALFORMED_MESSAGE;
    }
    if (d.type() == CoapType::CON) {
        Message ack;
        r = channel_->response(*msg, ack, msg->capacity() - msg->length());
        if (r != ProtocolError::NO_ERROR) {
            LOG(ERROR, "Failed to create message: %d", (int)r);
            return (ProtocolError)r;
        }
        if (d.hasToken()) {
            // Validate the request
            const int handlerResult = (this->*handler)(d, nullptr, nullptr, true);
            if (handlerResult >= 0) {
                // Acknowledge the request
                r = sendEmptyAck(&ack, CoapType::ACK, d.id());
                if (r < 0) {
                    return ProtocolError::IO_ERROR_GENERIC_SEND;
                }
            } else {
                // Send an error response
                r = sendErrorResponse(&ack, handlerResult, CoapType::ACK, d.id(), d.token(), d.tokenSize());
                if (r < 0) {
                    return ProtocolError::IO_ERROR_GENERIC_SEND;
                }
                return ProtocolError::NO_ERROR; // Not a critical error
            }
        } else {
            // All confirmable messages defined by the protocol must have a token
            ERROR_MESSAGE("Message token is missing");
            r = sendErrorResponse(&ack, SYSTEM_ERROR_PROTOCOL, CoapType::ACK, d.id(), nullptr, 0);
            if (r < 0) {
                return ProtocolError::IO_ERROR_GENERIC_SEND;
            }
            return ProtocolError::NO_ERROR; // Not a critical error
        }
    } else if (d.type() != CoapType::NON) {
        // The upper layer code shouldn't have passed a message of any other type to this handler
        LOG(ERROR, "Invalid message type");
        return ProtocolError::INTERNAL;
    }
    // TODO: All messages created via BufferMessageChannel share the same buffer. It is generally
    // not safe to start encoding a response message until the original request is fully processed
    Message resp;
    r = channel_->create(resp);
    if (r != ProtocolError::NO_ERROR) {
        LOG(ERROR, "Failed to create message: %d", (int)r);
        return (ProtocolError)r;
    }
    CoapMessageEncoder e((char*)resp.buf(), resp.capacity());
    CoapMessageId* respId = nullptr; // TODO: Use a callback to pass the response ID to the request handler
    const int handlerResult = (this->*handler)(d, &e, &respId, false);
    if (handlerResult >= 0) {
        // Encode and send the response message
        r = e.encode();
        if (r >= 0) {
            if (r > (int)resp.capacity()) {
                LOG(ERROR, "Too large message");
                return ProtocolError::INSUFFICIENT_STORAGE;
            }
            resp.set_length(r);
            r = channel_->send(resp);
            if (r != ProtocolError::NO_ERROR) {
                LOG(ERROR, "Failed to send message: %d", (int)r);
                return (ProtocolError)r;
            }
            if (respId) {
                *respId = resp.get_id();
            }
        } else if (r != SYSTEM_ERROR_INVALID_STATE) {
            // SYSTEM_ERROR_INVALID_STATE indicates that the handler hasn't encoded a response
            LOG(ERROR, "Failed to encode message: %d", r);
            return ProtocolError::INTERNAL;
        }
    } else {
        if (d.tokenSize() > 0) {
            // Reply with a separate confirmable or non-confirmable response
            r = sendErrorResponse(&resp, handlerResult, d.type(), 0, d.token(), d.tokenSize());
            if (r < 0) {
                return ProtocolError::IO_ERROR_GENERIC_SEND;
            }
        } else {
            // The original request doesn't have a token so we can only reply with an RST to it
            r = sendEmptyAck(&resp, CoapType::RST, d.id());
            if (r < 0) {
                return ProtocolError::IO_ERROR_GENERIC_SEND;
            }
        }
        // Errors during OTA are not fatal to the connection unless we weren't able to send
        // an error response
        cancelUpdate();
    }
    return ProtocolError::NO_ERROR;
}

int FirmwareUpdate::handleStartRequest(const CoapMessageDecoder& d, CoapMessageEncoder* e, CoapMessageId** respId,
        bool validateOnly) {
    const auto startTime = millis();
    const char* fileHash = nullptr;
    size_t fileSize = 0;
    size_t chunkSize = 0;
    bool discardData = false;
    CHECK(decodeStartRequest(d, &fileSize, &fileHash, &chunkSize, &discardData));
    if (validateOnly) {
        return 0;
    }
    if (!updating_) {
        LOG(INFO, "Received UpdateStart request");
    } else {
        LOG(WARN, "Received UpdateStart request but another update is already in progress");
    }
    reset();
    stats_.updateStartTime = startTime;
    LOG(INFO, "File size: %u", (unsigned)fileSize);
    LOG(INFO, "Chunk size: %u", (unsigned)chunkSize);
    if (discardData) {
        LOG(INFO, "Discard data: %u", (unsigned)discardData);
    }
    if (fileHash) {
        LOG(INFO, "File checksum:");
        LOG_DUMP(INFO, fileHash, Sha256::HASH_SIZE);
        LOG_PRINT(INFO, "\r\n");
    }
    LOG(INFO, "Starting firmware update");
    fileSize_ = fileSize;
    chunkSize_ = chunkSize;
    FirmwareUpdateFlags flags;
    if (discardData) {
        flags |= FirmwareUpdateFlag::DISCARD_DATA;
    }
    if (!fileHash) {
        flags |= FirmwareUpdateFlag::NON_RESUMABLE;
    }
    const auto t1 = millis();
    CHECK(callbacks_->start_firmware_update(fileSize_, fileHash, &fileOffset_, flags.value()));
    stats_.processingTime += millis() - t1;
    transferSize_ = fileSize_ - fileOffset_;
    chunkCount_ = (transferSize_ + chunkSize_ - 1) / chunkSize_;
    windowSize_ = OTA_RECEIVE_WINDOW_SIZE / chunkSize_;
    LOG(INFO, "Start offset: %u", (unsigned)fileOffset_);
    LOG(INFO, "Chunk count: %u", (unsigned)chunkCount_);
    LOG(TRACE, "Window size (chunks): %u", (unsigned)windowSize_);
    lastChunkTime_ = millis(); // ACK for the first chunk will be delayed
    updating_ = true;
    e->type(d.type());
    e->code(CoapCode::CREATED);
    e->id(0); // Will be assigned by the message channel
    e->token(d.token(), d.tokenSize());
    e->option(OtaCoapOption::WINDOW_SIZE, (unsigned)windowSize_);
    e->option(OtaCoapOption::FILE_SIZE, (unsigned)fileOffset_);
    return 0;
}

int FirmwareUpdate::handleFinishRequest(const CoapMessageDecoder& d, CoapMessageEncoder* e, CoapMessageId** respId,
        bool validateOnly) {
    bool cancelUpdate = false;
    bool discardData = false;
    CHECK(decodeFinishRequest(d, &cancelUpdate, &discardData));
    if (validateOnly) {
        return 0;
    }
    if (!updating_) {
        LOG(WARN, "Received UpdateFinish request but no update is in progress");
        return SYSTEM_ERROR_INVALID_STATE;
    }
    LOG(INFO, "Received UpdateFinish request");
    FirmwareUpdateFlags flags;
    if (discardData) {
        LOG(INFO, "Discard data: %u", (unsigned)discardData);
        flags |= FirmwareUpdateFlag::DISCARD_DATA;
    }
    if (cancelUpdate) {
        LOG(INFO, "Cancel update: %u", (unsigned)cancelUpdate);
        LOG(INFO, "Cancelling firmware update");
        flags |= FirmwareUpdateFlag::CANCEL;
    } else {
        if (fileOffset_ != fileSize_) {
            ERROR_MESSAGE("Incomplete file transfer");
            return SYSTEM_ERROR_PROTOCOL;
        }
        LOG(INFO, "Validating firmware update");
        flags |= FirmwareUpdateFlag::VALIDATE_ONLY;
    }
    const auto t1 = millis();
    CHECK(callbacks_->finish_firmware_update(flags.value()));
    stats_.processingTime += millis() - t1;
    e->type(d.type());
    e->code(CoapCode::CHANGED);
    e->id(0); // Will be assigned by the message channel
    e->token(d.token(), d.tokenSize());
    if (cancelUpdate) {
        updating_ = false;
    } else {
        *respId = &finishRespId_;
    }
    return 0;
}

int FirmwareUpdate::handleChunkRequest(const CoapMessageDecoder& d, CoapMessageEncoder* e, CoapMessageId** respId,
        bool validateOnly) {
    const auto chunkTime = millis();
    const char* data = nullptr;
    size_t size = 0;
    unsigned index = 0;
    CHECK(decodeChunkRequest(d, &data, &size, &index));
    if (validateOnly) {
        return 0;
    }
    if (!updating_) {
        LOG(WARN, "Received UpdateChunk request but no update is in progress");
        return SYSTEM_ERROR_INVALID_STATE;
    }
    ++stats_.receivedChunks;
    if (index == 0 || index > chunkCount_) { // Chunk indices are 1-based
        ERROR_MESSAGE("Invalid chunk index: %u", index);
        return SYSTEM_ERROR_PROTOCOL;
    }
    if ((index < chunkCount_ && size != chunkSize_) ||
            (index == chunkCount_ && (index - 1) * chunkSize_ + size > transferSize_)) {
        ERROR_MESSAGE("Invalid chunk size: %u", size);
        return SYSTEM_ERROR_PROTOCOL;
    }
    bool isDupChunk = false;
    if (index <= chunkIndex_) {
        isDupChunk = true;
    } else if (index > chunkIndex_ + windowSize_) {
        LOG(WARN, "Chunk is out of receiver window");
    } else {
        // Index of the chunk relative to the left edge of the receiver window (0-based)
        index -= chunkIndex_ + 1;
        // Position of the chunk bit in the bitmap
        const size_t wordIndex = index / 32;
        const unsigned bitIndex = index % 32;
        uint32_t w = chunks_[wordIndex];
        if (w & (1 << bitIndex)) {
            isDupChunk = true;
        } else {
            w |= (1 << bitIndex);
            chunks_[wordIndex] = w;
            const size_t offs = fileOffset_ + index * chunkSize_; // Chunk offset in the file
            if (index == 0) {
                // Shift the receiver window
                unsigned bits = 0;
                while ((bits = trailingOneBits(chunks_[0]))) {
                    for (size_t i = 0; i < OTA_CHUNK_BITMAP_ELEMENTS; ++i) {
                        chunks_[i] >>= bits;
                        if (i < OTA_CHUNK_BITMAP_ELEMENTS - 1) {
                            chunks_[i] |= chunks_[i + 1] << (32 - bits);
                        }
                    }
                    fileOffset_ += bits * chunkSize_;
                    chunkIndex_ += bits;
                }
                // Last chunk can be smaller than the maximum chunk size
                if (fileOffset_ > fileSize_) {
                    fileOffset_ = fileSize_;
                }
            } else if ((bitIndex > 0 && !(w & (1 << (bitIndex - 1)))) ||
                    (bitIndex == 0 && wordIndex > 0 && !(chunks_[wordIndex - 1] & (1 << 31)))) {
                ++stats_.outOfOrderChunks;
            }
            const auto t1 = millis();
            CHECK(callbacks_->save_firmware_chunk(data, size, offs, fileOffset_));
            stats_.processingTime += millis() - t1;
        }
    }
    if (isDupChunk) {
        ++stats_.duplicateChunks;
    }
    bool hasGaps = false;
    for (size_t i = 0; i < OTA_CHUNK_BITMAP_ELEMENTS; ++i) {
        // If some bits are still set in the bitmap, then there's a gap in the sequence of received chunks
        if (chunks_[i]) {
            hasGaps = true;
            break;
        }
    }
    ++unackChunks_;
    if (isDupChunk || hasGaps || hasGaps != hasGaps_ || chunkIndex_ == chunkCount_ || unackChunks_ >= OTA_CHUNK_ACK_COUNT ||
            lastChunkTime_ + OTA_CHUNK_ACK_DELAY <= millis()) {
        // Send an UpdateAck
        initChunkAck(e);
        unackChunks_ = 0;
        ++stats_.sentChunkAcks;
    }
    hasGaps_ = hasGaps;
    lastChunkTime_ = chunkTime;
    if (!stats_.transferStartTime) {
        stats_.transferStartTime = chunkTime;
    }
    if (chunkIndex_ == chunkCount_ && !stats_.transferFinishTime) {
        stats_.transferFinishTime = millis();
    }
    if (stateLogChunks_ < chunkIndex_ && (stateLogTime_ + TRANSFER_STATE_LOG_INTERVAL <= millis() ||
            chunkIndex_ == chunkCount_)) {
        const size_t bytesLeft = fileSize_ - fileOffset_;
        LOG(TRACE, "Received %u of %u bytes", (unsigned)(transferSize_ - bytesLeft), (unsigned)transferSize_);
        stateLogChunks_ = chunkIndex_;
        stateLogTime_ = millis();
    }
    return 0;
}

int FirmwareUpdate::decodeStartRequest(const CoapMessageDecoder& d, size_t* fileSize, const char** fileHash,
        size_t* chunkSize, bool* discardData) {
    if (d.type() != CoapType::CON) {
        ERROR_MESSAGE("Invalid message type");
        return SYSTEM_ERROR_PROTOCOL;
    }
    if (!d.hasToken()) {
        ERROR_MESSAGE("Invalid token size");
        return SYSTEM_ERROR_PROTOCOL;
    }
    bool hasFileSize = false;
    bool hasFileHash = false;
    bool hasChunkSize = false;
    bool hasDiscardData = false;
    auto it = d.options();
    while (it.next()) {
        switch (it.option()) {
        case OtaCoapOption::FILE_SIZE: {
            const size_t size = it.toUInt();
            if (!size) {
                ERROR_MESSAGE("Invalid file size: %u", (unsigned)size);
                return SYSTEM_ERROR_PROTOCOL;
            }
            *fileSize = size;
            hasFileSize = true;
            break;
        }
        case OtaCoapOption::FILE_SHA256: {
            if (it.size() != Sha256::HASH_SIZE) {
                ERROR_MESSAGE("Invalid option size");
                return SYSTEM_ERROR_PROTOCOL;
            }
            *fileHash = it.data();
            hasFileHash = true;
            break;
        }
        case OtaCoapOption::CHUNK_SIZE: {
            const size_t size = it.toUInt();
            if (size < MIN_OTA_CHUNK_SIZE || size > MAX_OTA_CHUNK_SIZE || size % 4 != 0) {
                ERROR_MESSAGE("Invalid chunk size: %u", (unsigned)size);
                return SYSTEM_ERROR_PROTOCOL;
            }
            *chunkSize = size;
            hasChunkSize = true;
            break;
        }
        case OtaCoapOption::DISCARD_DATA: {
            if (it.size() != 0) {
                ERROR_MESSAGE("Invalid option size");
                return SYSTEM_ERROR_PROTOCOL;
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
        ERROR_MESSAGE("Invalid message options");
        return SYSTEM_ERROR_PROTOCOL;
    }
    if (!hasFileHash) {
        *fileHash = nullptr;
    }
    if (!hasDiscardData) {
        *discardData = false;
    }
    return 0;
}

int FirmwareUpdate::decodeFinishRequest(const CoapMessageDecoder& d, bool* cancelUpdate, bool* discardData) {
    if (d.type() != CoapType::CON) {
        ERROR_MESSAGE("Invalid message type");
        return SYSTEM_ERROR_PROTOCOL;
    }
    if (!d.hasToken()) {
        ERROR_MESSAGE("Invalid token size");
        return SYSTEM_ERROR_PROTOCOL;
    }
    bool hasCancelUpdate = false;
    bool hasDiscardData = false;
    auto it = d.options();
    while (it.next()) {
        switch (it.option()) {
        case OtaCoapOption::CANCEL_UPDATE: {
            if (it.size() != 0) {
                ERROR_MESSAGE("Invalid option size");
                return SYSTEM_ERROR_PROTOCOL;
            }
            *cancelUpdate = true;
            hasCancelUpdate = true;
            break;
        }
        case OtaCoapOption::DISCARD_DATA: {
            if (it.size() != 0) {
                ERROR_MESSAGE("Invalid option size");
                return SYSTEM_ERROR_PROTOCOL;
            }
            *discardData = true;
            hasDiscardData = true;
            break;
        }
        default:
            break;
        }
    }
    if (!hasCancelUpdate) {
        *cancelUpdate = false;
    }
    if (!hasDiscardData) {
        *discardData = false;
    }
    return 0;
}

int FirmwareUpdate::decodeChunkRequest(const CoapMessageDecoder& d, const char** chunkData, size_t* chunkSize,
        unsigned* chunkIndex) {
    if (d.type() != CoapType::NON) {
        ERROR_MESSAGE("Invalid message type");
        return SYSTEM_ERROR_PROTOCOL;
    }
    if (d.hasToken()) {
        ERROR_MESSAGE("Invalid token size");
        return SYSTEM_ERROR_PROTOCOL;
    }
    if (d.payloadSize() == 0) {
        ERROR_MESSAGE("Invalid payload size");
        return SYSTEM_ERROR_PROTOCOL;
    }
    const auto it = d.findOption(OtaCoapOption::CHUNK_INDEX);
    if (!it) {
        ERROR_MESSAGE("Invalid message options");
        return SYSTEM_ERROR_PROTOCOL;
    }
    *chunkIndex = it.toUInt();
    *chunkData = d.payload();
    *chunkSize = d.payloadSize();
    return 0;
}

void FirmwareUpdate::initChunkAck(CoapMessageEncoder* e) {
    size_t payloadSize = 0;
    for (int i = OTA_CHUNK_BITMAP_ELEMENTS - 1; i >= 0; --i) {
        if (chunks_[i]) {
            payloadSize = (i + 1) * sizeof(uint32_t);
            break;
        }
    }
    e->type(CoapType::NON);
    e->code(CoapCode::POST);
    e->id(0); // Will be assigned by the message channel
    e->option(CoapOption::URI_PATH, "A");
    e->option(OtaCoapOption::CHUNK_INDEX, chunkIndex_);
    e->payload((const char*)chunks_, payloadSize);
}

int FirmwareUpdate::sendErrorResponse(Message* msg, int error, CoapType type, CoapMessageId id, const char* token,
        size_t tokenSize) {
    CoapMessageEncoder e((char*)msg->buf(), msg->capacity());
    e.type(type);
    e.code(coapCodeForSystemError(error));
    e.id(id);
    e.token(token, tokenSize);
    int r = formatDiagnosticPayload(e.payloadData(), e.maxPayloadSize(), error);
    if (r > 0) {
        e.payloadSize(r);
    }
    r = e.encode();
    if (r < 0) {
        LOG(ERROR, "Failed to encode message: %d", r);
        return r;
    }
    if (r > (int)msg->capacity()) {
        LOG(ERROR, "Too large message");
        return SYSTEM_ERROR_TOO_LARGE;
    }
    msg->set_length(r);
    r = channel_->send(*msg);
    if (r != ProtocolError::NO_ERROR) {
        LOG(ERROR, "Failed to send message: %d", (int)r);
        return SYSTEM_ERROR_IO;
    }
    return 0;
}

int FirmwareUpdate::sendEmptyAck(Message* msg, CoapType type, CoapMessageId id) {
    CoapMessageEncoder e((char*)msg->buf(), msg->capacity());
    e.type(type);
    e.code(CoapCode::EMPTY);
    e.id(id);
    int r = e.encode();
    if (r < 0) {
        LOG(ERROR, "Failed to encode message: %d", r);
        return r;
    }
    if (r > (int)msg->capacity()) {
        LOG(ERROR, "Too large message");
        return SYSTEM_ERROR_TOO_LARGE;
    }
    msg->set_length(r);
    msg->set_id(id);
    r = channel_->send(*msg);
    if (r != ProtocolError::NO_ERROR) {
        LOG(ERROR, "Failed to send message: %d", (int)r);
        return SYSTEM_ERROR_IO;
    }
    return 0;
}

void FirmwareUpdate::cancelUpdate() {
    if (updating_) {
        const int r = callbacks_->finish_firmware_update((unsigned)FirmwareUpdateFlag::CANCEL);
        if (r < 0) {
            LOG(ERROR, "Failed to cancel the update: %d", r);
        }
        updating_ = false;
    }
}

void FirmwareUpdate::reset() {
    cancelUpdate();
    memset(chunks_, 0, sizeof(chunks_));
    stats_ = FirmwareUpdateStats();
    lastChunkTime_ = 0;
    stateLogTime_ = 0;
    fileSize_ = 0;
    fileOffset_ = 0;
    transferSize_ = 0;
    chunkSize_ = 0;
    chunkCount_ = 0;
    windowSize_ = 0;
    chunkIndex_ = 0;
    unackChunks_ = 0;
    stateLogChunks_ = 0;
    finishRespId_ = 0;
    hasGaps_ = false;
}

} // namespace protocol

} // namespace particle

#endif // HAL_PLATFORM_OTA_PROTOCOL_V3
