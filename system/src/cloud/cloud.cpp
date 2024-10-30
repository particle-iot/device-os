/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
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

#include <memory>
#include <cstdio>

#include "cloud.h"
#include "event.h"

#include "spark_protocol_functions.h"

#include "coap_api.h"
#include "coap_util.h"

#include "str_util.h"
#include "check.h"

namespace particle::system::cloud {

namespace {

template<typename... ArgsT>
int formatUri(char* buf, size_t size, ArgsT&&... args) {
    int n = std::snprintf(buf, size, std::forward<ArgsT>(args)...);
    if (n < 0) {
        return SYSTEM_ERROR_INTERNAL;
    }
    return n;
}

class PublishCtx {
public:
    static int create(RefCountPtr<Event> event) {
        std::unique_ptr<PublishCtx> ctx(new(std::nothrow) PublishCtx());
        if (!ctx) {
            return SYSTEM_ERROR_NO_MEMORY;
        }

        char uri[COAP_MAX_URI_PATH_LENGTH + 1];
        CHECK(formatUri(uri, sizeof(uri), "E/%s", event->name()));

        coap_message* apiMsg = nullptr;
        CHECK(coap_begin_request(&apiMsg, uri, COAP_METHOD_POST, 0 /* timeout */, 0 /* flags */, nullptr /* reserved */));
        CoapMessagePtr msg(apiMsg);

        auto apiPayload = event->payload();
        CHECK(coap_set_payload(msg.get(), apiPayload, nullptr /* reserved */));

        ctx->event_ = std::move(event);
        ctx->msg_ = std::move(msg);

        CHECK(ctx->sendNext());
        ctx.release();
        return 0;
    }

private:
    RefCountPtr<Event> event_;
    CoapMessagePtr msg_;

    PublishCtx() {
    }

    // FIXME: Move this to the CoAP API implementation
    int sendNext() {
        CoapPayloadPtr payload;
        CHECK(getPayload(payload));

        char buf[128];
        bool eof = false;

        for(;;) {
            int r = coap_read_payload(payload.get(), buf, sizeof(buf), nullptr /* reserved */);
            if (r < 0) {
                if (r == SYSTEM_ERROR_END_OF_STREAM) {
                    eof = true;
                    break;
                }
                return r;
            }
            size_t bytesRead = r;
            size_t bytesWritten = bytesRead;
            r = CHECK(coap_write_block(msg_.get(), buf, &bytesWritten, blockCallback, errorCallback, this, nullptr /* reserved */));
            if (r == COAP_RESULT_WAIT_BLOCK) {
                size_t pos = CHECK(coap_get_payload_pos(payload.get(), nullptr /* reserved */));
                pos -= bytesRead - bytesWritten;
                CHECK(coap_set_payload_pos(payload.get(), pos, nullptr /* reserved */));
                break;
            }
        }

        if (eof) {
            CHECK(coap_end_request(msg_.get(), nullptr /* resp_cb */, ackCallback, errorCallback, this, nullptr /* reserved */));
        }
        return 0;
    }

    int getPayload(CoapPayloadPtr& payload) const {
        coap_payload* apiPayload = nullptr;
        CHECK(coap_get_payload(msg_.get(), &apiPayload, nullptr /* reserved */));
        payload.reset(apiPayload);
        return 0;
    }

    static int blockCallback(coap_message* msg, int reqId, void* arg) {
        auto self = static_cast<PublishCtx*>(arg);
        if (!self->event_) {
            return 0;
        }
        int r = self->sendNext();
        if (r < 0) {
            delete self;
            return r;
        }
        return 0;
    }

    static int ackCallback(int reqId, void* arg) {
        auto self = static_cast<PublishCtx*>(arg);
        if (!self->event_) {
            return 0;
        }
        self->event_->publishComplete(0 /* error */);
        delete self;
        return 0;
    }

    static void errorCallback(int error, int reqId, void* arg) {
        auto self = static_cast<PublishCtx*>(arg);
        if (!self->event_) {
            return;
        }
        self->event_->publishComplete(error);
        delete self;
    }
};

} // namespace

int Cloud::init() {
    CHECK(coap_add_request_handler("E", COAP_METHOD_POST, 0 /* flags */, coapRequestCallback, this, nullptr /* reserved */));
    return 0;
}

int Cloud::publish(RefCountPtr<Event> event) {
    int r = publishImpl(event);
    if (r < 0) {
        event->publishComplete(r);
    }
    return r;
}

int Cloud::subscribe(const char* prefix, cloud_event_subscribe_callback handler, void* arg) {
    Subscription sub;
    sub.prefix = prefix;
    if (!sub.prefix && prefix) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    sub.prefixLen = std::strlen(prefix);
    sub.handler = handler;
    sub.handlerArg = arg;
    if (!subs_.append(std::move(sub))) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    auto protocol = spark_protocol_instance();
    bool ok = spark_protocol_send_subscription(protocol, prefix, 0 /* flags */, nullptr /* reserved */);
    if (!ok) {
        LOG(ERROR, "spark_protocol_send_subscription() failed");
    }
    return 0;
}

Cloud* Cloud::instance() {
    static Cloud cloud;
    return &cloud;
}

int Cloud::publishImpl(RefCountPtr<Event> event) {
    CHECK(event->prepareForPublish());
    CHECK(PublishCtx::create(std::move(event)));
    return 0;
}

int Cloud::coapRequestCallback(coap_message* apiMsg, const char* uri, int method, int reqId, void* arg) {
    auto self = static_cast<Cloud*>(arg);

    CoapMessagePtr msg(apiMsg);

    const char* eventName = uri + 3; // Skip the "/E/" part
    size_t eventNameLen = std::strlen(eventName);

    // Find a subscription handler
    cloud_event_subscribe_callback handler = nullptr;
    void* handlerArg = nullptr;
    for (auto& sub: self->subs_) {
        if (startsWith(eventName, eventNameLen, sub.prefix, sub.prefixLen)) {
            handler = sub.handler;
            handlerArg = sub.handlerArg;
            break;
        }
    }
    if (!handler) {
        return 0; // Ignore event
    }

    auto ev = makeRefCountPtr<Event>();
    if (!ev) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    CHECK(ev->name(eventName)); 

    char buf[128];
    for (;;) {
        size_t size = sizeof(buf);
        int r = coap_read_block(msg.get(), buf, &size, nullptr /* block_cb */, nullptr /* error_cb */, nullptr /* arg */, nullptr /* reserved */);
        if (r < 0) {
            if (r == SYSTEM_ERROR_END_OF_STREAM) {
                break;
            }
            return r;
        }
        if (r == COAP_RESULT_WAIT_BLOCK) {
            return SYSTEM_ERROR_NOT_SUPPORTED; // TODO
        }
        CHECK(ev->write(buf, size));
    }
    CHECK(ev->seek(0));

    handler(reinterpret_cast<cloud_event*>(ev.unwrap()), handlerArg);

    return 0;
}

} // namespace particle::system::cloud
