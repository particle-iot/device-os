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

#pragma once

#include "spark_descriptor.h"

namespace particle {

namespace protocol {

namespace test {

// Helper class for mocking the descriptor callbacks (SparkDescriptor)
class DescriptorCallbacks {
public:
    DescriptorCallbacks();
    virtual ~DescriptorCallbacks();

    const SparkDescriptor& get() const;

    virtual bool appendSystemInfo(appender_fn append, void* arg, void* reserved);
    virtual bool appendAppInfo(appender_fn append, void* arg, void* reserved);
    virtual bool appendMetrics(appender_fn append, void* arg, uint32_t flags, uint32_t page, void* reserved);

private:
    SparkDescriptor desc_;
};

inline const SparkDescriptor& DescriptorCallbacks::get() const {
    return desc_;
}

inline bool DescriptorCallbacks::appendSystemInfo(appender_fn append, void* arg, void* reserved) {
    return false;
}

inline bool DescriptorCallbacks::appendAppInfo(appender_fn append, void* arg, void* reserved) {
    return false;
}

inline bool DescriptorCallbacks::appendMetrics(appender_fn append, void* arg, uint32_t flags, uint32_t page, void* reserved) {
    return false;
}

} // namespace test

} // namespace protocol

} // namespace particle
