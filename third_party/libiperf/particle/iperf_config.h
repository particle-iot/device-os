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

#include "hal_platform.h"

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

// shutdown(SHUT_WR) in iperf_sync_close_socket() causes a TCP_PCB double-free
// on lwIP without LWIP_NETCONN_FULLDUPLEX. The shutdown corrupts the netconn
// state, and the subsequent close() frees a PCB that's already in a bad state.
// Disable so iperf_sync_close_socket() falls back to a plain close().
#define HAVE_SOCKET_SHUTDOWN_SHUT_WR 0

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

#define IPERF_LWIP 1

// On a RAM-constrained device (e.g. Gen 3) a UDP blast can momentarily exhaust
// the heap, at which point lwIP's select() returns -1 with ENOMEM/ENOBUFS
// because it can't allocate its internal signalling. That is transient - the
// data path frees pbufs as it drains - so the server retries select() a bounded
// number of consecutive times (yielding briefly to let memory free up) instead
// of tearing the control socket down ("control socket has closed unexpectedly").
#define IPERF_SELECT_MAX_TRANSIENT_FAILS 32
#define IPERF_SELECT_RETRY_DELAY_MS 20

// When a UDP send fails because the heap is exhausted (ENOMEM/ENOBUFS) the
// sender must back off hard rather than spin: a tight retry immediately re-pins
// the heap with fresh in-flight pbufs and never lets the network stack drain
// what's already queued, collapsing the whole test. Backing off lets the stack
// free the in-flight working set so the next send succeeds - i.e. memory
// pressure self-throttles the sender to a sustainable rate. This is much longer
// than the plain EWOULDBLOCK yield, which only needs to unblock the main thread.
#define IPERF_SEND_MEMPRESSURE_BACKOFF_MS 20

// Cap the peer-requested block size (params "len") on RAM-constrained
// platforms. Standard iperf3 clients default to a 128 KB block for TCP and
// iperf_new_stream() mallocs exactly blksize, which can never succeed on a
// ~20 KB heap - every host-initiated TCP test would die with IECREATESTREAM.
// TCP is a byte stream, so smaller chunks are semantically identical; UDP
// block sizes are MTU-bounded and unaffected. Sizing: the stream buffer AND the
// worker thread stack (IPERF_WORKER_THREAD_STACK_SIZE, 4 KB) are allocated
// back-to-back from a fragmented heap whose largest free block is ~12 KB at
// that point; an 8 KB buffer left <4 KB contiguous and thread creation failed
// (IEPTHREADCREATE). 4 KB leaves room for both, and at the achievable rates
// (~1.5 Mbps) a 4 KB block is still only ~45 send() calls per second.
#if HAL_PLATFORM_NRF52840
#define IPERF_MAX_BLKSIZE (4 * 1024)
#endif

// Totals-only JSON on RAM-constrained platforms. json_output is still fully
// produced (json_start + json_end totals, which the host-side tests read), but
// per-interval cJSON objects are discarded rather than accumulated. Without this
// the ~1.5 KB/s build-up exhausts the ~22 KB heap in ~10 s. Note even a small
// bounded window (tried IPERF_JSON_MAX_INTERVALS=4) is too costly: rendering the
// results with cJSON_PrintUnformatted() needs one contiguous buffer ~= the whole
// output, which fails at a few KB free and kills the results exchange. Gen 4
// (rtl872x) has ample RAM and keeps the full interval history (macro undefined).
#if HAL_PLATFORM_NRF52840
#define IPERF_JSON_NO_INTERVALS 1
#endif

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

// iperf worker (pthread) stack size. Gen 3 (nRF52840) has tight RAM, so trim
// aggressively: the worker only runs the send/recv loop (no cJSON - that runs
// on the server/control thread), so 3 KB suffices and leaves room for the
// stream buffer (IPERF_MAX_BLKSIZE) in the same fragmented largest free block.
// Other platforms keep the larger default. The StackOverflow PANIC hook guards
// against undersizing.
#if HAL_PLATFORM_NRF52840
#define IPERF_WORKER_THREAD_STACK_SIZE (3 * 1024)
#else
#define IPERF_WORKER_THREAD_STACK_SIZE (6 * 1024)
#endif

#ifndef IPERF_VERSION
#error "IPERF_VERSION is not defined, check build.mk"
#endif
#define PACKAGE_STRING "iperf " IPERF_VERSION
