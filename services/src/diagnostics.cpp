/*
 * Copyright (c) 2017 Particle Industries, Inc.  All rights reserved.
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

#include "diagnostics.h"

#include "spark_wiring_vector.h"

#include "system_error.h"

#include <algorithm>

namespace {

using namespace spark;

class Diagnostics {
public:
    int registerSource(const diag_source* src) {
        if (started_) {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        const int index = indexForId(src->id);
        if (index < srcs_.size() && srcs_.at(index)->id == src->id) {
            return SYSTEM_ERROR_ALREADY_EXISTS;
        }
        if (!srcs_.insert(index, src)) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        return SYSTEM_ERROR_NONE;
    }

    int enumSources(diag_enum_sources_callback callback, size_t* count, void* data) {
        if (!started_) {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        if (callback) {
            for (const diag_source* src: srcs_) {
                const int ret = callback(src, data);
                if (ret != SYSTEM_ERROR_NONE) {
                    return ret;
                }
            }
        }
        if (count) {
            *count = srcs_.size();
        }
        return SYSTEM_ERROR_NONE;
    }

    int getSource(uint16_t id, const diag_source** src) {
        if (!started_) {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        const int index = indexForId(id);
        if (index >= srcs_.size() || srcs_.at(index)->id != id) {
            return SYSTEM_ERROR_NOT_FOUND;
        }
        if (src) {
            *src = srcs_.at(index);
        }
        return SYSTEM_ERROR_NONE;
    }

    int command(int cmd, void* data) {
        switch (cmd) {
#if PLATFORM_ID == 3
        case DIAG_SERVICE_CMD_RESET:
            srcs_.clear();
            started_ = 0;
            break;
#endif
        case DIAG_SERVICE_CMD_START:
            started_ = 1;
            break;
        default:
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
        return SYSTEM_ERROR_NONE;
    }

    static Diagnostics* instance() {
        static Diagnostics* diag = new Diagnostics();
        return diag;
    }

private:
    Vector<const diag_source*> srcs_;
    volatile uint8_t started_;

    Diagnostics() :
            started_(0) { // The service is stopped initially
        srcs_.reserve(32);
    }

    int indexForId(uint16_t id) const {
        return std::distance(srcs_.begin(), std::lower_bound(srcs_.begin(), srcs_.end(), id,
                [](const diag_source* src, uint16_t id) {
                    return (src->id < id);
                }));
    }
};

} // namespace

int diag_register_source(const diag_source* src, void* reserved) {
    return Diagnostics::instance()->registerSource(src);
}

int diag_enum_sources(diag_enum_sources_callback callback, size_t* count, void* data, void* reserved) {
    return Diagnostics::instance()->enumSources(callback, count, data);
}

int diag_get_source(uint16_t id, const diag_source** src, void* reserved) {
    return Diagnostics::instance()->getSource(id, src);
}

int diag_command(int cmd, void* data, void* reserved) {
    return Diagnostics::instance()->command(cmd, data);
}
