#pragma once

#include "coap_api.h"
#include "coap_util.h"

#include "cloud/cloud.pb.h"

namespace particle::system {

/**
 * Abstract CoAP request handler.
 */
class CloudRequestHandler {
public:
    typedef void (*CompleteFn)(int result, CloudRequestHandler* handler, void* arg);

    CloudRequestHandler() :
            completeFn_(nullptr),
            completeArg_(nullptr),
            reqId_(COAP_INVALID_REQUEST_ID),
            needResp_(false),
            running_(false) {
    }

    virtual ~CloudRequestHandler();

    int handle(int reqId, CoapMessagePtr& req, void* arg = nullptr, CompleteFn fn = nullptr);

    bool isRunning() const {
        return running_;
    }

    bool needResponse() const {
        return needResp_;
    }

    int requestId() const {
        return reqId_;
    }

protected:
    enum HandleResult {
        RUNNING = 1
    };

    virtual int handle(CoapMessagePtr& req) = 0;

    virtual void reset() {
    }

    int sendProtobufResponse(coap_status status, const particle_cloud_Response& resp, void* arg = nullptr,
            coap_ack_callback ackCb = nullptr, coap_error_callback errorCb = nullptr);
    int sendEmptyProtobufResponse(coap_status status, int result, const char* msg = nullptr, void* arg = nullptr,
            coap_ack_callback ackCb = nullptr, coap_error_callback errorCb = nullptr);

    void notifyComplete(int result = 0);

private:
    CompleteFn completeFn_;
    void* completeArg_;
    int reqId_;
    bool needResp_;
    bool running_;
};

} // namespace particle::system
