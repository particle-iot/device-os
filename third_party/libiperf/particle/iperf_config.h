/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

#define HAVE_INTTYPES_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_STRINGS_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_UNISTD_H 1
#define HAVE_STDATOMIC_H 1
#define STDC_HEADERS 1

#define HAVE_SYS_ENDIAN_H 1

#define HAVE_SYS_SOCKET_H 1
#define HAVE_POLL_H 1
#define HAVE_SOCKET_SHUTDOWN_SHUT_WR 1

#define HAVE_SO_BINDTODEVICE 1
#define CAN_BIND_TO_DEVICE 1

#define HAVE_PTHREAD 1

// Implemented in particle/nanosleep.c, newlib does not declare it
#define HAVE_NANOSLEEP 1
struct timespec;
int nanosleep(const struct timespec* req, struct timespec* rem);

// Implemented in particle/getentropy.c, newlib does not declare it
#define HAVE_GETENTROPY 1
#include <stddef.h>
int getentropy(void* buffer, size_t length);

// Implemented in particle/clock_gettime.c, newlib does not declare it.
// CLOCK_MONOTONIC id matches newlib's (POSIX-gated) time.h definition.
#define HAVE_CLOCK_GETTIME 1
#include <sys/types.h>
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC ((clockid_t) 4)
#endif
int clock_gettime(clockid_t clock_id, struct timespec* tp);

#define IPERF_OUTPUT_CALLBACK_ONLY 1

// exit() is fatal on the device, longjmp() to the recovery point registered
// with iperf_set_test_exit_jmp_buf() instead
#define IPERF_NO_EXIT 1

// Interrupting a blocked worker recv()/send() by calling shutdown() on the
// stream socket from the control thread is only safe when lwIP allows
// concurrent access to a netconn from multiple threads (LWIP_NETCONN_FULLDUPLEX).
// Otherwise it corrupts the netconn and wedges/crashes the server. Enable only
// when LWIP_NETCONN_FULLDUPLEX is enabled.
#ifndef IPERF_CANCEL_SHUTDOWN
#define IPERF_CANCEL_SHUTDOWN 0
#endif
#if IPERF_CANCEL_SHUTDOWN
#define IPERF_STREAM_CANCEL_SHUTDOWN(sp) shutdown((sp)->socket, SHUT_RDWR)
#else
#define IPERF_STREAM_CANCEL_SHUTDOWN(sp) ((void)0)
#endif

#ifndef IPERF_VERSION
#error "IPERF_VERSION is not defined, check build.mk"
#endif
#define PACKAGE_STRING "iperf " IPERF_VERSION
