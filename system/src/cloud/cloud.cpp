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

#include <cstdio>

#include "cloud.h"
#include "event.h"

#include "coap_api.h"
#include "coap_util.h"

#include "check.h"

namespace particle::system::cloud {

namespace {

const size_t MAX_URI_LEN = 127;

template<typename... ArgsT>
int formatUri(char* buf, size_t size, ArgsT&&... args) {
    int n = std::snprintf(buf, size, std::forward<ArgsT>(args)...);
    if (n < 0) {
        return SYSTEM_ERROR_INTERNAL;
    }
    return n;
}

} // namespace

int Cloud::publish(RefCountPtr<Event> event) {
    int r = publishImpl(std::move(event));
    if (r < 0) {
        event->publishComplete(r);
    }
    return 0;
}

Cloud* Cloud::instance() {
    static Cloud cloud;
    return &cloud;
}

int Cloud::publishImpl(RefCountPtr<Event> event) {
/*
    // FIXME
    char uri[MAX_URI_LEN + 1];
    CHECK(formatUri(uri, sizeof(uri), "E/%s", event->name()));
*/
    coap_message* apiMsg = nullptr;
    /* int reqId = */ CHECK(coap_begin_request(&apiMsg, "E", COAP_METHOD_POST, 0 /* timeout */, 0 /* flags */, nullptr /* reserved */));
    CoapMessagePtr msg(apiMsg);

    CHECK(coap_add_string_option(msg.get(), COAP_OPTION_URI_PATH, event->name(), nullptr /* reserved */)); // FIXME

    size_t size = event->size();
    CHECK(coap_write_payload(msg.get(), event->data(), &size, nullptr /* block_cb */, nullptr /* error_cb */,
            nullptr /* arg */, nullptr /* reserved */));
    CHECK(coap_end_request(msg.get(), nullptr /* resp_cb */, coapAckCallback, coapErrorCallback, event.get(), nullptr /* reserved */));
    event->addRef();

    return 0;
}

int Cloud::coapAckCallback(int reqId, void* arg) {
    auto event = RefCountPtr<Event>::wrap(static_cast<Event*>(arg));
    event->publishComplete(0 /* error */);
    return 0;
}

void Cloud::coapErrorCallback(int error, int reqId, void* arg) {
    auto event = RefCountPtr<Event>::wrap(static_cast<Event*>(arg));
    event->publishComplete(error);
}

} // namespace particle::system::cloud
