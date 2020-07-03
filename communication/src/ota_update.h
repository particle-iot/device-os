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

#include "c_string.h"

#include <cstdint>

namespace particle::protocol {

// Size of the chunk map in 32-bit words
const size_t OTA_UPDATE_CHUNK_MAP_SIZE = 10;

class Message;
class MessageChannel;

// TODO: Move this struct to some public header
struct Sha256 {
    char data[32];
};

enum class OtaUpdateCommand {
    VALIDATE_UPDATE = 1,
    APPLY_UPDATE = 2,
    CANCEL_UPDATE = 3
};

class OtaUpdateContext {
public:
    explicit OtaUpdateContext(void* userData);

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
    typedef int (*PrepareFn)(size_t fileSize, const Sha256& sha256, size_t* offset, OtaUpdateContext* ctx);
    typedef int (*SaveChunkFn)(const char* data, size_t size, size_t offset, OtaUpdateContext* ctx);
    typedef int (*CommandFn)(OtaUpdateCommand cmd, OtaUpdateContext* ctx);

    OtaUpdateCallbacks();

    OtaUpdateCallbacks& prepareFn(PrepareFn fn);
    PrepareFn prepareFn() const;

    OtaUpdateCallbacks& saveChunkFn(SaveChunkFn fn);
    SaveChunkFn saveChunkFn() const;

    OtaUpdateCallbacks& commandFn(CommandFn fn);
    CommandFn commandFn() const;

    OtaUpdateCallbacks& userData(void* data);
    void* userData() const;

private:
    PrepareFn prepareFn_;
    SaveChunkFn saveChunkFn_;
    CommandFn commandFn_;
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

    bool isUpdating() const;

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

    int prepare(size_t fileSize, const Sha256& sha256, size_t* offset);
    int saveChunk(const char* data, size_t size, size_t offset);
    int command(OtaUpdateCommand cmd);
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
        prepareFn_(nullptr),
        saveChunkFn_(nullptr),
        commandFn_(nullptr),
        userData_(nullptr) {
}

inline OtaUpdateCallbacks& OtaUpdateCallbacks::prepareFn(PrepareFn fn) {
    prepareFn_ = fn;
    return *this;
}

inline OtaUpdateCallbacks::PrepareFn OtaUpdateCallbacks::prepareFn() const {
    return prepareFn_;
}

inline OtaUpdateCallbacks& OtaUpdateCallbacks::saveChunkFn(SaveChunkFn fn) {
    saveChunkFn_ = fn;
    return *this;
}

inline OtaUpdateCallbacks::SaveChunkFn OtaUpdateCallbacks::saveChunkFn() const {
    return saveChunkFn_;
}

inline OtaUpdateCallbacks& OtaUpdateCallbacks::commandFn(CommandFn fn) {
    commandFn_ = fn;
    return *this;
}

inline OtaUpdateCallbacks::CommandFn OtaUpdateCallbacks::commandFn() const {
    return commandFn_;
}

inline OtaUpdateCallbacks& OtaUpdateCallbacks::userData(void* data) {
    userData_ = data;
    return *this;
}

inline void* OtaUpdateCallbacks::userData() const {
    return userData_;
}

inline bool OtaUpdate::isUpdating() const {
    return state_ != State::IDLE;
}

inline int OtaUpdate::prepare(size_t fileSize, const Sha256& sha256, size_t* offset) {
    return callbacks_.prepareFn()(fileSize, sha256, offset, &ctx_);
}

inline int OtaUpdate::saveChunk(const char* data, size_t size, size_t offset) {
    return callbacks_.saveChunkFn()(data, size, offset, &ctx_);
}

inline int OtaUpdate::command(OtaUpdateCommand cmd) {
    return callbacks_.commandFn()(cmd, &ctx_);
}

} // namespace particle::protocol
