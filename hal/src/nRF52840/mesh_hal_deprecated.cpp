/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#include "mesh_hal_deprecated.h"

#include "radio_common.h"

#include "check.h"

using namespace particle;

int __attribute__((deprecated("Will be removed in 5.x!")))
mesh_select_antenna_deprecated(int antenna, void* reserved) {
    CHECK(selectRadioAntenna((radio_antenna_type)antenna));
    return 0;
}
