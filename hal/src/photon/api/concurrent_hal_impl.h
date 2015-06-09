/**
 ******************************************************************************
 * @file    concurrency_hal_impl.h
 * @authors mat
 * @date    03 March 2015
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#ifndef CONCURRENCY_HAL_IMPL_H
#define	CONCURRENCY_HAL_IMPL_H

typedef int32_t os_result_t;
typedef uint8_t os_thread_prio_t;

const os_thread_prio_t OS_THREAD_PRIORITY_DEFAULT = 0;

const size_t OS_THREAD_STACK_SIZE_DEFAULT = 512;

#endif	/* CONCURRENCY_HAL_IMPL_H */

