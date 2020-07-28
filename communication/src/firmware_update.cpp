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
#include "endian_util.h"
#include "logging.h"
#include "check.h"

#include <algorithm>

LOG_SOURCE_CATEGORY("comm.ota")

namespace particle::protocol {

namespace {

static_assert(PARTICLE_LITTLE_ENDIAN, "This code is optimized for little-endian architectures");

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

ProtocolError FirmwareUpdate::process() {
    if (!updating_) {
        return ProtocolError::NO_ERROR;
    }
    if (unackChunks_ > 0 && lastChunkTime_ + OTA_CHUNK_ACK_DELAY <= millis()) {
        // Send a ChunkAck
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
            return ProtocolError::UNKNOWN;
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
#if OTA_UPDATE_STATS
        ++sentAcks_;
#endif
    }
    return ProtocolError::NO_ERROR;
}

ProtocolError FirmwareUpdate::handleRequest(Message* msg, RequestHandlerFn handler) {
    CoapMessageDecoder d;
    int r = d.decode((const char*)msg->buf(), msg->length());
    if (r < 0) {
        LOG(ERROR, "Failed to decode message: %d", r);
        return ProtocolError::MALFORMED_MESSAGE;
    }
    if (d.type() == CoapType::CON) {
        // Send an empty ACK. Make sure not to overwrite the contents of the original message
        const size_t avail = msg->capacity() - msg->length();
        Message ack;
        r = channel_->response(*msg, ack, avail);
        if (r != ProtocolError::NO_ERROR) {
            LOG(ERROR, "Failed to create message");
            return (ProtocolError)r;
        }
        r = sendEmptyAck(&ack, CoapType::ACK, d.id());
        if (r < 0) {
            LOG(ERROR, "Failed to send ACK: %d", r);
            return ProtocolError::IO_ERROR_GENERIC_SEND;
        }
    }
    // TODO: All messages created via BufferMessageChannel share the same buffer. It is typically
    // not safe to start encoding a response message until the original request is fully processed
    Message resp;
    r = channel_->create(resp);
    if (r != ProtocolError::NO_ERROR) {
        LOG(ERROR, "Failed to create message");
        return (ProtocolError)r;
    }
    CoapMessageEncoder e((char*)resp.buf(), resp.capacity());
    const int handlerResult = (this->*handler)(d, &e);
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
        } else if (r != SYSTEM_ERROR_INVALID_STATE) {
            LOG(ERROR, "Failed to encode message: %d", r);
            return ProtocolError::UNKNOWN;
        } // SYSTEM_ERROR_INVALID_STATE indicates that the request handler hasn't encoded a response
    } else {
        if (d.tokenSize() > 0) {
            // Reply with a separate confirmable or non-confirmable response
            r = sendErrorResponse(&resp, handlerResult, d.type(), d.token(), d.tokenSize());
            if (r < 0) {
                LOG(ERROR, "Failed to send response: %d", r);
                return ProtocolError::IO_ERROR_GENERIC_SEND;
            }
        } else {
            // The original request doesn't have a token so we can only reply with an RST to it
            r = sendEmptyAck(&resp, CoapType::RST, d.id());
            if (r < 0) {
                LOG(ERROR, "Failed to send RST: %d", r);
                return ProtocolError::IO_ERROR_GENERIC_SEND;
            }
        }
        // Errors during OTA are not fatal to the connection unless we weren't able to send
        // an error response
        reset();
    }
    return ProtocolError::NO_ERROR;
}

int FirmwareUpdate::handleBeginRequest(const CoapMessageDecoder& d, CoapMessageEncoder* e) {
    if (!updating_) {
        LOG(INFO, "Received Begin request");
    } else {
        LOG(WARN, "Received Begin request but another update is already in progress");
    }
    reset();
    updateStartTime_ = millis();
    // Parse message
    const char* fileHash = nullptr;
    bool discardData = false;
    CHECK(decodeBeginRequest(d, &fileHash, &fileSize_, &chunkSize_, &discardData));
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
#if OTA_UPDATE_STATS
    const auto t1 = millis();
#endif
    CHECK(callbacks_->start_firmware_update(fileSize_, fileHash, &fileOffset_, flags.value()));
#if OTA_UPDATE_STATS
    processTime_ += millis() - t1;
#endif
    transferSize_ = fileSize_ - fileOffset_;
    chunkCount_ = (transferSize_ + chunkSize_ - 1) / chunkSize_;
    LOG(INFO, "File offset: %u", (unsigned)fileOffset_);
    LOG(INFO, "Chunk count: %u", (unsigned)chunkCount_);
    windowSize_ = OTA_RECEIVE_WINDOW_SIZE / chunkSize_;
#if OTA_UPDATE_STATS
    LOG(TRACE, "Window size (chunks): %u", (unsigned)windowSize_);
#endif
    lastChunkTime_ = millis(); // ACK for the first chunk will be delayed
    updating_ = true;
    e->type(d.type());
    e->code(CoapCode::CREATED);
    e->id(0); // Will be assigned by the message channel
    e->token(d.token(), d.tokenSize());
    e->option(MessageOption::FILE_SIZE, fileOffset_);
    return 0;
}

