/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHAN'TABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "feature_flags.h"

bool feature_is_enabled(Feature_Flag flag) {
    uint32_t flags = 0;
    if (dct_read_app_data_copy(DCT_FEATURE_FLAGS_OFFSET, &flags, sizeof(flags))) {
        return false;
    } else {
        // NOTE: inverted logic!
        return (flags & flag) ? false : true;
    }
    return false;
}
