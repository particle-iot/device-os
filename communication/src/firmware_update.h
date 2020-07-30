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

#include "hal_platform.h"

#if HAL_PLATFORM_OTA_PROTOCOL_V3

#include "spark_protocol_functions.h"
#include "protocol_defs.h"
#include "coap_defs.h"

#include "system_defs.h"
#include "c_string.h"

#include <cstdint>
#include <cstddef>

#define OTA_UPDATE_STATS 1 // FIXME

#ifndef OTA_UPDATE_STATS
#ifdef DEBUG_BUILD
#define OTA_UPDATE_STATS 1
#else
#define OTA_UPDATE_STATS 0
#endif
#endif // !defined(OTA_UPDATE_STATS)

namespace particle::protocol {

/**
 * Size of the receiver window in bytes.
 *
 * Received chunks get consumed immediately, so the receiver window can be relatively large.
 * This parameter affects the size of the chunk bitmap maintained by the protocol implementation.
 */
const size_t OTA_RECEIVE_WINDOW_SIZE = 128 * 1024;

static_assert(OTA_RECEIVE_WINDOW_SIZE > MAX_OTA_CHUNK_SIZE, "Invalid RECEIVE_WINDOW_SIZE");

/**
 * Size of the chunk bitmap in 32-bit words.
 */
const size_t OTA_CHUNK_BITMAP_ELEMENTS = (OTA_RECEIVE_WINDOW_SIZE / MIN_OTA_CHUNK_SIZE + 31) / 32;

/**
 * Acknowledgement delay in milliseconds.
 *
 * SCTP recommends using a delay of 200ms with 500ms being the absolute maximum. Setting this
 * parameter to 0 disables delayed acknowledgements.
 */
const system_tick_t OTA_CHUNK_ACK_DELAY = 300;

/**
 * Minimum number of chunks to receive before generating an acknowledgement.
 *
 * Setting this parameter to 1 disables delayed acknowledgements.
 */
const unsigned OTA_CHUNK_ACK_COUNT = 2;

class Message;
class MessageChannel;
class CoapMessageDecoder;
class CoapMessageEncoder;

/**
 * A class implementing the OTA update protocol v3.
 */
class FirmwareUpdate {
public:
    FirmwareUpdate();
    ~FirmwareUpdate();

    int init(MessageChannel* channel, const SparkCallbacks& callbacks);
    void destroy();

    ProtocolError startRequest(Message* msg);
    ProtocolError finishRequest(Message* msg);
    ProtocolError chunkRequest(Message* msg);
    ProtocolError responseAck(Message* msg);
    ProtocolError process();

    bool isRunning() const;

    void reset();

private:
    typedef int (FirmwareUpdate::*RequestHandlerFn)(const CoapMessageDecoder& d, CoapMessageEncoder* e, CoapMessageId** respId);

    const SparkCallbacks* callbacks_; // Callbacks
    MessageChannel* channel_; // Message channel
    bool updating_; // Whether an update is in progress

    uint32_t chunks_[OTA_CHUNK_BITMAP_ELEMENTS]; // Bitmap of received chunks within the receiver window
    system_tick_t lastChunkTime_; // Time when the last chunk was received
    size_t fileSize_; // File size
    size_t fileOffset_; // Current offset in the file
    size_t transferSize_; // Total size of the data to transfer
    size_t chunkSize_; // Chunk size
    size_t chunkCount_; // Total number of chunks to transfer
    size_t windowSize_; // Size of the receiver window in chunks
    unsigned chunkIndex_; // Number of cumulatively acknowledged chunks
    unsigned unackChunks_; // Number or chunks received since the last acknowledgement
    CoapMessageId finishRespId_; // Message ID of the UpdateFinish response

#if OTA_UPDATE_STATS
    system_tick_t updateStartTime_; // Time when the update started
    system_tick_t transferStartTime_; // Time when the file transfer started
    system_tick_t transferFinishTime_; // Time when the file transfer finished
    system_tick_t processTime_; // System processing time
    system_tick_t lastLogTime_; // Time when the transfer state was last logged
    unsigned lastLogChunks_; // Number of cumulatively acknowledged chunks at the time when the transfer state was last logged
    unsigned recvChunks_; // Number of received chunks
    unsigned sentAcks_; // Number of sent acknowledgements
    unsigned outOrderChunks_; // Number of chunks received out of order
    unsigned outWindowChunks_; // Number of chunks received out of window
    unsigned dupChunks_; // Number of received duplicate chunks
#endif // OTA_UPDATE_STATS

    ProtocolError handleRequest(Message* msg, RequestHandlerFn handler);

    int handleStartRequest(const CoapMessageDecoder& d, CoapMessageEncoder* e, CoapMessageId** respId);
    int handleFinishRequest(const CoapMessageDecoder& d, CoapMessageEncoder* e, CoapMessageId** respId);
    int handleChunkRequest(const CoapMessageDecoder& d, CoapMessageEncoder* e, CoapMessageId** respId);

    static int decodeStartRequest(const CoapMessageDecoder& d, const char** fileHash, size_t* fileSize, size_t* chunkSize,
            bool* discardData);
    static int decodeFinishRequest(const CoapMessageDecoder& d, bool* cancelUpdate, bool* discardData);
    static int decodeChunkRequest(const CoapMessageDecoder& d, const char** chunkData, size_t* chunkSize,
            unsigned* chunkIndex);

    void initChunkAck(CoapMessageEncoder* e);

    int sendErrorResponse(Message* msg, int error, CoapType type, const char* token, size_t tokenSize);
    int sendEmptyAck(Message* msg, CoapType type, CoapMessageId id);

    system_tick_t millis() const;
};

inline FirmwareUpdate::FirmwareUpdate() :
        callbacks_(nullptr),
        channel_(nullptr),
        updating_(false) {
    reset();
}

inline FirmwareUpdate::~FirmwareUpdate() {
    destroy();
}

inline ProtocolError FirmwareUpdate::startRequest(Message* msg) {
    return handleRequest(msg, &FirmwareUpdate::handleStartRequest);
}

inline ProtocolError FirmwareUpdate::finishRequest(Message* msg) {
    return handleRequest(msg, &FirmwareUpdate::handleFinishRequest);
}

inline ProtocolError FirmwareUpdate::chunkRequest(Message* msg) {
    return handleRequest(msg, &FirmwareUpdate::handleChunkRequest);
}

inline bool FirmwareUpdate::isRunning() const {
    return updating_;
}

inline system_tick_t FirmwareUpdate::millis() const {
    return callbacks_->millis();
}

} // namespace particle::protocol

#endif // HAL_PLATFORM_OTA_PROTOCOL_V3
