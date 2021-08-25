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

#include "jsmn_compat.h"
#include <stdint.h>

jsmnerr_t jsmn_parse_deprecated(jsmn_parser *parser, const char *js, size_t len,
        jsmntok_t *tokens, unsigned int num_tokens, void* reserved) {
    int r = jsmn_parse(parser, js, len, tokens, num_tokens, NULL);
    if (reserved) {
        int* ret = (int*)reserved;
        *ret = r;
    }
    if (r > INT8_MAX) {
        r = INT8_MAX;
    }
    return (jsmnerr_t)r;
}
