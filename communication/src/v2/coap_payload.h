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
 * The initial portion of the data is stored in RAM. The data above the certain size is stored in a
 * temporary file.
 */
class CoapPayload: public RefCount {
public:
    CoapPayload() :
            pos_(0),
            size_(0),
            fileNum_(0) {
    }

    ~CoapPayload();

    int read(char* data, size_t size);
    int read(char* data, size_t size, size_t pos);

    int write(const char* data, size_t size);
    int write(const char* data, size_t size, size_t pos);

    int setSize(size_t size);

    size_t size() const {
        return size_;
    }

    int setPos(int pos, coap_whence whence);

    size_t pos() const {
        return pos_;
    }

private:
    Buffer buf_; // Portion of the payload data stored in RAM
    std::unique_ptr<lfs_file_t> file_; // Handle of the temporary file with the rest of the payload data
    size_t pos_; // Current position in the payload data
    size_t size_; // Total size of the payload data
    unsigned fileNum_; // Sequence number of the temporary file

    int createTempFile(lfs_t* fs);
    void removeTempFile(lfs_t* fs);
};

} // namespace particle::protocol::v2
