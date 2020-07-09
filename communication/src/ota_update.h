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

#include "ota_update_context.h"
#include "enumflags.h"

#include <cstdint>

namespace particle::protocol {

// Size of the chunk map in 32-bit words
const size_t OTA_UPDATE_CHUNK_MAP_SIZE = 10;

class Message;
class MessageChannel;

enum class OtaUpdateFlag {
    RESTART = 0x01,
    CANCEL = 0x02,
    VALIDATE_ONLY = 0x04
};

typedef EnumFlags<OtaUpdateFlag> OtaUpdateFlags;

class OtaUpdateContext {
public:
    explicit OtaUpdateContext(void* userData = nullptr);

    void errorMessage(const char* msg);
    void formatErrorMessage(const char* fmt, ...);
    const char* errorMessage() const;

    void* userData() const;

    void reset();

private:
    CString errMsg_;
    void* const userData_;
};

class OtaUpdateCallbacks {
public:
    typedef int (*StartUpdateFn)(size_t fileSize, const char* sha256, size_t* offset, OtaUpdateFlags flags,
            OtaUpdateContext* ctx);
    typedef int (*FinishUpdateFn)(OtaUpdateFlags flags, OtaUpdateContext* ctx);
    typedef int (*SaveChunkFn)(const char* data, size_t size, size_t offset, OtaUpdateContext* ctx);

    OtaUpdateCallbacks();

    OtaUpdateCallbacks& startUpdateFn(StartUpdateFn fn);
    StartUpdateFn startUpdateFn() const;

    OtaUpdateCallbacks& finishUpdateFn(FinishUpdateFn fn);
    FinishUpdateFn finishUpdateFn() const;

    OtaUpdateCallbacks& saveChunkFn(SaveChunkFn fn);
    SaveChunkFn saveChunkFn() const;

    OtaUpdateCallbacks& userData(void* data);
    void* userData() const;

private:
    StartUpdateFn startUpdateFn_;
    FinishUpdateFn finishUpdateFn_;
    SaveChunkFn saveChunkFn_;
    void* userData_;
};

/**
 * OTA update protocol v3.
 */
class OtaUpdate {
public:
    // Class-specific result codes
    enum Result {
        MESSAGE_NOT_FOUND = 1
    };

    OtaUpdate(MessageChannel* channel, OtaUpdateCallbacks callbacks);
    ~OtaUpdate();

    int receiveBegin(Message* msg);
    int receiveEnd(Message* msg);
    int receiveChunk(Message* msg);
    int receiveAck(Message* msg);

    int process();

    bool isActive() const;

    void reset();

private:
    enum class State {
        IDLE
    };

    uint32_t chunkMap_[OTA_UPDATE_CHUNK_MAP_SIZE];
    OtaUpdateContext ctx_;
    OtaUpdateCallbacks const callbacks_;
    MessageChannel* const channel_;
    State state_;

    int startUpdate(size_t fileSize, const char* sha256, size_t* offset, OtaUpdateFlags flags);
    int finishUpdate(OtaUpdateFlags flags);
    int saveChunk(const char* data, size_t size, size_t offset);
    const char* lastErrorMessage() const;
};

inline OtaUpdateContext::OtaUpdateContext(void* userData) :
        userData_(userData) {
}

inline void OtaUpdateContext::errorMessage(const char* msg) {
    errMsg_ = msg;
}

inline const char* OtaUpdateContext::errorMessage() const {
    return errMsg_;
}

inline void* OtaUpdateContext::userData() const {
    return userData_;
}

inline void OtaUpdateContext::reset() {
    errMsg_ = CString();
}

inline OtaUpdateCallbacks::OtaUpdateCallbacks() :
        startUpdateFn_(nullptr),
        finishUpdateFn_(nullptr),
        saveChunkFn_(nullptr),
        userData_(nullptr) {
}

inline OtaUpdateCallbacks& OtaUpdateCallbacks::startUpdateFn(StartUpdateFn fn) {
    startUpdateFn_ = fn;
    return *this;
}

inline OtaUpdateCallbacks::StartUpdateFn OtaUpdateCallbacks::startUpdateFn() const {
    return startUpdateFn_;
}

inline OtaUpdateCallbacks& OtaUpdateCallbacks::finishUpdateFn(FinishUpdateFn fn) {
    finishUpdateFn_ = fn;
    return *this;
}

inline OtaUpdateCallbacks::FinishUpdateFn OtaUpdateCallbacks::finishUpdateFn() const {
    return finishUpdateFn_;
}

inline OtaUpdateCallbacks& OtaUpdateCallbacks::saveChunkFn(SaveChunkFn fn) {
    saveChunkFn_ = fn;
    return *this;
}

inline OtaUpdateCallbacks::SaveChunkFn OtaUpdateCallbacks::saveChunkFn() const {
    return saveChunkFn_;
}

inline OtaUpdateCallbacks& OtaUpdateCallbacks::userData(void* data) {
    userData_ = data;
    return *this;
}

inline void* OtaUpdateCallbacks::userData() const {
    return userData_;
}

inline bool OtaUpdate::isActive() const {
    return state_ != State::IDLE;
}

inline int OtaUpdate::startUpdate(size_t fileSize, const char* sha256, size_t* offset, OtaUpdateFlags flags) {
    return callbacks_.startUpdateFn()(fileSize, sha256, offset, flags, &ctx_);
}

inline int OtaUpdate::finishUpdate(OtaUpdateFlags flags) {
    return callbacks_.finishUpdateFn()(flags, &ctx_);
}

inline int OtaUpdate::saveChunk(const char* data, size_t size, size_t offset) {
    return callbacks_.saveChunkFn()(data, size, offset, &ctx_);
}

inline const char* OtaUpdate::lastErrorMessage() const {
    return ctx_.errorMessage();
}

} // namespace particle::protocol

#endif // HAL_PLATFORM_OTA_PROTOCOL_V3
