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

#ifndef PART1_PREINIT_DYNALIB_H
#define	PART1_PREINIT_DYNALIB_H


#include "dynalib.h"

DYNALIB_BEGIN(part1_preinit)
DYNALIB_FN(0, part1_preinit, bootloader_part1_preinit, int(void))
DYNALIB_END(part1_preinit)


#endif	/* PART1_PREINIT_DYNALIB_H */

