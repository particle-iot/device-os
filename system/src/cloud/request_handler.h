#pragma once

#include "coap_api.h"
#include "coap_util.h"

#include "cloud/cloud.pb.h"

namespace particle::system {

/**
 * Abstract handler for a cloud request.
 *
 * A request can be handled synchronously or asynchronously. The implementation may send a response
 * to the request at any point while handling it, using the helper methods of this class, such as
 * `sendProtobufResponse()`. If it hasn't sent a response, the caller is responsible for sending
 * a response.
 */
class CloudRequestHandler {
public:
    /**
     * Result codes returned by `handle()`.
     */
    enum Result {
        RUNNING = 1 ///< The request is being handled asynchronously.
    };

    /**
     * Completion callback.
     *
     * Invoked only if the request was handled asynchronously. The handler can be destroyed in the
     * callback.
     *
     * @param result 0 on success, otherwise an error code defined by `system_error_t`.
     * @param handler Request handler.
     * @param arg User argument.
     */
    typedef void (*CompleteFn)(int result, CloudRequestHandler* handler, void* arg);

    CloudRequestHandler() :
            completeFn_(nullptr),
            completeArg_(nullptr),
            reqId_(COAP_INVALID_REQUEST_ID),
            needResp_(false),
            running_(false) {
    }

    virtual ~CloudRequestHandler();

    /**
     * Handle a request.
     *
     * If this method returns `RUNNING`, the request will be processed asynchronously and the result
     * will be reported via the completion callback. Otherwise, the returned value is the result of
     * the operation.
     *
     * @param reqId Request ID.
     * @param req Request message.
     * @param arg User argument for the completion callback.
     * @param fn Completion callback.
     * @return 0 or `RUNNING` on success, otherwise an error code defined by `system_error_t`.
     */
    int handle(int reqId, CoapMessagePtr& req, void* arg = nullptr, CompleteFn fn = nullptr);

    bool needResponse() const {
        return needResp_;
    }

    int requestId() const {
        return reqId_;
    }

protected:
    /**
     * Handle a request.
     *
     * To handle the request asynchronously, the implementation returns `RUNNING` and calls
     * `notifyComplete()` when done.
     *
     * @param req Request message.
     * @return 0 or `RUNNING` on success, otherwise an error code defined by `system_error_t`.
     */
    virtual int handle(CoapMessagePtr& req) = 0;

    /**
     * Release the resources allocated for handling the request.
     *
     * Called when the request is handled.
     */
    virtual void reset() {
    }

    /**
     * Complete handling of the asynchronous request.
     *
     * The handler may be destroyed by the time this method returns so the implementation must not
     * access its members afterwards.
     *
     * @param result 0 on success, otherwise an error code defined by `system_error_t`.
     */
    void notifyComplete(int result = 0);

    ///@{
    /**
     * Helper methods for sending a response to the original request.
     *
     * Use these methods instead of the CoAP API or functions from protobuf.h directly as they keep
     * track of whether a response has been sent (see `needResponse()`).
     */
    int sendProtobufResponse(coap_status status, const particle_cloud_Response& resp, void* arg = nullptr,
            coap_ack_callback ackCb = nullptr, coap_error_callback errorCb = nullptr);

    int sendEmptyProtobufResponse(coap_status status, int result, const char* msg = nullptr, void* arg = nullptr,
            coap_ack_callback ackCb = nullptr, coap_error_callback errorCb = nullptr);
    ///@}

private:
    CompleteFn completeFn_;
    void* completeArg_;
    int reqId_;
    bool needResp_;
    bool running_;
};

} // namespace particle::system
