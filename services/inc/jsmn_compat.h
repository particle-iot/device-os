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

#include "jsmn.h"
#include <assert.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Due to usage of -fshort-enums we were exporting jsmn_parse() which in fact was only
// returning an int8_t, preventing parsing of relatively large JSONs. This is now fixed
// in the upstream version of jsmn library and also in our "fork" contained within Device OS
// sources in services.
// For compatibility purposes, we are still exporting such a variant of jsmn_parse().
jsmnerr_t jsmn_parse_deprecated(jsmn_parser *parser, const char *js, size_t len,
        jsmntok_t *tokens, unsigned int num_tokens, void* reserved);

#if defined(DYNALIB_EXPORT) && defined(MODULAR_FIRMWARE) && MODULAR_FIRMWARE
static_assert(sizeof(jsmnerr_t) == sizeof(int8_t), "size of jsmnerr_t used for compat jsmn_parse changed");
#endif // defined(DYNALIB_EXPORT) && defined(MODULAR_FIRMWARE) && MODULAR_FIRMWARE

#ifdef __cplusplus
}
#endif // __cplusplus
