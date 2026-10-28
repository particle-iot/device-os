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
    if (r != HandleResult::RUNNING) {
        reset();
        return r;
    }
    running_ = true;
    return 0;
}

int CloudRequestHandler::sendProtobufResponse(coap_status status, const PB_CLOUD(Response)& pbResp, void* arg,
        coap_ack_callback ackCb, coap_error_callback errorCb) {
    if (!needResp_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    needResp_ = false;
    CHECK(system::sendProtobufResponse(reqId_, status, pbResp, arg, ackCb, errorCb));
    return 0;
}

int CloudRequestHandler::sendEmptyProtobufResponse(coap_status status, int result, const char* msg, void* arg,
        coap_ack_callback ackCb, coap_error_callback errorCb) {
    if (!needResp_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    needResp_ = false;
    CHECK(system::sendEmptyProtobufResponse(reqId_, status, result, msg, arg, ackCb, errorCb));
    return 0;
}

void CloudRequestHandler::notifyComplete(int result) {
    if (!running_ || !completeFn_) {
        return;
    }
    running_ = false;
    reset();
    completeFn_(result, this, completeArg_);
}

} // namespace particle::system
