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

// FIXME: We should perhaps introduce a separate module for utilities like this
#include "../../communication/inc/coap_api.h"
#include "system_error.h"

#include <stdlib.h>
#include <limits.h>

#if HAL_PLATFORM_FILESYSTEM
#include "filesystem.h"

static bool write_file_callback(pb_ostream_t* strm, const uint8_t* data, size_t size) {
    filesystem_t* const fs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, NULL);
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
    filesystem_t* const fs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, NULL);
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

static bool read_coap_message_callback(pb_istream_t* strm, uint8_t* data, size_t size) {
    size_t n = size;
    int r = coap_read_block((coap_message*)strm->state, data, &n, NULL /* block_cb */, NULL /* error_cb */,
            NULL /* arg */, NULL /* reserved */);
    if (r != 0 || n != size) { // COAP_RESULT_WAIT_BLOCK is treated as an error
        return false;
    }
    return true;
}

static bool write_coap_message_callback(pb_ostream_t* strm, const uint8_t* data, size_t size) {
    size_t n = size;
    int r = coap_write_block((coap_message*)strm->state, data, &n, NULL /* block_cb */, NULL /* error_cb */,
            NULL /* arg */, NULL /* reserved */);
    if (r != 0 || n != size) { // COAP_RESULT_WAIT_BLOCK is treated as an error
        return false;
    }
    return true;
}

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

int pb_ostream_from_file(pb_ostream_t* stream, lfs_file_t* file, void* reserved) {
    if (!stream || !file) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    filesystem_t* const fs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, NULL);
    if (!fs) {
        return SYSTEM_ERROR_FILESYSTEM;
    }
    memset(stream, 0, sizeof(pb_ostream_t));
    stream->callback = write_file_callback;
    stream->state = file;
    stream->max_size = SIZE_MAX;
    return 0;
}

int pb_istream_from_file(pb_istream_t* stream, lfs_file_t* file, int size, void* reserved) {
    if (!stream || !file) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (size < 0) {
        filesystem_t* const fs = filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, NULL);
        if (!fs) {
            return SYSTEM_ERROR_FILESYSTEM;
        }
        lfs_soff_t pos = lfs_file_tell(&fs->instance, file);
        if (pos < 0) {
            return filesystem_to_system_error(pos);
        }
        lfs_soff_t sz = lfs_file_size(&fs->instance, file);
        if (sz < 0) {
            return filesystem_to_system_error(sz);
        }
        size = sz - pos;
    }
    memset(stream, 0, sizeof(pb_istream_t));
    stream->callback = read_file_callback;
    stream->state = file;
    stream->bytes_left = size;
    return 0;
}

#endif // HAL_PLATFORM_FILESYSTEM

int pb_istream_from_coap_message(pb_istream_t* stream, coap_message* msg, void* reserved) {
    int size = coap_peek_block(msg, NULL /* data */, SIZE_MAX, NULL /* reserved */);
    if (size < 0) {
        return size;
    }
    memset(stream, 0, sizeof(*stream));
    stream->callback = read_coap_message_callback;
    stream->state = msg;
    stream->bytes_left = size;
    return 0;
}

int pb_ostream_from_coap_message(pb_ostream_t* stream, coap_message* msg, void* reserved) {
    memset(stream, 0, sizeof(*stream));
    stream->callback = write_coap_message_callback;
    stream->state = msg;
    stream->max_size = COAP_BLOCK_SIZE;
    return 0;
}
