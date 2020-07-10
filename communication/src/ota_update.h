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

#include "system_defs.h"
#include "c_string.h"

#include <cstdint>
#include <cstddef>

namespace particle::protocol {

class Message;
class MessageChannel;

class FirmwareUpdateContext {
public:
    explicit FirmwareUpdateContext(void* userData = nullptr);

    void errorMessage(CString msg);
    const CString& errorMessage() const;

    void* userData() const;

    void reset();

private:
    CString errMsg_;
    void* userData_;
};

class OtaUpdateCallbacks {
public:
    typedef int (*StartUpdateFn)(size_t fileSize, const char* fileHash, size_t* fileOffset, FirmwareUpdateFlags flags,
            FirmwareUpdateContext* ctx);
    typedef int (*ValidateUpdateFn)(FirmwareUpdateContext* ctx);
    typedef int (*FinishUpdateFn)(FirmwareUpdateFlags flags, FirmwareUpdateContext* ctx);
    typedef int (*SaveChunkFn)(const char* chunkData, size_t chunkSize, size_t chunkOffset, size_t partialSize,
            FirmwareUpdateContext* ctx);

    OtaUpdateCallbacks();

    OtaUpdateCallbacks& startUpdateFn(StartUpdateFn fn);
    StartUpdateFn startUpdateFn() const;

    OtaUpdateCallbacks& validateUpdateFn(ValidateUpdateFn fn);
    ValidateUpdateFn validateUpdateFn() const;

    OtaUpdateCallbacks& finishUpdateFn(FinishUpdateFn fn);
    FinishUpdateFn finishUpdateFn() const;

    OtaUpdateCallbacks& saveChunkFn(SaveChunkFn fn);
    SaveChunkFn saveChunkFn() const;

    OtaUpdateCallbacks& userData(void* data);
    void* userData() const;

private:
    StartUpdateFn startUpdateFn_;
    ValidateUpdateFn validateUpdateFn_;
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

    FirmwareUpdateContext ctx_;
    OtaUpdateCallbacks const callbacks_;
    MessageChannel* const channel_;
    State state_;

    int startUpdate(size_t fileSize, const char* fileHash, size_t* fileOffset, FirmwareUpdateFlags flags);
    int validateUpdate();
    int finishUpdate(FirmwareUpdateFlags flags);
    int saveChunk(const char* chunkData, size_t chunkSize, size_t chunkOffset, size_t partialSize);
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

inline void* FirmwareUpdateContext::userData() const {
    return userData_;
}

inline void FirmwareUpdateContext::reset() {
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

inline int OtaUpdate::startUpdate(size_t fileSize, const char* fileHash, size_t* fileOffset, FirmwareUpdateFlags flags) {
    return callbacks_.startUpdateFn()(fileSize, fileHash, fileOffset, flags, &ctx_);
}

inline int OtaUpdate::validateUpdate() {
    return callbacks_.validateUpdateFn()(&ctx_);
}

inline int OtaUpdate::finishUpdate(FirmwareUpdateFlags flags) {
    return callbacks_.finishUpdateFn()(flags, &ctx_);
}

inline int OtaUpdate::saveChunk(const char* chunkData, size_t chunkSize, size_t chunkOffset, size_t partialSize) {
    return callbacks_.saveChunkFn()(chunkData, chunkSize, chunkOffset, partialSize, &ctx_);
}

} // namespace particle::protocol

#endif // HAL_PLATFORM_OTA_PROTOCOL_V3
