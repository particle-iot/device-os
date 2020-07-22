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

#include "protocol_defs.h"
#include "coap_defs.h"

#include "system_defs.h"
#include "c_string.h"

#include <cstdint>
#include <cstddef>

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

class FirmwareUpdateContext {
public:
    explicit FirmwareUpdateContext(void* userData = nullptr);

    void errorMessage(CString msg);
    const CString& errorMessage() const;
    void clearErrorMessage();

    void* userData() const;

    void reset();

private:
    CString errMsg_;
    void* userData_;
};

class FirmwareUpdateCallbacks {
public:
    typedef int (*StartUpdateFn)(size_t fileSize, const char* fileHash, size_t* fileOffset, FirmwareUpdateFlags flags,
            FirmwareUpdateContext* ctx);
    typedef int (*FinishUpdateFn)(FirmwareUpdateFlags flags, FirmwareUpdateContext* ctx);
    typedef int (*SaveChunkFn)(const char* chunkData, size_t chunkSize, size_t chunkOffset, size_t partialSize,
            FirmwareUpdateContext* ctx);
    typedef system_tick_t (*MillisFn)();

    FirmwareUpdateCallbacks();

    FirmwareUpdateCallbacks& startUpdateFn(StartUpdateFn fn);
    StartUpdateFn startUpdateFn() const;

    FirmwareUpdateCallbacks& finishUpdateFn(FinishUpdateFn fn);
    FinishUpdateFn finishUpdateFn() const;

    FirmwareUpdateCallbacks& saveChunkFn(SaveChunkFn fn);
    SaveChunkFn saveChunkFn() const;

    FirmwareUpdateCallbacks& millisFn(MillisFn fn);
    MillisFn millisFn() const;

    FirmwareUpdateCallbacks& userData(void* data);
    void* userData() const;

private:
    StartUpdateFn startUpdateFn_;
    FinishUpdateFn finishUpdateFn_;
    SaveChunkFn saveChunkFn_;
    MillisFn millisFn_;
    void* userData_;
};

/**
 * OTA update protocol v3.
 */
class FirmwareUpdate {
public:
    FirmwareUpdate(MessageChannel* channel, FirmwareUpdateCallbacks callbacks);
    ~FirmwareUpdate();

    int beginRequest(Message* msg);
    int endRequest(Message* msg);
    int chunkRequest(Message* msg);
    int process();

    bool isRunning() const;

    void reset();

private:
    typedef int (FirmwareUpdate::*RequestHandlerFn)(const CoapMessageDecoder& d, Message* msg);

    FirmwareUpdateContext ctx_; // Update context
    FirmwareUpdateCallbacks const callbacks_; // Callbacks
    MessageChannel* const channel_; // Message channel
    bool updating_; // Whether an update is in progress

    uint32_t chunks_[OTA_CHUNK_BITMAP_ELEMENTS]; // Bitmap of received chunks within the receiver window
    system_tick_t updateStartTime_; // Time when the update started
    system_tick_t lastChunkTime_; // Time when the last chunk was received
    size_t fileSize_; // File size
    size_t fileOffset_; // Current offset in the file
    size_t transferSize_; // Total size of the data to transfer
    size_t chunkSize_; // Chunk size
    size_t chunkCount_; // Total number of chunks to transfer
    size_t windowSize_; // Size of the receiver window in chunks
    unsigned chunkIndex_; // Index of the chunk at the left edge of the receiver window
    unsigned unackChunks_; // Number or chunks received since the last acknowledgement
    bool hasChunkGaps_; // Whether there are gaps in the sequence of received chunks

#if OTA_UPDATE_STATS
    system_tick_t processTime_; // System processing time
    unsigned recvChunks_; // Number of received chunks
    unsigned sentAcks_; // Number of sent acknowledgements
    unsigned outOrderChunks_; // Number of chunks received out of order
    unsigned outWindowChunks_; // Number of chunks received out of window
    unsigned dupChunks_; // Number of received duplicate chunks
#endif // OTA_UPDATE_STATS

    // Note: This method returns ProtocolError
    int handleRequest(Message* msg, RequestHandlerFn handler);

    int handleBeginRequest(const CoapMessageDecoder& d, Message* msg);
    int handleEndRequest(const CoapMessageDecoder& d, Message* msg);
    int handleChunkRequest(const CoapMessageDecoder& d, Message* msg);

    int sendErrorResponse(int error, CoapType type, CoapMessageId id, const char* token, size_t tokenSize);
    int sendEmptyAck(Message* origMsg, CoapMessageId id);

    static int parseBeginRequest(const CoapMessageDecoder& d, const char** fileHash, size_t* fileSize, size_t* chunkSize,
            bool* discardData);
    static int parseEndRequest(const CoapMessageDecoder& d, bool* discardData, bool* cancelUpdate);
    static int parseChunkRequest(const CoapMessageDecoder& d, const char** chunkData, size_t* chunkSize, size_t* chunkIndex);

