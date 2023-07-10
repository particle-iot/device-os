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

#include "mbedtls_config.h"

#include <cstdint>
#include <cstddef>

namespace particle {

namespace protocol {

/**
 * Minimum supported chunk size.
 *
 * This parameter affects the size of the chunk bitmap maintained by the protocol implementation.
 */
const size_t MIN_OTA_CHUNK_SIZE = 512;

static_assert(MIN_OTA_CHUNK_SIZE % 4 == 0, "Invalid MIN_OTA_CHUNK_SIZE");

/**
 * CoAP overhead per chunk message:
 *
 * Message header (4 bytes)
 * Uri-Path option (2 bytes)
 * Chunk-Index option (4-7 bytes)
 * Payload marker (1 byte)
 */
const size_t OTA_CHUNK_COAP_OVERHEAD = 14;

static_assert(MBEDTLS_SSL_MAX_CONTENT_LEN >= MIN_OTA_CHUNK_SIZE + OTA_CHUNK_COAP_OVERHEAD,
        "MBEDTLS_SSL_MAX_CONTENT_LEN is too small");

/**
 * Maximum chunk size.
 */
const size_t MAX_OTA_CHUNK_SIZE = (MBEDTLS_SSL_MAX_CONTENT_LEN - OTA_CHUNK_COAP_OVERHEAD) / 4 * 4;

static_assert(MAX_OTA_CHUNK_SIZE >= MIN_OTA_CHUNK_SIZE, "Invalid MAX_OTA_CHUNK_SIZE");

/**
 * Size of the receiver window in bytes.
 *
 * Received chunks get consumed immediately, so the receiver window can be relatively large.
 * This parameter affects the size of the chunk bitmap maintained by the protocol implementation.
 */
const size_t OTA_RECEIVE_WINDOW_SIZE = 128 * 1024;

static_assert(OTA_RECEIVE_WINDOW_SIZE > MAX_OTA_CHUNK_SIZE, "Invalid OTA_RECEIVE_WINDOW_SIZE");

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
const system_tick_t OTA_CHUNK_ACK_DELAY = 200;

/**
 * Minimum number of chunks to receive before generating an acknowledgement.
 *
 * Setting this parameter to 1 disables delayed acknowledgements.
 */
const unsigned OTA_CHUNK_ACK_COUNT = 2;

/**
 * Maximum time to wait for the next chunk before timing out the transfer.
 */
const system_tick_t OTA_TRANSFER_TIMEOUT = 300000;

class Message;
class MessageChannel;
class CoapMessageDecoder;
class CoapMessageEncoder;

/**
 * OTA protocol v3 statistics.
 */
struct FirmwareUpdateStats {
    system_tick_t updateStartTime; // Time when the update started
    system_tick_t updateFinishTime; // Time when the update finished
    system_tick_t transferStartTime; // Time when the file transfer started
    system_tick_t transferFinishTime; // Time when the file transfer finished
    system_tick_t processingTime; // System processing time
    unsigned receivedChunks; // Total number of received chunks
    unsigned sentChunkAcks; // Number of sent acknowledgements
    unsigned outOfOrderChunks; // Number of chunks received out of order
    unsigned duplicateChunks; // Number of duplicate chunks received
};

/**
 * A class implementing the OTA update protocol v3.
 */
class FirmwareUpdate {
public:
    FirmwareUpdate();
    ~FirmwareUpdate();

    ProtocolError init(MessageChannel* channel, const SparkCallbacks& callbacks);
    void destroy();

    ProtocolError startRequest(Message* msg);
    ProtocolError finishRequest(Message* msg);
    ProtocolError chunkRequest(Message* msg);
    ProtocolError responseAck(Message* msg);
    ProtocolError process();

    const FirmwareUpdateStats& stats() const;

    bool isRunning() const;

    void reset();

private:
    typedef int (FirmwareUpdate::*RequestHandlerFn)(const CoapMessageDecoder& d, CoapMessageEncoder* e, int** respId,
            bool validateOnly);

    uint32_t chunks_[OTA_CHUNK_BITMAP_ELEMENTS]; // Bitmap of received chunks within the receiver window
    FirmwareUpdateStats stats_; // Protocol statistics
    const SparkCallbacks* callbacks_; // System callbacks
    MessageChannel* channel_; // Message channel
    system_tick_t lastChunkTime_; // Time when the last chunk was received
    system_tick_t stateLogTime_; // Time when the transfer state was last logged
    size_t fileSize_; // File size
    size_t fileOffset_; // Current offset in the file
    size_t transferSize_; // Total size of the data to transfer
    size_t chunkSize_; // Chunk size
    size_t chunkCount_; // Total number of chunks to transfer
    size_t windowSize_; // Size of the receiver window in chunks
    unsigned chunkIndex_; // Number of cumulatively acknowledged chunks
    unsigned unackChunks_; // Number or chunks received since the last acknowledgement
    unsigned stateLogChunks_; // Number of cumulatively acknowledged chunks at the time when the transfer state was last logged
    int finishRespId_; // Message ID of the UpdateFinish response
    int errorRespId_; // Message ID of the last confirmable error response sent to the server
    bool hasGaps_; // Whether the sequence of received chunks has gaps
    bool updating_; // Whether an update is in progress

    ProtocolError handleRequest(Message* msg, RequestHandlerFn handler);

    int handleStartRequest(const CoapMessageDecoder& d, CoapMessageEncoder* e, int** respId, bool validateOnly);
    int handleFinishRequest(const CoapMessageDecoder& d, CoapMessageEncoder* e, int** respId, bool validateOnly);
    int handleChunkRequest(const CoapMessageDecoder& d, CoapMessageEncoder* e, int** respId, bool validateOnly);

    static int decodeStartRequest(const CoapMessageDecoder& d, size_t* fileSize, const char** fileHash, size_t* chunkSize,
            bool* discardData, int* moduleFunction);
    static int decodeFinishRequest(const CoapMessageDecoder& d, bool* cancelUpdate, bool* discardData);
    static int decodeChunkRequest(const CoapMessageDecoder& d, const char** chunkData, size_t* chunkSize,
            unsigned* chunkIndex);

    void initChunkAck(CoapMessageEncoder* e);

    int sendErrorResponse(Message* msg, int error, CoapType type, int id, const char* token, size_t tokenSize);
    int sendEmptyAck(Message* msg, CoapType type, CoapMessageId id);

    void cancelUpdate();

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

inline const FirmwareUpdateStats& FirmwareUpdate::stats() const {
    return stats_;
}

inline bool FirmwareUpdate::isRunning() const {
    return updating_;
}

inline system_tick_t FirmwareUpdate::millis() const {
    return callbacks_->millis();
}

} // namespace protocol

} // namespace particle

#endif // HAL_PLATFORM_OTA_PROTOCOL_V3
