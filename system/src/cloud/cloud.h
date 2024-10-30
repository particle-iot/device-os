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

#pragma once

#include "system_cloud_event.h"

#include "c_string.h"
#include "ref_count.h"

#include "spark_wiring_vector.h"

struct coap_message;

namespace particle::system::cloud {

class Event;

class Cloud {
public:
    int init();

    int publish(RefCountPtr<Event> event);
    int subscribe(const char* prefix, cloud_event_subscribe_callback handler, void* arg);

    static Cloud* instance();

private:
    struct Subscription {
        CString prefix;
        cloud_event_subscribe_callback handler;
        void* handlerArg;
        size_t prefixLen;
    };

    Vector<Subscription> subs_; // TODO: Use a map

    int publishImpl(RefCountPtr<Event> event);

    static int coapRequestCallback(coap_message* msg, const char* uri, int method, int reqId, void* arg);
};

} // namespace particle::system::cloud
