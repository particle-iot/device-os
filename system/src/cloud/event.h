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

#include <optional>

#include "spark_wiring_buffer.h"

#include "system_cloud_event.h"

#include "coap_defs.h"

#include "system_error.h"
#include "ref_count.h"

namespace particle::system::cloud {

using particle::protocol::CoapContentFormat;

class Cloud;

const size_t MAX_EVENT_DATA_SIZE_IN_RAM = 1024;

class Event: public RefCount {
public:
    Event() :
            name_(),
            status_(CLOUD_EVENT_STATUS_NEW),
            pos_(0),
            error_(0) {
    }

    int name(const char* name);

    const char* name() const {
        return name_;
    }

    void contentFormat(CoapContentFormat fmt) {
        contentFmt_ = fmt;
    }

    CoapContentFormat contentFormat() const {
        return contentFmt_.value_or(CoapContentFormat::TEXT_PLAIN);
    }

    bool hasContentFormat() const {
        return contentFmt_.has_value();
    }

    cloud_event_status status() const {
        return status_;
    }

    int error(int code) {
        if (status_ != CLOUD_EVENT_STATUS_FAILED) {
            status_ = CLOUD_EVENT_STATUS_FAILED;
            error_ = code;
        }
        return code;
    }

    int error() const {
        return error_;
    }

    int read(char* data, size_t size);
    int peek(char* data, size_t size);
    int write(const char* data, size_t size);

    int seek(size_t pos);
    int tell() const;

    int resize(size_t size);
    int size() const;

protected:
    int prepareForPublish(); // Called by Cloud
    void publishComplete(int error); // ditto

    const char* data() const { // FIXME
        return data_.data();
    }

private:
    char name_[CLOUD_EVENT_MAX_NAME_LENGTH + 1];
    Buffer data_;
    std::optional<CoapContentFormat> contentFmt_;
    cloud_event_status status_;
    size_t pos_;
    int error_;

    int checkStatus(cloud_event_status expectedStatus) const {
        if (status_ != expectedStatus) {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        return 0;
    }

    friend class Cloud;
};

} // namespace particle::system::cloud
