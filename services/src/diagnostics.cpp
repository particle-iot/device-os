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

class DiagnosticManager {
public:
    DiagnosticManager() :
            inited_(false) {
        srcs_.reserve(32);
    }

    int registerSource(const diag_source* src) {
        if (inited_) {
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
        if (!inited_) {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        if (callback) {
            for (const diag_source* src: srcs_) {
                callback(src, data);
            }
        }
        if (count) {
            *count = srcs_.size();
        }
        return SYSTEM_ERROR_NONE;
    }

    int getSource(uint16_t id, const diag_source** src) {
        if (!inited_) {
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

    int init() {
        if (inited_) {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        inited_ = true;
        return SYSTEM_ERROR_NONE;
    }

private:
    Vector<const diag_source*> srcs_;
    bool inited_;

    int indexForId(uint16_t id) const {
        return std::distance(srcs_.begin(), std::lower_bound(srcs_.begin(), srcs_.end(), id,
                [](const diag_source* src, uint16_t id) {
                    return (src->id < id);
                }));
    }
};

DiagnosticManager g_diagManager;

} // namespace

int diag_register_source(const diag_source* src, void* reserved) {
    return g_diagManager.registerSource(src);
}

int diag_enum_sources(diag_enum_sources_callback callback, size_t* count, void* data, void* reserved) {
    return g_diagManager.enumSources(callback, count, data);
}

int diag_get_source(uint16_t id, const diag_source** src, void* reserved) {
    return g_diagManager.getSource(id, src);
}

int diag_init(void* reserved) {
    return g_diagManager.init();
}
