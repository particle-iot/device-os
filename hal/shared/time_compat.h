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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <sys/types.h>
#include <sys/time.h>
#include <stdint.h>
#ifndef __cplusplus
#include <assert.h>
#endif // __cplusplus
#include <stddef.h>

#ifdef __NEWLIB__
#include <sys/config.h>
#endif // __NEWLIB__

// Newlib-specific
#ifdef __NEWLIB__
#if __NEWLIB__ >= 3
#ifdef _USE_LONG_TIME_T
#if __LONG_MAX__ > 0x7fffffffL
#define LIBC_64_BIT_TIME_T
#endif // __LONG_MAX__ > 0x7fffffffL
#else
// Newlib has switched to 64-bit time_t by default
#define LIBC_64_BIT_TIME_T
#endif // _USE_LONG_TIME_T
#endif // __NEWLIB__ >= 3

#ifndef LIBC_64_BIT_TIME_T
#error "Unsupported newlib version with 32-bit time_t"
#endif // LIBC_64_BIT_TIME_T

#endif // __NEWLIB__

#ifndef __NEWLIB__
// On all the other platforms assume that 'long' is used
#if __LONG_MAX__ > 0x7fffffffL
#define LIBC_64_BIT_TIME_T
#endif // __LONG_MAX__ > 0x7fffffffL
#endif // __NEWLIB__

#ifdef LIBC_64_BIT_TIME_T
// time_t is 64-bit
typedef time_t time64_t;
typedef int32_t time32_t;

struct timeval32 {
    time32_t tv_sec;
    int32_t tv_usec;
};

#define LIBC_TIMEVAL32 struct timeval32
#define LIBC_TIMEVAL64 struct timeval

#else
// time_t is 32-bit
typedef time_t time32_t;
typedef int64_t time64_t;

struct timeval64 {
    time64_t tv_sec;
    int64_t tv_usec;
};

#define LIBC_TIMEVAL32 struct timeval
#define LIBC_TIMEVAL64 struct timeval64

#endif // LIBC_64_BIT_TIME_T

#ifdef LIBC_64_BIT_TIME_T
static_assert(sizeof(LIBC_TIMEVAL64) == sizeof(struct timeval), "sizeof compat timeval64 does not match libc timeval");
static_assert(offsetof(LIBC_TIMEVAL64, tv_usec) == offsetof(struct timeval, tv_usec), "offsetof tv_usec int timeval64 does not match libc timeval tv_usec");
static_assert(sizeof(time64_t) == sizeof(time_t), "sizeof time64_t does not match time_t");
static_assert(sizeof(struct timeval) == sizeof(time_t) * 2, "sizeof libc timeval does not match expected");

static_assert(sizeof(time32_t) == sizeof(int32_t), "sizeof time32_t does not match 32-bit time_t");
static_assert(sizeof(LIBC_TIMEVAL32) == sizeof(time32_t) * 2, "sizeof compat timeval32 does not match libc timeval with 32-bit time_t");
static_assert(offsetof(LIBC_TIMEVAL32, tv_usec) == sizeof(time32_t), "sizeof compat timeval32 does not match libc timeval with 32-bit time_t");
#else
static_assert(sizeof(LIBC_TIMEVAL32) == sizeof(struct timeval), "sizeof compat timeval32 does not match libc timeval");
static_assert(offsetof(LIBC_TIMEVAL32, tv_usec) == offsetof(struct timeval, tv_usec), "offsetof tv_usec in timeval32 does not match libc timeval tv_usec");
static_assert(sizeof(time32_t) == sizeof(time_t), "sizeof time32_t does not match time_t");
static_assert(sizeof(struct timeval) == sizeof(time_t) * 2, "sizeof libc timeval does not match expected");

static_assert(sizeof(time64_t) == sizeof(int64_t), "sizeof time64_t does not match 64-bit time_t");
static_assert(sizeof(LIBC_TIMEVAL64) == sizeof(time64_t) * 2, "sizeof compat timeval32 does not match libc timeval with 32-bit time_t");
static_assert(offsetof(LIBC_TIMEVAL64, tv_usec) == sizeof(time64_t), "sizeof compat timeval64 does not match libc timeval with 32-bit time_t");
#endif // LIBC_64_BIT_TIME_T

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct tm* localtime32_r(const time32_t* timep, struct tm* result);
time32_t mktime32(struct tm* tm);

#ifdef __cplusplus
}
#endif // __cplusplus
