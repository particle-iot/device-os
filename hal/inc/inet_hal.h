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

#ifndef INET_HAL_H
#define INET_HAL_H

#include "hal_platform.h"

#if defined(HAL_USE_INET_HAL_COMPAT) && HAL_USE_INET_HAL_COMPAT == 1
#include "inet_hal_compat.h"
#endif /* defined(HAL_USE_INET_HAL_COMPAT) && HAL_USE_INET_HAL_COMPAT == 1 */

#if defined(HAL_USE_INET_HAL_POSIX) && HAL_USE_INET_HAL_POSIX == 1
#include "inet_hal_posix.h"
#endif /* defined(HAL_USE_INET_HAL_POSIX) && HAL_USE_INET_HAL_POSIX == 1 */

#endif /* INET_HAL_H */
