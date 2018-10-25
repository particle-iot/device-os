/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#include "platform_ncp.h"

namespace particle {

class AtParser;
class InputStream;

enum class NcpState {
    OFF = 0,
    ON = 1,
    DISABLED = 2
};

enum class NcpConnectionState {
    DISCONNECTED = 0,
    CONNECTING = 1,
    CONNECTED = 2
};

struct NcpEvent {
    enum Type {
        NCP_STATE_CHANGED = 1,
        CONNECTION_STATE_CHANGED = 2,
        CUSTOM_EVENT_TYPE_BASE = 100
    };

    int type;
};

struct NcpStateChangedEvent: NcpEvent {
    NcpState state;
};

struct NcpConnectionStateChangedEvent: NcpEvent {
    NcpConnectionState state;
};

typedef void(*NcpEventHandler)(const NcpEvent& event, void* data);
typedef void(*NcpDataHandler)(int id, const uint8_t* data, size_t size, void* ctx);

class NcpClientConfig {
public:
    NcpClientConfig();

    NcpClientConfig& eventHandler(NcpEventHandler handler, void* data = nullptr);
    NcpEventHandler eventHandler() const;
    void* eventHandlerData() const;

    NcpClientConfig& dataHandler(NcpDataHandler handler, void* data = nullptr);
    NcpDataHandler dataHandler() const;
    void* dataHandlerData() const;

private:
    NcpEventHandler eventHandler_;
    void* eventHandlerData_;
    NcpDataHandler dataHandler_;
    void* dataHandlerData_;
};

class NcpClient {
public:
    virtual ~NcpClient() = default;

    virtual int init(const NcpClientConfig& conf) = 0;
    virtual void destroy() = 0;

    virtual int on() = 0;
    virtual int off() = 0;
    virtual int enable() = 0;
    virtual void disable() = 0;
    virtual NcpState ncpState() = 0;

    virtual int disconnect() = 0;
    virtual NcpConnectionState connectionState() = 0;

    virtual int getFirmwareVersionString(char* buf, size_t size) = 0;
    virtual int getFirmwareModuleVersion(uint16_t* ver) = 0;
    virtual int updateFirmware(InputStream* file, size_t size) = 0;

    virtual int dataChannelWrite(int id, const uint8_t* data, size_t size) = 0;
    virtual void processEvents() = 0;

    virtual AtParser* atParser();

    virtual void lock() = 0;
    virtual void unlock() = 0;

    virtual int ncpId() const = 0;
};

class NcpClientLock {
public:
    explicit NcpClientLock(NcpClient* client);
    NcpClientLock(NcpClientLock&& lock);
    ~NcpClientLock();

    void lock();
    void unlock();

    NcpClientLock(const NcpClientLock&) = delete;
    NcpClientLock& operator=(const NcpClientLock&) = delete;

private:
    NcpClient* client_;
    bool locked_;
};

inline NcpClientConfig::NcpClientConfig() :
        eventHandler_(nullptr),
        eventHandlerData_(nullptr) {
}

inline NcpClientConfig& NcpClientConfig::eventHandler(NcpEventHandler handler, void* data) {
    eventHandler_ = handler;
    eventHandlerData_ = data;
    return *this;
}

inline NcpEventHandler NcpClientConfig::eventHandler() const {
    return eventHandler_;
}

inline void* NcpClientConfig::eventHandlerData() const {
    return eventHandlerData_;
}

inline NcpClientConfig& NcpClientConfig::dataHandler(NcpDataHandler handler, void* data) {
    dataHandler_ = handler;
    dataHandlerData_ = data;
    return *this;
}

inline NcpDataHandler NcpClientConfig::dataHandler() const {
    return dataHandler_;
}

inline void* NcpClientConfig::dataHandlerData() const {
    return dataHandlerData_;
}


inline NcpClientLock::NcpClientLock(NcpClient* client) :
        client_(client),
        locked_(false) {
    lock();
}

inline NcpClientLock::NcpClientLock(NcpClientLock&& lock) :
        client_(lock.client_),
        locked_(lock.locked_) {
    lock.client_ = nullptr;
    lock.locked_ = false;
}

inline NcpClientLock::~NcpClientLock() {
    if (locked_) {
        unlock();
    }
}

inline void NcpClientLock::lock() {
    client_->lock();
    locked_ = true;
}

inline void NcpClientLock::unlock() {
    client_->unlock();
    locked_ = false;
}

} // particle
