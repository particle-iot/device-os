/*
 * Copyright (c) 2017 Particle Industries, Inc.  All rights reserved.
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

#include "system_error.h"

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// System data sources
typedef enum diag_source_id {
    DIAG_SOURCE_ID_INVALID = 0 // Invalid source ID
} diag_source_id;

// Data types
typedef enum diag_source_type {
    DIAG_SOURCE_TYPE_SCALAR = 1,
    DIAG_SOURCE_TYPE_COUNTER = 2,
    DIAG_SOURCE_TYPE_ENUM = 3,
    DIAG_SOURCE_TYPE_STRING = 4
} diag_source_type;

typedef struct diag_source diag_source;

typedef int(*diag_get_callback)(const diag_source* src, char* data, size_t* size);
typedef void(*diag_reset_callback)(const diag_source* src);
typedef void(*diag_enum_callback)(const diag_source* src);

typedef struct diag_source {
    size_t size; // Size of this structure
    uint16_t id; // Source ID
    uint16_t type; // Data type
    void* data; // User data
    diag_get_callback get;
    diag_reset_callback reset;
} diag_source;

int diag_register_source(const diag_source* src, void* reserved);
void diag_enum_sources(diag_enum_callback callback, size_t* count, void* data, void* reserved);
int diag_get_source(uint16_t id, const diag_source** src, void* reserved);
void diag_reset(void* reserved);

#ifdef __cplusplus
} // extern "C"
#endif
