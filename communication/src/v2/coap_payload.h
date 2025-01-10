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

#include <memory>

#include "spark_wiring_buffer.h"

#include "coap_api.h"

#include "filesystem.h"

#include "ref_count.h"

namespace particle::protocol::v2 {

/**
 * Payload data of a CoAP message.
 *
 * The initial portion of the data is stored on the heap. The data above the configured size (`maxHeapSize`)
 * is stored in a temporary file.
 */
class CoapPayload: public RefCount {
public:
    explicit CoapPayload(size_t maxHeapSize = COAP_BLOCK_SIZE);
    ~CoapPayload();

    int read(char* data, size_t size, size_t pos);
    int write(const char* data, size_t size, size_t pos);

    int setSize(size_t size);

    size_t size() const {
        return dataSize_;
    }

private:
    Buffer buf_; // Portion of the payload data stored in RAM
    std::unique_ptr<lfs_file_t> file_; // Handle of the temporary file with the rest of the payload data
    size_t dataSize_; // Total size of the payload data
    const size_t maxHeapSize_; // Maximum amount of payload data that can be stored on the heap
    unsigned fileNum_; // Sequence number of the temporary file

    int createTempFile(lfs_t* fs);
    void removeTempFile(lfs_t* fs);
};

} // namespace particle::protocol::v2