int FirmwareUpdate::handleEndRequest(const CoapMessageDecoder& d, CoapMessageEncoder* e) {
    if (!updating_) {
        LOG(WARN, "Received End request but no update is in progress");
        return SYSTEM_ERROR_INVALID_STATE;
    }
    bool cancelUpdate = false;
    bool discardData = false;
    CHECK(decodeEndRequest(d, &cancelUpdate, &discardData));
    LOG(INFO, "Finishing firmware update");
    LOG(INFO, "Cancel update: %u", (unsigned)cancelUpdate);
    LOG(INFO, "Discard data: %u", (unsigned)discardData);
    FirmwareUpdateFlags flags;
    if (cancelUpdate) {
        flags |= FirmwareUpdateFlag::CANCEL;
    } else if (fileOffset_ != fileSize_) {
        LOG(ERROR, "Incomplete file transfer");
        return SYSTEM_ERROR_PROTOCOL;
    }
    if (discardData) {
        flags |= FirmwareUpdateFlag::DISCARD_DATA;
    }
    flags |= FirmwareUpdateFlag::VALIDATE_ONLY;
#if OTA_UPDATE_STATS
    auto t1 = millis();
#endif
    CHECK(callbacks_->finish_firmware_update(flags.value()));
#if OTA_UPDATE_STATS
    processTime_ += millis() - t1;
#endif
    updating_ = false;
    e->type(d.type());
    e->code(CoapCode::CHANGED);
    e->id(0); // Will be assigned by the message channel
    e->token(d.token(), d.tokenSize());
    return 0;
}

