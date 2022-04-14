/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#include "descriptor_callbacks.h"

#include "protocol_callbacks.h"

namespace particle {

namespace protocol {

namespace test {

namespace {

DescriptorCallbacks* g_callbacks = nullptr;

bool appendSystemInfoCallback(appender_fn append, void* arg, void* reserved) {
    if (g_callbacks) {
        return g_callbacks->appendSystemInfo(append, arg, reserved);
    }
    return false;
}

bool appendAppInfoCallback(appender_fn append, void* arg, void* reserved) {
    if (g_callbacks) {
        return g_callbacks->appendAppInfo(append, arg, reserved);
    }
    return false;
}

bool appendMetricsCallback(appender_fn append, void* arg, uint32_t flags, uint32_t page, void* reserved) {
    if (g_callbacks) {
        return g_callbacks->appendMetrics(append, arg, flags, page, reserved);
    }
    return false;
}

} // namespace

DescriptorCallbacks::DescriptorCallbacks() :
        desc_() {
    desc_.size = sizeof(desc_);
    desc_.append_system_info = appendSystemInfoCallback;
    desc_.append_app_info = appendAppInfoCallback;
    desc_.append_metrics = appendMetricsCallback;
    g_callbacks = this;
}

DescriptorCallbacks::~DescriptorCallbacks() {
    g_callbacks = nullptr;
}

} // namespace test

} // namespace protocol

} // namespace particle
