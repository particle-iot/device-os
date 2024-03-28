#pragma once

#include <functional>
#include <optional>

#include <spark_wiring_ticks.h>
#include <spark_wiring_map.h>
#include <ref_count.h>

#include "util/buffer.h"

namespace particle::constrained {

using util::Buffer;

class ProtocolBase {
public:
    typedef std::function<void()> ConnectedFn;
    typedef std::function<void(int error)> DisconnectedFn;
    typedef std::function<void(int error)> AckReceivedFn;
    typedef std::function<void(int error, int result, Buffer data)> ResponseReceivedFn;
    typedef std::function<int(Buffer data, int port, AckReceivedFn ackFn)> SendMessageFn;

    static const system_tick_t DEFAULT_REQUEST_TIMEOUT = 60000;
    static const unsigned DEFAULT_PORT = 1;
};

class ProtocolConfig {
public:
    ProtocolConfig() :
            reqTimeout_(ProtocolBase::DEFAULT_REQUEST_TIMEOUT),
            port_(ProtocolBase::DEFAULT_PORT) {
    }

    ProtocolConfig& connected(ProtocolBase::ConnectedFn fn) {
        connFn_ = std::move(fn);
        return *this;
    }

    const ProtocolBase::ConnectedFn& connected() const {
        return connFn_;
    }

    ProtocolConfig& disconnected(ProtocolBase::DisconnectedFn fn) {
        disconnFn_ = std::move(fn);
        return *this;
    }

    const ProtocolBase::DisconnectedFn& disconnected() const {
        return disconnFn_;
    }

    ProtocolConfig& sendMessage(ProtocolBase::SendMessageFn fn) {
        sendMsgFn_ = std::move(fn);
        return *this;
    }

    const ProtocolBase::SendMessageFn& sendMessage() const {
        return sendMsgFn_;
    }

    ProtocolConfig& requestTimeout(system_tick_t timeout) {
        reqTimeout_ = timeout;
        return *this;
    }

    system_tick_t requestTimeout() const {
        return reqTimeout_;
    }

    ProtocolConfig& port(unsigned port) {
        port_ = port;
        return *this;
    }

    unsigned port() const {
        return port_;
    }

private:
    ProtocolBase::ConnectedFn connFn_;
    ProtocolBase::DisconnectedFn disconnFn_;
    ProtocolBase::SendMessageFn sendMsgFn_;
    system_tick_t reqTimeout_;
    unsigned port_;
};

class RequestOptions {
public:
    RequestOptions() :
            noResp_(false) {
    }

    RequestOptions& timeout(system_tick_t timeout) {
        timeout_ = timeout;
        return *this;
    }

    system_tick_t timeout() const {
        return timeout_.value_or(0);
    }

    bool hasTimeout() const {
        return timeout_.has_value();
    }

    RequestOptions& noResponse(bool enabled) {
        noResp_ = enabled;
        return *this;
    }

    bool noResponse() const {
        return noResp_;
    }

private:
    std::optional<system_tick_t> timeout_;
    bool noResp_;
};

class Protocol: public ProtocolBase {
public:
    Protocol();
    ~Protocol();

    int init(ProtocolConfig conf);

    int connect();
    void disconnect();
    int receiveMessage(int port, Buffer data);
    int updateMaxPayloadSize(size_t size);
    int run();

    int sendRequest(unsigned type, Buffer data, ResponseReceivedFn respFn, RequestOptions opts = RequestOptions());

private:
    enum class State {
        NEW,
        DISCONNECTED,
        CONNECTED
    };

    struct InRequest;
    struct OutRequest;

    Map<unsigned, RefCountPtr<OutRequest>> outReqs_; // Pending outgoing requests
    ProtocolConfig conf_; // Protocol configuration
    State state_; // Protocol state
    size_t maxPayloadSize_; // Maximum size of application data that can be transmitted in a single transport packet
    unsigned nextOutReqId_; // Next ID to use with an outgoing request
};

} // namespace particle::constrained
