/*
 * Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.
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

#ifndef HAL_EVENT_H
#define HAL_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "core_hal.h"

// Generates HAL event. See HAL_Event enum for the list of defined events
void hal_notify_event(int event, int flags, void* data);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // HAL_EVENT_H
