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

#include "spark_wiring_diagnostics.h"

int particle::AbstractDiagnosticData::callback(const diag_source* src, int cmd, void* data) {
    const auto d = static_cast<AbstractDiagnosticData*>(src->data);
    switch (cmd) {
    case DIAG_SOURCE_CMD_GET: {
        const auto cmdData = static_cast<diag_source_get_cmd_data*>(data);
        return d->get(cmdData->data, cmdData->size);
    }
    default:
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
}

int particle::AbstractIntegerDiagnosticData::get(char* data, size_t& size) {
    if (!data) {
        size = sizeof(int32_t);
        return SYSTEM_ERROR_NONE;
    }
    if (size < sizeof(int32_t)) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    const int ret = get(*(int32_t*)data);
    if (ret != SYSTEM_ERROR_NONE) {
        return ret;
    }
    size = sizeof(int32_t);
    return SYSTEM_ERROR_NONE;
}
