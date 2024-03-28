#pragma once

#include <functional>
#include <cstdint>

#include "mqtt_client.h"
#include "util/buffer.h"

namespace particle {

class LorawanBase {
public:
    typedef std::function<void()> ConnectedFn;
    typedef std::function<void(int error)> DisconnectedFn;
    typedef std::function<void(util::Buffer data, int port)> MessageReceivedFn;
    typedef std::function<void(int error)> AckReceivedFn;
};

class LorawanConfig {
public:
    LorawanConfig() = default;

    LorawanConfig& connected(LorawanBase::ConnectedFn fn) {
        connFn_ = std::move(fn);
        return *this;
    }

    const LorawanBase::ConnectedFn& connected() const {
        return connFn_;
    }

    LorawanConfig& disconnected(LorawanBase::DisconnectedFn fn) {
        disconnFn_ = std::move(fn);
        return *this;
    }

    const LorawanBase::DisconnectedFn& disconnected() const {
        return disconnFn_;
    }

    LorawanConfig& messageReceived(LorawanBase::MessageReceivedFn fn) {
        msgRecvFn_ = std::move(fn);
        return *this;
    }

    const LorawanBase::MessageReceivedFn& messageReceived() const {
        return msgRecvFn_;
    }

private:
    LorawanBase::ConnectedFn connFn_;
    LorawanBase::DisconnectedFn disconnFn_;
    LorawanBase::MessageReceivedFn msgRecvFn_;
};

class Lorawan: public LorawanBase {
public:
    Lorawan() :
            fcntUp_(0),
            inited_(false) {
    }

    int init(LorawanConfig conf);

    int connect();
    int send(util::Buffer data, int port, AckReceivedFn ackFn);

    int run();

private:
    MqttClient mqtt_;
    LorawanConfig conf_;
    uint32_t fcntUp_;
    bool inited_;
};

} // namespace particle
