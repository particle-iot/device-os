/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#include <stdint.h>
#include "basic_types.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct rtl_binary_header {
    uint32_t signature_high;
    uint32_t signature_low;
    uint32_t size;
    uint32_t load_address;
    uint32_t sboot_address;
    uint32_t reserved0;
    uint64_t reserved1;
} rtl_binary_header;

typedef struct rtl_secure_boot_footer {
    uint32_t reserved[12];
    uint8_t sb_sig[64];
} rtl_secure_boot_footer;

static const uint32_t RTL_HEADER_SIGNATURE_HIGH = 0x96969999;
static const uint32_t RTL_HEADER_SIGNATURE_LOW = 0xfc66cc3f;
// XXX:
static const uint32_t RTL_HEADER_SIGNATURE_UNK_HIGH = 0x35393138;
static const uint32_t RTL_HEADER_SIGNATURE_UNK_LOW = 0x31313738;

#define RTL_HEADER_RAM_VALID_PATTERN {0x23, 0x79, 0x16, 0x88, 0xff, 0xff, 0xff, 0xff}

#ifdef __cplusplus
}
#endif // __cplusplus