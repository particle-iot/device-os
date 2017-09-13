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

#include "diagnostics.h"

int diag_register_source(const diag_source* src, void* reserved) {
    return -1; // TODO
}

void diag_enum_sources(diag_enum_callback callback, size_t* count, void* data, void* reserved) {
    // TODO
}

int diag_get_source(uint16_t id, const diag_source** src, void* reserved) {
    return -1; // TODO
}

void diag_reset(void* reserved) {
    // TODO
}
