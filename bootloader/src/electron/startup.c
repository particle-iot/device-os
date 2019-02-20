/*
 ******************************************************************************
 *  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.
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
 ******************************************************************************
 */

#include "dct_hal.h"
/* Define caddr_t as char* */
#include <sys/types.h>
#include <errno.h>
#include <malloc.h>
/* Define abort() */
#include <stdlib.h>

void platform_startup()
{
    dcd_migrate_data();
}

caddr_t _sbrk(int incr)
{
   return 0;
}

// Naive memXXX functions

void* memcpy(void *dest, const void *src, size_t n) {
    const uint8_t* p = src;
    uint8_t* q = dest;
    while (n--) {
        *q++ = *p++;
    }
    return dest;
}

int memcmp(const void* s1, const void* s2, size_t n)
{
  const volatile uint8_t* p1 = (const volatile uint8_t*)s1;
  const volatile uint8_t* p2 = (const volatile uint8_t*)s2;

  while (n--)
  {
    if (*p1++ != *p2++)
        return *p1 - *p2;
  }
  return 0;
}

__attribute__((used)) void* memset(void* s, int c, size_t n) {
    uint8_t* p = s;
    uint8_t v = c & 0xff;
    while (n--) {
        *p++ = v;
    }
    return s;
}
