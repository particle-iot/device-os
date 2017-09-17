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

#include "debug.h"

int particle::DiagnosticData::get(uint16_t id, void* data, size_t& size) {
    const diag_source* src = nullptr;
    const int ret = diag_get_source(id, &src, nullptr);
    if (ret != SYSTEM_ERROR_NONE) {
        return ret;
    }
    return get(src, data, size);
}

int particle::DiagnosticData::get(const diag_source* src, void* data, size_t& size) {
    SPARK_ASSERT(src && src->callback);
    diag_source_get_cmd_data d = { sizeof(diag_source_get_cmd_data), data, size };
    const int ret = src->callback(src, DIAG_CMD_GET, &d);
    if (ret == SYSTEM_ERROR_NONE) {
        size = d.data_size;
    }
    return ret;
}

int particle::DiagnosticData::callback(const diag_source* src, int cmd, void* data) {
    const auto d = static_cast<DiagnosticData*>(src->data);
    switch (cmd) {
    case DIAG_CMD_GET: {
        const auto cmdData = static_cast<diag_source_get_cmd_data*>(data);
        return d->get(cmdData->data, cmdData->data_size);
    }
    default:
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
}

int particle::IntegerDiagnosticData::get(uint16_t id, int32_t& val) {
    const diag_source* src = nullptr;
    const int ret = diag_get_source(id, &src, nullptr);
    if (ret != SYSTEM_ERROR_NONE) {
        return ret;
    }
    return get(src, val);
}

int particle::IntegerDiagnosticData::get(const diag_source* src, int32_t& val) {
    SPARK_ASSERT(src->type == DIAG_TYPE_INT);
    size_t size = sizeof(val);
    return DiagnosticData::get(src, &val, size);
}

int particle::IntegerDiagnosticData::get(void* data, size_t& size) {
    if (!data) {
        size = sizeof(int32_t);
        return SYSTEM_ERROR_NONE;
    }
    if (size < sizeof(int32_t)) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    const int ret = get(*(int32_t*)data);
    if (ret == SYSTEM_ERROR_NONE) {
        size = sizeof(int32_t);
    }
    return ret;
}
