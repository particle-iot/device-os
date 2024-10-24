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

#include "system_cloud_event.h"
#include "system_threading.h"

#include "cloud/cloud.h"
#include "cloud/event.h"

#include "check.h"

using namespace particle;
using namespace particle::system::cloud;

int cloud_event_create(cloud_event** event, void* reserved) {
    auto ev = makeRefCountPtr<Event>();
    if (!ev) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    *event = reinterpret_cast<cloud_event*>(ev.unwrap());
    return 0;
}

void cloud_event_add_ref(cloud_event* event, void* reserved) {
    auto ev = reinterpret_cast<Event*>(event);
    ev->addRef();
}

void cloud_event_release(cloud_event* event, void* reserved) {
    if (event) {
        auto ev = reinterpret_cast<Event*>(event);
        ev->release();
    }
}

int cloud_event_set_name(cloud_event* event, const char* name, void* reserved) {
    auto ev = reinterpret_cast<Event*>(event);
    CHECK(ev->name(name));
    return 0;
}

const char* cloud_event_get_name(cloud_event* event, void* reserved) {
    auto ev = reinterpret_cast<Event*>(event);
    return ev->name();
}

int cloud_event_set_properties(cloud_event* event, const cloud_event_properties* prop, void* reserved) {
    // TODO
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int cloud_event_get_properties(cloud_event* event, cloud_event_properties* prop, void* reserved) {
    // TODO
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int cloud_event_read(cloud_event* event, char* data, size_t size, void* reserved) {
    auto ev = reinterpret_cast<Event*>(event);
    size_t n = CHECK(ev->read(data, size));
    return n;
}

int cloud_event_peek(cloud_event* event, char* data, size_t size, void* reserved) {
    auto ev = reinterpret_cast<Event*>(event);
    size_t n = CHECK(ev->peek(data, size));
    return n;
}

int cloud_event_write(cloud_event* event, const char* data, size_t size, void* reserved) {
    auto ev = reinterpret_cast<Event*>(event);
    size_t n = CHECK(ev->write(data, size));
    return n;
}

int cloud_event_seek(cloud_event* event, size_t pos, void* reserved) {
    auto ev = reinterpret_cast<Event*>(event);
    pos = CHECK(ev->seek(pos));
    return pos;
}

int cloud_event_tell(cloud_event* event, void* reserved) {
    auto ev = reinterpret_cast<Event*>(event);
    size_t pos = CHECK(ev->tell());
    return pos;
}

int cloud_event_set_size(cloud_event* event, size_t size, void* reserved) {
    auto ev = reinterpret_cast<Event*>(event);
    CHECK(ev->resize(size));
    return 0;
}

int cloud_event_get_size(cloud_event* event, void* reserved) {
    auto ev = reinterpret_cast<Event*>(event);
    size_t n = CHECK(ev->size());
    return n;
}

void cloud_event_set_status_change_callback(cloud_event* event, cloud_event_status_change_callback status_change, cloud_event_destroy_callback destroy, void* arg, void* reserved) {
    // TODO
}

int cloud_event_get_status(cloud_event* event, void* reserved) {
    auto ev = reinterpret_cast<Event*>(event);
    return ev->status();
}

void cloud_event_set_error(cloud_event* event, int error, void* reserved) {
    auto ev = reinterpret_cast<Event*>(event);
    ev->error(error);
}

int cloud_event_get_error(cloud_event* event, void* reserved) {
    auto ev = reinterpret_cast<Event*>(event);
    return ev->error();
}

void cloud_event_clear_error(cloud_event* event, void* reserved) {
    // TODO
}

int cloud_event_publish(cloud_event* event, const cloud_event_publish_options* opts, void* reserved) {
    SYSTEM_THREAD_CONTEXT_SYNC(cloud_event_publish(event, opts, reserved));
    auto ev = reinterpret_cast<Event*>(event);
    CHECK(Cloud::instance()->publish(ev));
    return 0;
}

int cloud_event_subscribe(const char* prefix, cloud_event_subscribe_callback subscribe, cloud_event_destroy_callback destroy, void* arg, const cloud_event_subscribe_options* opts, void* reserved) {
    SYSTEM_THREAD_CONTEXT_SYNC(cloud_event_subscribe(prefix, subscribe, destroy, arg, opts, reserved));
    CHECK(Cloud::instance()->subscribe(prefix, subscribe, arg));
    return 0;
}

void cloud_event_unsubscribe(const char* prefix, void* reserved) {
    // TODO
}
