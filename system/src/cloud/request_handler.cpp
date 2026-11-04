#include "request_handler.h"
#include "protobuf.h"
#include "check.h"
#include "logging.h"

#define PB_CLOUD(_name) particle_cloud_##_name

namespace particle::system {

CloudRequestHandler::~CloudRequestHandler() {
    if (running_) {
        LOG(WARN, "Destroying CloudRequestHandler while it's running");
    }
}

int CloudRequestHandler::handle(int reqId, CoapMessagePtr& req, void* arg, CompleteFn fn) {
    if (running_) {
    	return SYSTEM_ERROR_INVALID_STATE;
    }
    reqId_ = reqId;
    completeFn_ = fn;
    completeArg_ = arg;
    needResp_ = true;
    int r = handle(req);
    if (r == Result::RUNNING) {
        running_ = true;
    } else {
        reset();
    }
    return r;
}

void CloudRequestHandler::notifyComplete(int result) {
    if (!running_ || !completeFn_) {
        return;
    }
    reset();
    running_ = false;
    completeFn_(result, this, completeArg_);
}

int CloudRequestHandler::sendProtobufResponse(coap_status status, const PB_CLOUD(Response)& pbResp, void* arg,
        coap_ack_callback ackCb, coap_error_callback errorCb) {
    if (!needResp_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    CHECK(system::sendProtobufResponse(reqId_, status, pbResp, arg, ackCb, errorCb));
    needResp_ = false;
    return 0;
}

int CloudRequestHandler::sendEmptyProtobufResponse(coap_status status, int result, const char* msg, void* arg,
        coap_ack_callback ackCb, coap_error_callback errorCb) {
    if (!needResp_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    CHECK(system::sendEmptyProtobufResponse(reqId_, status, result, msg, arg, ackCb, errorCb));
    needResp_ = false;
    return 0;
}

} // namespace particle::system