int FirmwareUpdate::handleChunkRequest(const CoapMessageDecoder& d, CoapMessageEncoder* e) {
    if (!updating_) {
        LOG(WARN, "Received Chunk request but no update is in progress");
        return SYSTEM_ERROR_INVALID_STATE;
    }
    const char* data = nullptr;
    size_t size = 0;
    unsigned index = 0;
    CHECK(decodeChunkRequest(d, &data, &size, &index));
    if (index == 0 || index > chunkCount_) { // Chunk indices are 1-based
        LOG(ERROR, "Invalid chunk index: %u", index);
        return SYSTEM_ERROR_PROTOCOL;
    }
    if ((index < chunkCount_ && size != chunkSize_) ||
            (index == chunkCount_ && (index - 1) * chunkSize_ + size > transferSize_)) {
        LOG(ERROR, "Invalid chunk size: %u", size);
        return SYSTEM_ERROR_PROTOCOL;
    }
    bool dupChunk = false;
    bool outWindowChunk = false;
    if (index <= chunkIndex_) {
        dupChunk = true;
        outWindowChunk = true;
    } else if (index > chunkIndex_ + windowSize_) {
        outWindowChunk = true;
    } else {
        // Index of the chunk relative to the left edge of the receiver window (0-based)
        const unsigned relIndex = index - chunkIndex_ - 1;
        // Position of the chunk bit in the bitmap
        const size_t wordIndex = relIndex / 32;
        const unsigned bitIndex = relIndex % 32;
        uint32_t w = chunks_[wordIndex];
        if (w & (1 << bitIndex)) {
            dupChunk = true;
        } else {
            w |= (1 << bitIndex);
            chunks_[wordIndex] = w;
            const size_t offs = fileOffset_ + relIndex * chunkSize_; // Chunk offset in the file
            if (relIndex == 0) {
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
            } else {
#if OTA_UPDATE_STATS
                ++outOrderChunks_;
#endif
            }
            // Last chunk can be smaller than the maximum chunk size
            if (fileOffset_ > fileSize_) {
                fileOffset_ = fileSize_;
            }
#if OTA_UPDATE_STATS
            const auto t1 = millis();
#endif
            CHECK(callbacks_->save_firmware_chunk(data, size, offs, fileOffset_));
#if OTA_UPDATE_STATS
            processTime_ += millis() - t1;
#endif
        }
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
    if (hasGaps || dupChunk || outWindowChunk || unackChunks_ == OTA_CHUNK_ACK_COUNT ||
            lastChunkTime_ + OTA_CHUNK_ACK_DELAY <= millis()) {
        // Send a ChunkAck
        initChunkAck(e);
        unackChunks_ = 0;
#if OTA_UPDATE_STATS
        if (dupChunk) {
            ++dupChunks_;
        }
        if (outWindowChunk) {
            ++outWindowChunks_;
        }
        ++sentAcks_;
#endif // OTA_UPDATE_STATS
    }
    lastChunkTime_ = millis();
#if OTA_UPDATE_STATS
    ++recvChunks_;
#endif
    return 0;
}

int FirmwareUpdate::decodeBeginRequest(const CoapMessageDecoder& d, const char** fileHash, size_t* fileSize,
        size_t* chunkSize, bool* discardData) {
    if (d.type() != CoapType::CON) {
        LOG(ERROR, "Invalid message type");
        return SYSTEM_ERROR_PROTOCOL;
    }
    if (!d.hasToken()) {
        LOG(ERROR, "Invalid token size");
        return SYSTEM_ERROR_PROTOCOL;
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
                return SYSTEM_ERROR_PROTOCOL;
            }
            *fileSize = size;
            hasFileSize = true;
            break;
        }
        case MessageOption::FILE_SHA_256: {
            if (it.size() != Sha256::HASH_SIZE) {
                LOG(ERROR, "Invalid option size");
                return SYSTEM_ERROR_PROTOCOL;
            }
            *fileHash = it.data();
            hasFileHash = true;
            break;
        }
        case MessageOption::CHUNK_SIZE: {
            const size_t size = it.toUInt();
            if (size < MIN_OTA_CHUNK_SIZE || size > MAX_OTA_CHUNK_SIZE || size % 4 != 0) {
                LOG(ERROR, "Invalid chunk size: %u", (unsigned)size);
                return SYSTEM_ERROR_PROTOCOL;
            }
            *chunkSize = size;
            hasChunkSize = true;
            break;
        }
        case MessageOption::DISCARD_DATA: {
            if (it.size() != 0) {
                LOG(ERROR, "Invalid option size");
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
        LOG(ERROR, "Invalid message options");
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

int FirmwareUpdate::decodeEndRequest(const CoapMessageDecoder& d, bool* cancelUpdate, bool* discardData) {
    if (d.type() != CoapType::CON) {
        LOG(ERROR, "Invalid message type");
        return SYSTEM_ERROR_PROTOCOL;
    }
    if (!d.hasToken()) {
        LOG(ERROR, "Invalid token size");
        return SYSTEM_ERROR_PROTOCOL;
    }
    bool hasCancelUpdate = false;
    bool hasDiscardData = false;
    auto it = d.options();
    while (it.next()) {
        switch (it.option()) {
        case MessageOption::CANCEL_UPDATE: {
            if (it.size() != 0) {
                LOG(ERROR, "Invalid option size");
                return SYSTEM_ERROR_PROTOCOL;
            }
            *cancelUpdate = true;
            hasCancelUpdate = true;
            break;
        }
        case MessageOption::DISCARD_DATA: {
            if (it.size() != 0) {
                LOG(ERROR, "Invalid option size");
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
    if (hasDiscardData && !hasCancelUpdate) {
        LOG(ERROR, "Invalid message options");
        return SYSTEM_ERROR_PROTOCOL;
    }
    if (!hasCancelUpdate) {
        *cancelUpdate = false;
    }
    if (!hasDiscardData) {
        *discardData = false;
    }
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
    e->option(MessageOption::CHUNK_INDEX, chunkIndex_);
    e->payload((const char*)chunks_, payloadSize);
}

int FirmwareUpdate::decodeChunkRequest(const CoapMessageDecoder& d, const char** chunkData, size_t* chunkSize,
        unsigned* chunkIndex) {
    if (d.type() != CoapType::NON) {
        LOG(ERROR, "Invalid message type");
        return SYSTEM_ERROR_PROTOCOL;
    }
    if (d.hasToken()) {
        LOG(ERROR, "Invalid token size");
        return SYSTEM_ERROR_PROTOCOL;
    }
    if (d.payloadSize() == 0) {
        LOG(ERROR, "Invalid payload size");
        return SYSTEM_ERROR_PROTOCOL;
    }
    const auto it = d.findOption(MessageOption::CHUNK_INDEX);
    if (!it) {
        LOG(ERROR, "Invalid message options");
        return SYSTEM_ERROR_PROTOCOL;
    }
    *chunkIndex = it.toUInt();
    *chunkData = d.payload();
    *chunkSize = d.payloadSize();
    return 0;
}

int FirmwareUpdate::sendErrorResponse(Message* msg, int error, CoapType type, const char* token, size_t tokenSize) {
    CoapMessageEncoder e((char*)msg->buf(), msg->capacity());
    e.type(type);
    e.code(coapCodeForSystemError(error));
    e.id(0); // Will be assigned by the message channel
    e.token(token, tokenSize);
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
    r = channel_->send(*msg);
    if (r != ProtocolError::NO_ERROR) {
        LOG(ERROR, "Failed to send message: %d", (int)r);
        return SYSTEM_ERROR_IO;
    }
    return 0;
}

void FirmwareUpdate::reset() {
    if (updating_) {
        const int r = callbacks_->finish_firmware_update((unsigned)FirmwareUpdateFlag::CANCEL);
        if (r < 0) {
            LOG(ERROR, "Failed to cancel the update: %d", r);
        }
        updating_ = false;
    }
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
