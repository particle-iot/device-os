/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#include "nanopb_misc.h"

#include <stdlib.h>

#if HAL_PLATFORM_FILESYSTEM
#include "filesystem.h"

static bool write_file_callback(pb_ostream_t* strm, const uint8_t* data, size_t size) {
    filesystem_t* const fs = filesystem_get_instance(NULL);
    if (!fs) {
        return false;
    }
    const lfs_ssize_t n = lfs_file_write(&fs->instance, strm->state, data, size);
    if (n != (lfs_ssize_t)size) {
        return false;
    }
    return true;
}

static bool read_file_callback(pb_istream_t* strm, uint8_t* data, size_t size) {
    filesystem_t* const fs = filesystem_get_instance(NULL);
    if (!fs) {
        return false;
    }
    const lfs_ssize_t n = lfs_file_read(&fs->instance, strm->state, data, size);
    if (n != (lfs_ssize_t)size) {
        return false;
    }
    return true;
}

#endif // HAL_PLATFORM_FILESYSTEM

pb_ostream_t* pb_ostream_init(void* reserved) {
    return (pb_ostream_t*)calloc(sizeof(pb_ostream_t), 1);
}

bool pb_ostream_free(pb_ostream_t* stream, void* reserved) {
    if (stream != NULL) {
        free(stream);
        return true;
    }

    return false;
}

pb_istream_t* pb_istream_init(void* reserved) {
    return (pb_istream_t*)calloc(sizeof(pb_istream_t), 1);
}

bool pb_istream_free(pb_istream_t* stream, void* reserved) {
    if (stream != NULL) {
        free(stream);
        return true;
    }

    return false;
}

bool pb_ostream_from_buffer_ex(pb_ostream_t* stream, pb_byte_t *buf, size_t bufsize, void* reserved) {
    if (stream != NULL) {
        pb_ostream_t tmp = pb_ostream_from_buffer(buf, bufsize);
        memcpy(stream, &tmp, sizeof(pb_ostream_t));
        return true;
    }

    return false;
}

bool pb_istream_from_buffer_ex(pb_istream_t* stream, const pb_byte_t *buf, size_t bufsize, void* reserved) {
    if (stream != NULL) {
        pb_istream_t tmp = pb_istream_from_buffer(buf, bufsize);
        memcpy(stream, &tmp, sizeof(pb_istream_t));
        return true;
    }

    return false;
}

#if HAL_PLATFORM_FILESYSTEM

bool pb_ostream_from_file(pb_ostream_t* stream, lfs_file_t* file, void* reserved) {
    if (!stream || !file) {
        return false;
    }
    filesystem_t* const fs = filesystem_get_instance(NULL);
    if (!fs) {
        return false;
    }
    memset(stream, 0, sizeof(pb_ostream_t));
    stream->callback = write_file_callback;
    stream->state = file;
    stream->max_size = SIZE_MAX;
    return true;
}

bool pb_istream_from_file(pb_istream_t* stream, lfs_file_t* file, void* reserved) {
    if (!stream || !file) {
        return false;
    }
    filesystem_t* const fs = filesystem_get_instance(NULL);
    if (!fs) {
        return false;
    }
    const lfs_soff_t pos = lfs_file_tell(&fs->instance, file);
    const lfs_soff_t size = lfs_file_size(&fs->instance, file);
    if (pos < 0 || size < 0) {
        return false;
    }
    memset(stream, 0, sizeof(pb_istream_t));
    stream->callback = read_file_callback;
    stream->state = file;
    stream->bytes_left = size - pos;
    return true;
}

#endif // HAL_PLATFORM_FILESYSTEM
