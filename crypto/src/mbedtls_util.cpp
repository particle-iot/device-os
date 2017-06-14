/**
 ******************************************************************************
 Copyright (c) 2013-2017 Particle Industries, Inc.  All rights reserved.

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

#include "mbedtls_util.h"
#include "mbedtls/timing.h"
#include <cstring>

#include "rng_hal.h"
#include "timer_hal.h"

int mbedtls_default_rng(void*, unsigned char* data, size_t size) {
    while (size >= 4) {
        *((uint32_t*)data) = HAL_RNG_GetRandomNumber();
        data += 4;
        size -= 4;
    }
    while (size-- > 0) {
        *data++ = HAL_RNG_GetRandomNumber();
    }
    return 0;
}

#if !defined(CRYPTO_PART1_SIZE_OPTIMIZATIONS) && PLATFORM_ID != 0
__attribute__((weak)) int mbedtls_set_callbacks(mbedtls_callbacks_t* callbacks, void* reserved)
{
    return 0;
}

__attribute__((weak)) mbedtls_callbacks_t* mbedtls_get_callbacks(void* reserved)
{
    return NULL;
}
#endif // defined(CRYPTO_PART1_SIZE_OPTIMIZATIONS) || PLATFORM_ID == 0

unsigned long mbedtls_timing_hardclock()
{
    return HAL_Timer_Microseconds();
}
