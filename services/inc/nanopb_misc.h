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

#ifndef SERVICES_NANOPB_MISC_H
#define SERVICES_NANOPB_MISC_H

#include "pb.h"
#include "pb_encode.h"
#include "pb_decode.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

pb_ostream_t* pb_ostream_init(void* reserved);
bool pb_ostream_free(pb_ostream_t* stream, void* reserved);

pb_istream_t* pb_istream_init(void* reserved);
bool pb_istream_free(pb_istream_t* stream, void* reserved);

bool pb_ostream_from_buffer_ex(pb_ostream_t* stream, pb_byte_t *buf, size_t bufsize, void* reserved);
bool pb_istream_from_buffer_ex(pb_istream_t* stream, const pb_byte_t *buf, size_t bufsize, void* reserved);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* SERVICES_NANOPB_MISC_H */