    int startUpdate(size_t fileSize, const char* fileHash, size_t* fileOffset, FirmwareUpdateFlags flags);
    int finishUpdate(FirmwareUpdateFlags flags);
    int saveChunk(const char* chunkData, size_t chunkSize, size_t chunkOffset, size_t partialSize);
    system_tick_t millis() const;
};

inline FirmwareUpdateContext::FirmwareUpdateContext(void* userData) :
        userData_(userData) {
}

inline void FirmwareUpdateContext::errorMessage(CString msg) {
    errMsg_ = std::move(msg);
}

inline const CString& FirmwareUpdateContext::errorMessage() const {
    return errMsg_;
}

inline void FirmwareUpdateContext::clearErrorMessage() {
    errMsg_ = CString();
}

inline void* FirmwareUpdateContext::userData() const {
    return userData_;
}

inline void FirmwareUpdateContext::reset() {
    errMsg_ = CString();
}

inline FirmwareUpdateCallbacks::FirmwareUpdateCallbacks() :
        startUpdateFn_(nullptr),
        finishUpdateFn_(nullptr),
        saveChunkFn_(nullptr),
        millisFn_(nullptr),
        userData_(nullptr) {
}

inline FirmwareUpdateCallbacks& FirmwareUpdateCallbacks::startUpdateFn(StartUpdateFn fn) {
    startUpdateFn_ = fn;
    return *this;
}

inline FirmwareUpdateCallbacks::StartUpdateFn FirmwareUpdateCallbacks::startUpdateFn() const {
    return startUpdateFn_;
}

inline FirmwareUpdateCallbacks& FirmwareUpdateCallbacks::finishUpdateFn(FinishUpdateFn fn) {
    finishUpdateFn_ = fn;
    return *this;
}

inline FirmwareUpdateCallbacks::FinishUpdateFn FirmwareUpdateCallbacks::finishUpdateFn() const {
    return finishUpdateFn_;
}

inline FirmwareUpdateCallbacks& FirmwareUpdateCallbacks::saveChunkFn(SaveChunkFn fn) {
    saveChunkFn_ = fn;
    return *this;
}

inline FirmwareUpdateCallbacks::SaveChunkFn FirmwareUpdateCallbacks::saveChunkFn() const {
    return saveChunkFn_;
}

inline FirmwareUpdateCallbacks& FirmwareUpdateCallbacks::millisFn(MillisFn fn) {
    millisFn_ = fn;
    return *this;
}

inline FirmwareUpdateCallbacks::MillisFn FirmwareUpdateCallbacks::millisFn() const {
    return millisFn_;
}

inline FirmwareUpdateCallbacks& FirmwareUpdateCallbacks::userData(void* data) {
    userData_ = data;
    return *this;
}

inline void* FirmwareUpdateCallbacks::userData() const {
    return userData_;
}

inline int FirmwareUpdate::beginRequest(Message* msg) {
    return handleRequest(msg, &FirmwareUpdate::handleBeginRequest);
}

inline int FirmwareUpdate::endRequest(Message* msg) {
    return handleRequest(msg, &FirmwareUpdate::handleEndRequest);
}

inline int FirmwareUpdate::chunkRequest(Message* msg) {
    return handleRequest(msg, &FirmwareUpdate::handleChunkRequest);
}

inline bool FirmwareUpdate::isRunning() const {
    return updating_;
}

inline int FirmwareUpdate::startUpdate(size_t fileSize, const char* fileHash, size_t* fileOffset, FirmwareUpdateFlags flags) {
#if OTA_UPDATE_STATS
    const auto t1 = millis();
#endif
    const int r = callbacks_.startUpdateFn()(fileSize, fileHash, fileOffset, flags, &ctx_);
#if OTA_UPDATE_STATS
    processTime_ += millis() - t1;
#endif
    return r;
}

inline int FirmwareUpdate::finishUpdate(FirmwareUpdateFlags flags) {
#if OTA_UPDATE_STATS
    const auto t1 = millis();
#endif
    const int r = callbacks_.finishUpdateFn()(flags, &ctx_);
#if OTA_UPDATE_STATS
    processTime_ += millis() - t1;
#endif
    return r;
}

inline int FirmwareUpdate::saveChunk(const char* chunkData, size_t chunkSize, size_t chunkOffset, size_t partialSize) {
#if OTA_UPDATE_STATS
    const auto t1 = millis();
#endif
    const int r = callbacks_.saveChunkFn()(chunkData, chunkSize, chunkOffset, partialSize, &ctx_);
#if OTA_UPDATE_STATS
    processTime_ += millis() - t1;
#endif
    return r;
}

inline system_tick_t FirmwareUpdate::millis() const {
    return callbacks_.millisFn()();
}

} // namespace particle::protocol

#endif // HAL_PLATFORM_OTA_PROTOCOL_V3
