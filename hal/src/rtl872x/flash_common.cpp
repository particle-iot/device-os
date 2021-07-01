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

#include "flash_common.h"
#include <cstring>
#include <algorithm>

// Do not define Particle's STATIC_ASSERT() to avoid conflicts with the nRF SDK's own macro
#define NO_STATIC_ASSERT
#include "module_info.h"

#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

#include "static_recursive_mutex.h"

/* Because non-trivial designated initializers are not supported in GCC,
 * we can't have exflash_hal.c as exflash_hal.cpp and thus can't use
 * StaticRecursiveMutex in it.
 *
 * Initialize mutex here and provide helper functions for locking
 */
static StaticRecursiveMutex s_exflash_mutex;

int hal_exflash_lock(void) {
    return !s_exflash_mutex.lock();
}

int hal_exflash_unlock(void) {
    return !s_exflash_mutex.unlock();
}

#else

__attribute__((weak)) int hal_exflash_lock(void) {
    return 0;
}

__attribute__((weak)) int hal_exflash_unlock(void) {
    return 0;
}
#endif /* MODULE_FUNCTION != MOD_FUNC_BOOTLOADER */

int hal_flash_common_dummy_read(uintptr_t addr, uint8_t* buf, size_t size) {
    /* This function is used for unaligned writes to retrive the data from the flash
     * to keep unaligned unaffected bytes unmodified.
     * Due to how flash memory works, we can simply return 0xff, as the bits in the flash memory
     * can only be flipped from 1 to 0.
     */
    memset(buf, 0xff, size);

    return 0;
}

static int write_unaligned_word(uintptr_t addr, const uint8_t* data, size_t size,
                                hal_flash_common_write_cb write_func, hal_flash_common_read_cb read_func)
{
    if (size > 0) {
        uint32_t tmp __attribute__((aligned(4)));
        const uintptr_t addr_aligned = ADDR_ALIGN_WORD(addr);
        /* Read in the data from the flash to keep the unaffected unaligned bytes intact */
        if (read_func(addr_aligned, (uint8_t*)&tmp, sizeof(tmp))) {
            return -1;
        }

        const size_t unaligned_bytes = (addr - addr_aligned);
        const size_t bytes_to_copy = std::min(sizeof(uint32_t) - unaligned_bytes, size);

        memcpy((uint8_t*)&tmp + unaligned_bytes, data, bytes_to_copy);

        /* Write */
        if (write_func(addr_aligned, (const uint8_t*)&tmp, sizeof(uint32_t))) {
            return -1;
        }

        return bytes_to_copy;
    }

    return 0;
}

int hal_flash_common_write(uintptr_t addr, const uint8_t* data_buf, size_t data_size,
                           hal_flash_common_write_cb write_func, hal_flash_common_read_cb read_func)
{

    /* Fist we need to align the destination address */
    if (!IS_WORD_ALIGNED(addr)) {
        const int bytes_copied = write_unaligned_word(addr, data_buf, data_size, write_func, read_func);
        if (bytes_copied < 0) {
            return -1;
        }

        /* Adjust destination, source and size */
        addr += bytes_copied;
        data_buf += bytes_copied;
        data_size -= bytes_copied;
    }

    if (data_size == 0) {
        /* We're already done */
        return 0;
    }

    /* destination address should already be aligned due to the previous step */
    if (IS_WORD_ALIGNED(addr) && IS_WORD_ALIGNED((uintptr_t)data_buf)) {
        /* Fast-path: both addresses are aligned */
        const size_t bytes_to_copy = ADDR_ALIGN_WORD(data_size);
        if (bytes_to_copy > 0) {
            if (write_func(addr, data_buf, bytes_to_copy)) {
                return -1;
            }

            /* Adjust destination, source and size */
            addr += bytes_to_copy;
            data_buf += bytes_to_copy;
            data_size -= bytes_to_copy;
        }
    } else {
        /* Slow-path: use a temporary buffer */
        /* TODO: Use a shared 4k (both internal and external flash block size) static global buffer? */
        uint8_t temp_buf[FLASH_OPERATION_TEMP_BLOCK_SIZE] __attribute__((aligned(4)));

        const size_t total_bytes_to_copy = ADDR_ALIGN_WORD(data_size);

        for (size_t bytes_left = total_bytes_to_copy; bytes_left > 0;) {
            const size_t bytes_to_copy = std::min(bytes_left, (size_t)FLASH_OPERATION_TEMP_BLOCK_SIZE);

            /* Copy data into the temporary buffer */
            memcpy(temp_buf, data_buf, bytes_to_copy);

            if (write_func(addr, temp_buf, bytes_to_copy)) {
                return -1;
            }

            bytes_left -= bytes_to_copy;

            /* Adjust destination and source */
            addr += bytes_to_copy;
            data_buf += bytes_to_copy;
        }

        data_size -= total_bytes_to_copy;
    }

    /* Write the remainder */
    return write_unaligned_word(addr, data_buf, data_size, write_func, read_func) < 0 ? -1 : 0;
}
