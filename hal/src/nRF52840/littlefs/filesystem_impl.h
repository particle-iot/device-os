/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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

#include "platform_config.h"

/* FIXME */
#define FILESYSTEM_PROG_SIZE    (256)
#define FILESYSTEM_READ_SIZE    (256)

#define FILESYSTEM_BLOCK_SIZE   (sFLASH_PAGESIZE)

/* XXX: Using half of the external flash for now */
#define FILESYSTEM_BLOCK_COUNT  (sFLASH_PAGECOUNT / 2)
#define FILESYSTEM_FIRST_BLOCK  (0)
#define FILESYSTEM_LOOKAHEAD    (128)
