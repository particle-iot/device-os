/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
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

#include "devicetree_stubs.h"
#include "system_error.h"

int devicetree_init(const void*, uint32_t, void*) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int devicetree_overlay_apply(const void*, uint32_t, void*) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int devicetree_string_dictionary_register(const void*, size_t, uint32_t, void*) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int devicetree_tree_lock(void*) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int devicetree_tree_get(void*, uint32_t, void*) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

const char* devicetree_string_dictionary_lookup(uint32_t, void*) {
    return nullptr;
}

uint32_t devicetree_hash_string(const char*, size_t) {
    return 0;
}
