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

#include "filesystem.h"

#include "ref_count.h"

namespace particle::protocol::experimental {

class CoapPayload: public RefCount {
public:
    CoapPayload() :
            pos_(0),
            size_(0) {
    }

    ~CoapPayload() {
        removeTempFile();
    }

    int read(char* data, size_t size);
    int write(const char* data, size_t size);

    int size(size_t size);

    int size() const {
        return size_;
    }

    int pos(size_t pos);

    int pos() const {
        return pos_;
    }

    static int initTempDir();

private:
    Buffer buf_; // Portion of the payload data stored in RAM
    std::unique_ptr<lfs_file_t> file_; // Temporary file with the rest of the payload data
    size_t pos_; // Current position in the payload data
    size_t size_; // Total size of the payload data
    unsigned fileNum_; // Sequence number of the temporary file created

    int createTempFile();
    void removeTempFile();
};

} // namespace particle::protocol::experimental
