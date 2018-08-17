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

#ifndef HAL_LWIP_LOGGING_H
#define HAL_LWIP_LOGGING_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void lwip_log_message(const char *fmt, ...);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef DEBUG_BUILD
#define LWIP_PLATFORM_DIAG(x) do { lwip_log_message x; } while(0)
#else
#define LWIP_PLATFORM_DIAG(x)
#endif /* DEBUG_BUILD */

#endif /* HAL_LWIP_LOGGING_H */
