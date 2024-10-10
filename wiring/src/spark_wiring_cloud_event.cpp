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

#include <cstring>

#include "spark_wiring_cloud_event.h"
#include "spark_wiring_variant.h"
#include "spark_wiring_logging.h"

#include "check.h"

namespace particle {

namespace {

/*
void statusChangeCallback(cloud_event* event, void* arg) {
}
*/

} // namespace

CloudEvent& CloudEvent::contentType(ContentType type) {
    if (ev_) {
        cloud_event_properties prop = {};
        prop.version = CLOUD_EVENT_API_VERSION;
        prop.flags = CLOUD_EVENT_PROPERTY_CONTENT_TYPE;
        prop.content_type = static_cast<int>(type);
        cloud_event_set_properties(ev_, &prop, nullptr /* reserved */);
    }
    return *this;
}

ContentType CloudEvent::contentType() const {
    if (!ev_) {
        return ContentType::TEXT;
    }
    cloud_event_properties prop = {};
    prop.version = CLOUD_EVENT_API_VERSION;
    prop.flags = CLOUD_EVENT_PROPERTY_CONTENT_TYPE;
    int r = cloud_event_get_properties(ev_, &prop, nullptr /* reserved */);
    if (r < 0) {
        LOG(ERROR, "cloud_event_get_properties() failed: %d", r);
        return ContentType::TEXT;
    }
    return static_cast<ContentType>(prop.content_type);
}

CloudEvent& CloudEvent::data(const char* data, size_t size) {
    if (ev_) {
        pos(0);
        write((const uint8_t*)data, size);
        this->size(size);
    }
    return *this;
}

CloudEvent& CloudEvent::data(const Variant& data) {
    if (ev_) {
        pos(0);
        int r = encodeToCBOR(data, *this);
        if (r < 0) {
            cloud_event_set_error(ev_, r, nullptr /* reserved */);
        } else {
            this->size(pos());
        }
    }
    return *this;
}

Buffer CloudEvent::data() const {
    if (!ev_) {
        return Buffer();
    }
    Buffer buf;
    if (!buf.resize(size())) {
        cloud_event_set_error(ev_, Error::NO_MEMORY, nullptr /* reserved */);
        return Buffer();
    }
    auto origPos = pos();
    pos(0);
    int r = cloud_event_read(ev_, buf.data(), buf.size(), nullptr /* reserved */);
    if (r < 0) {
        return Buffer();
    }
    pos(origPos);
    return buf;
}

Variant CloudEvent::dataAsVariant() {
    if (!ev_) {
        return Variant();
    }
    auto origPos = pos();
    pos(0);
    Variant v;
    int r = decodeFromCBOR(v, *this);
    if (r < 0) {
        cloud_event_set_error(ev_, r, nullptr /* reserved */);
        return Variant();
    }
    pos(origPos);
    return v;
}

CloudEvent& CloudEvent::onStatusChange(OnStatusChange callback, void* arg) {
    // TODO
    return *this;
}

CloudEvent& CloudEvent::onStatusChange(OnStatusChangeFn callback) {
    // TODO
    return *this;
}

int CloudEvent::read() {
    if (!ev_) {
        return -1;
    }
    char c;
    int r = cloud_event_read(ev_, &c, 1, nullptr /* reserved */);
    if (r < 0) {
        return -1;
    }
    return (unsigned char)c;
}

size_t CloudEvent::readBytes(char* data, size_t size) {
    if (!ev_) {
        return 0;
    }
    int r = cloud_event_read(ev_, data, size, nullptr /* reserved */);
    if (r < 0) {
        return 0;
    }
    return r;
}

int CloudEvent::peek() {
    if (!ev_) {
        return -1;
    }
    char c;
    int r = cloud_event_peek(ev_, &c, 1, nullptr /* reserved */);
    if (r < 0) {
        return -1;
    }
    return (unsigned char)c;
}

size_t CloudEvent::write(const uint8_t* data, size_t size) {
    if (!ev_) {
        return 0;
    }
    int r = cloud_event_write(ev_, (const char*)data, size, nullptr /* reserved */);
    if (r < 0) {
        return 0;
    }
    return r;
}

size_t CloudEvent::pos() const {
    if (!ev_) {
        return 0;
    }
    int pos = cloud_event_tell(ev_, nullptr /* reserved */);
    if (pos < 0) {
        return 0;
    }
    return pos;
}

} // namespace particle
