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

/**
 * @file
 * @brief
 *  This is a POSIX wrapper for inet_hal_posix
 */

#ifndef ARPA_INET_H
#define ARPA_INET_H

#include "inet_hal.h"

#define inet_addr(cp) inet_inet_addr(cp)
#define inet_aton(cp, pin) inet_inet_aton(cp, pin)
#define inet_network(cp) inet_inet_network(cp)
#define inet_ntoa(in) inet_inet_ntoa(in)
#define inet_ntoa_r(in, buf, size) inet_inet_ntoa_r(in, buf, size)
#define inet_ntop(af, src, dst, size) inet_inet_ntop(af, src, dst, size)
#define inet_pton(af, src, dst) inet_inet_pton(af, src, dst)

#endif /* ARPA_INET_H */
