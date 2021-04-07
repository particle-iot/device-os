/**
 ******************************************************************************
 * @file    flash_mal.c
  * @authors Eugene
  * @version V1.0.0
  * @date    23-4-2018
 * @brief   Media access layer for platform dependent flash interfaces
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
//#include <string.h>
#include "hw_config.h"
#include "module_info.h"
#include "module_info_hal.h"
#include "dct.h"
#include "flash_mal.h"
#include "flash_hal.h"
#include "exflash_hal.h"
#include "hal_platform.h"
#include "inflate.h"
#include "bspatch.h"
#include "nrf_mbr.h"

// Decompression of firmware modules is only supported in the bootloader
#if (HAL_PLATFORM_COMPRESSED_OTA) && (MODULE_FUNCTION == MOD_FUNC_BOOTLOADER)
#define HAS_COMPRESSED_OTA 1
#else
#define HAS_COMPRESSED_OTA 0
#endif

#if (HAL_PLATFORM_DELTA_UPDATES) && (MODULE_FUNCTION == MOD_FUNC_BOOTLOADER)
#define HAS_DELTA_UPDATES 1
#else
#define HAS_DELTA_UPDATES 0
#endif

#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
#define SOFTDEVICE_MBR_UPDATES 1
#else
#define SOFTDEVICE_MBR_UPDATES 0
#endif // MODULE_FUNCTION == MOD_FUNC_BOOTLOADER

#define CEIL_DIV(A, B)        (((A) + (B) - 1) / (B))

#define COPY_BLOCK_SIZE 256

static bool flash_read(flash_device_t dev, uintptr_t addr, uint8_t* buf, size_t size) {
    bool ok = false;
    switch (dev) {
    case FLASH_INTERNAL: {
        ok = (hal_flash_read(addr, buf, size) == 0);
        break;
    }
#ifdef USE_SERIAL_FLASH
    case FLASH_SERIAL: {
        ok = (hal_exflash_read(addr, buf, size) == 0);
        break;
    }
#endif // USE_SERIAL_FLASH
    }
    return ok;
}

static bool flash_write(flash_device_t dev, uintptr_t addr, const uint8_t* buf, size_t size) {
    bool ok = false;
    switch (dev) {
    case FLASH_INTERNAL: {
        ok = (hal_flash_write(addr, buf, size) == 0);
        break;
    }
#ifdef USE_SERIAL_FLASH
    case FLASH_SERIAL: {
        ok = (hal_exflash_write(addr, buf, size) == 0);
        break;
    }
#endif // USE_SERIAL_FLASH
    }
    return ok;
}

static bool flash_copy(flash_device_t src_dev, uintptr_t src_addr, flash_device_t dest_dev, uintptr_t dest_addr, size_t size) {
    uint8_t buf[COPY_BLOCK_SIZE];
    const uintptr_t src_end_addr = src_addr + size;
    while (src_addr < src_end_addr) {
        size_t n = src_end_addr - src_addr;
        if (n > sizeof(buf)) {
            n = sizeof(buf);
        }
        if (!flash_read(src_dev, src_addr, buf, n)) {
            return false;
        }
        if (!flash_write(dest_dev, dest_addr, buf, n)) {
            return false;
        }
        src_addr += n;
        dest_addr += n;
    }
    return true;
}

static bool verify_module(flash_device_t src_dev, uintptr_t src_addr, size_t src_size, flash_device_t dest_dev,
        uintptr_t dest_addr, size_t dest_size, uint8_t module_func, uint8_t flags) {
    if (!FLASH_CheckValidAddressRange(src_dev, src_addr, src_size)) {
        return false;
    }
    if (!FLASH_CheckValidAddressRange(dest_dev, dest_addr, dest_size)) {
        return false;
    }
    if ((flags & MODULE_DELTA_UPDATE) && !HAS_DELTA_UPDATES) {
        return false;
    }
    if ((flags & MODULE_COMPRESSED) && !HAS_COMPRESSED_OTA) {
        return false;
    }
    if (flags & MODULE_VERIFY_MASK) {
        uintptr_t module_info_start_addr = 0;
        size_t module_info_size = 0;
        int module_info_platform_id = -1;
        int module_info_func = -1;
        const module_info_t* const info = FLASH_ModuleInfo(src_dev, src_addr, NULL);
        if (info) {
            module_info_start_addr = (uintptr_t)info->module_start_address;
            module_info_size = info->module_end_address - info->module_start_address;
            module_info_platform_id = info->platform_id;
            module_info_func = info->module_function;
        }
        if (module_info_func != MODULE_FUNCTION_RESOURCE && module_info_platform_id != PLATFORM_ID) {
            return false;
        }
        if((flags & (MODULE_VERIFY_LENGTH | MODULE_VERIFY_CRC)) && src_size < module_info_size + 4) {
            return false;
        }
        if ((flags & MODULE_VERIFY_FUNCTION) && module_info_func != module_func) {
            return false;
        }
        if ((flags & MODULE_VERIFY_DESTINATION_IS_START_ADDRESS) && module_info_start_addr != dest_addr) {
#if SOFTDEVICE_MBR_UPDATES
            if (!(module_func == MODULE_FUNCTION_BOOTLOADER && dest_addr == USER_FIRMWARE_IMAGE_LOCATION)) {
                return false;
            }
#else
            return false;
#endif // SOFTDEVICE_MBR_UPDATES
        }
        if ((flags & MODULE_VERIFY_CRC) && !FLASH_VerifyCRC32(src_dev, src_addr, module_info_size)) {
            return false;
        }
    }
    return true;
}

#if HAS_COMPRESSED_OTA

typedef struct inflate_output_ctx {
    uint8_t buf[COPY_BLOCK_SIZE];
    size_t buf_offs;
    uintptr_t flash_addr;
    uintptr_t flash_end_addr;
    flash_device_t flash_dev;
} inflate_output_ctx;

static int inflate_output_callback(const char* data, size_t size, void* user_data) {
    inflate_output_ctx* out = (inflate_output_ctx*)user_data;
    const size_t avail = sizeof(out->buf) - out->buf_offs;
    if (size > avail) {
        size = avail;
    }
    memcpy(out->buf + out->buf_offs, data, size);
    out->buf_offs += size;
    if (out->buf_offs == sizeof(out->buf)) {
        // Flush the output buffer
        if (out->flash_addr + sizeof(out->buf) > out->flash_end_addr) {
            return -1;
        }
        if (!flash_write(out->flash_dev, out->flash_addr, out->buf, sizeof(out->buf))) {
            return -1;
        }
        out->flash_addr += sizeof(out->buf);
        out->buf_offs = 0;
    }
    return size;
}

static bool flash_decompress(flash_device_t src_dev, uintptr_t src_addr, size_t src_size, flash_device_t dest_dev,
        uintptr_t dest_addr, size_t dest_size) {
    uint8_t in_buf[COPY_BLOCK_SIZE];
    inflate_output_ctx out = {};
    out.buf_offs = 0;
    out.flash_dev = dest_dev;
    out.flash_addr = dest_addr;
    out.flash_end_addr = dest_addr + dest_size;
    inflate_ctx* infl = NULL;
    int r = inflate_create(&infl, NULL, inflate_output_callback, &out);
    if (r != 0) {
        goto error;
    }
    uintptr_t src_end_addr = src_addr + src_size;
    while (src_addr < src_end_addr) {
        size_t in_bytes = src_end_addr - src_addr;
        if (in_bytes > sizeof(in_buf)) {
            in_bytes = sizeof(in_buf);
        }
        if (!flash_read(src_dev, src_addr, in_buf, in_bytes)) {
            goto error;
        }
        size_t in_buf_offs = 0;
        do {
            size_t n = in_bytes - in_buf_offs;
            r = inflate_input(infl, (const char*)in_buf + in_buf_offs, &n, INFLATE_HAS_MORE_INPUT);
            if (r < 0) {
                goto error;
            }
            if (r == INFLATE_DONE) {
                goto done;
            }
            in_buf_offs += n;
        } while (in_buf_offs < in_bytes || r == INFLATE_HAS_MORE_OUTPUT);
        src_addr += in_bytes;
    }
    if (r != INFLATE_DONE) {
        goto error;
    }
done:
    if (out.buf_offs > 0) {
        // Flush the output buffer
        if (!flash_write(dest_dev, out.flash_addr, out.buf, out.buf_offs)) {
            goto error;
        }
        out.flash_addr += out.buf_offs;
    }
    if (out.flash_addr != out.flash_end_addr) {
        goto error;
    }
    inflate_destroy(infl);
    return true;
error:
    inflate_destroy(infl);
    return false;
}

static bool parse_compressed_module_header(flash_device_t dev, uintptr_t addr, size_t size, compressed_module_header* header) {
    if (size < sizeof(module_info_t) + sizeof(compressed_module_header)) {
        return false;
    }
    if (!flash_read(dev, addr + sizeof(module_info_t), (uint8_t*)header, sizeof(compressed_module_header))) {
        return false;
    }
    if (header->size > sizeof(compressed_module_header) && size < sizeof(module_info_t) + header->size) {
        return false;
    }
    if (header->method != 0) { // Raw Deflate
        return false;
    }
    return true;
}

#endif // HAS_COMPRESSED_OTA

#if HAS_DELTA_UPDATES

// Context structure for the callbacks used by inflate_create() and bspatch()
typedef struct {
    uint8_t decomp_buf[COPY_BLOCK_SIZE]; // Buffer for decompressed patch data
    inflate_ctx* inflate;
    flash_device_t src_dev;
    flash_device_t dest_dev;
    flash_device_t patch_dev;
    uintptr_t src_addr;
    uintptr_t src_start_addr;
    uintptr_t src_end_addr;
    uintptr_t dest_addr;
    uintptr_t dest_end_addr;
    uintptr_t patch_addr;
    uintptr_t patch_end_addr;
    size_t decomp_read_offs;
    size_t decomp_write_offs;
} flash_patch_ctx;

static int flash_patch_src_seek(ssize_t offs, void* user_data) {
    flash_patch_ctx* const ctx = (flash_patch_ctx*)user_data;
    const intptr_t src_addr = (intptr_t)ctx->src_addr + offs;
    if (src_addr < (intptr_t)ctx->src_start_addr || src_addr > (intptr_t)ctx->src_end_addr) {
        return -1;
    }
    ctx->src_addr = src_addr;
    return 0;
}

static int flash_patch_src_read(char* data, size_t size, void* user_data) {
    flash_patch_ctx* const ctx = (flash_patch_ctx*)user_data;
    if (ctx->src_addr + size > ctx->src_end_addr) {
        return -1;
    }
    const bool ok = flash_read(ctx->src_dev, ctx->src_addr, data, size);
    if (!ok) {
        return -1;
    }
    ctx->src_addr += size;
    return size;
}

static int flash_patch_dest_write(const char* data, size_t size, void* user_data) {
    flash_patch_ctx* const ctx = (flash_patch_ctx*)user_data;
    if (ctx->dest_addr + size > ctx->dest_end_addr) {
        return -1;
    }
    const bool ok = flash_write(ctx->dest_dev, ctx->dest_addr, data, size);
    if (!ok) {
        return -1;
    }
    ctx->dest_addr += size;
    return size;
}

static int flash_patch_read(char* data, size_t size, void* user_data) {
    flash_patch_ctx* const ctx = (flash_patch_ctx*)user_data;
    char comp_buf[COPY_BLOCK_SIZE]; // Buffer for compressed patch data
    size_t comp_read_offs = 0;
    size_t comp_write_offs = 0;
    size_t bytes_to_read = size;
    while (bytes_to_read > 0) {
        if (ctx->decomp_read_offs < ctx->decomp_write_offs) {
            // Read decompressed patch data
            size_t n = ctx->decomp_write_offs - ctx->decomp_read_offs;
            if (n > bytes_to_read) {
                n = bytes_to_read;
            }
            memcpy(data, ctx->decomp_buf + ctx->decomp_read_offs, n);
            bytes_to_read -= n;
            ctx->decomp_read_offs += n;
            if (ctx->decomp_read_offs == ctx->decomp_write_offs) {
                ctx->decomp_read_offs = 0;
                ctx->decomp_write_offs = 0;
            }
        } else if (ctx->patch_addr < ctx->patch_end_addr && comp_write_offs < sizeof(comp_buf)) {
            // Read compressed patch data
            size_t n = ctx->patch_end_addr - ctx->patch_addr;
            if (n > sizeof(comp_buf) - comp_write_offs) {
                n = sizeof(comp_buf) - comp_write_offs;
            }
            const bool ok = flash_read(ctx->patch_dev, ctx->patch_addr, comp_buf + comp_write_offs, n);
            if (!ok) {
                return -1;
            }
            comp_write_offs += n;
            ctx->patch_addr += n;
        } else if (comp_read_offs < comp_write_offs) {
            // Decompress patch data
            size_t n = comp_write_offs - comp_read_offs;
            unsigned flags = 0;
            if (ctx->patch_addr < ctx->patch_end_addr) {
                flags |= INFLATE_HAS_MORE_INPUT;
            }
            const int r = inflate_input(ctx->inflate, comp_buf + comp_read_offs, &n, flags);
            if (r < 0) {
                return -1;
            }
            comp_read_offs += n;
            if (comp_read_offs > comp_write_offs) { // Sanity check
                return -1;
            }
            if (comp_read_offs == comp_write_offs) {
                comp_read_offs = 0;
                comp_write_offs = 0;
            }
        }
    }
    return size;
}

static int flash_patch_inflate_output(const char* data, size_t size, void* user_data) {
    flash_patch_ctx* const ctx = (flash_patch_ctx*)user_data;
    size_t n = sizeof(ctx->decomp_buf) - ctx->decomp_write_offs;
    if (n > size) {
        n = size;
    }
    memcpy(ctx->decomp_buf + ctx->decomp_write_offs, data, n);
    ctx->decomp_write_offs += n;
    return n;
}

static bool flash_patch(flash_device_t src_dev, uintptr_t src_addr, size_t src_size, flash_device_t dest_dev,
        uintptr_t dest_addr, size_t dest_size, flash_device_t patch_dev, uintptr_t patch_addr, size_t patch_size) {
    flash_patch_ctx ctx = {};
    ctx.src_dev = src_dev;
    ctx.src_addr = src_addr;
    ctx.src_start_addr = src_addr;
    ctx.src_end_addr = src_addr + src_size;
    ctx.dest_dev = dest_dev;
    ctx.dest_addr = dest_addr;
    ctx.dest_end_addr = dest_addr + dest_size;
    ctx.patch_dev = patch_dev;
    ctx.patch_addr = patch_addr;
    ctx.patch_end_addr = patch_addr + patch_size;
    ctx.decomp_read_offs = 0;
    ctx.decomp_write_offs = 0;
    int r = inflate_create(&ctx.inflate, NULL /* opts */, flash_patch_inflate_output, &ctx);
    if (r < 0) {
        goto error;
    }
    r = bspatch(flash_patch_read, flash_patch_src_read, flash_patch_src_seek, flash_patch_dest_write, dest_size, &ctx);
    if (r < 0) {
        goto error;
    }
    inflate_destroy(ctx.inflate);
    return true;
error:
    inflate_destroy(ctx.inflate);
    return false;
}

static bool parse_delta_update_module_header(flash_device_t dev, uintptr_t addr, size_t size,
        delta_update_module_header* header) {
    if (size < sizeof(module_info_t) + sizeof(delta_update_module_header)) {
        return false;
    }
    if (!flash_read(dev, addr + sizeof(module_info_t), (uint8_t*)header, sizeof(delta_update_module_header))) {
        return false;
    }
    if (header->size > sizeof(delta_update_module_header) && size < sizeof(module_info_t) + header->size) {
        return false;
    }
    if (header->format != 0) { // ENDSLEY/BSDIFF43 compressed with raw Deflate
        return false;
    }
    return true;
}

#endif // HAS_DELTA_UPDATES

bool FLASH_CheckValidAddressRange(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length)
{
    // FIXME: remove magic numbers
    uint32_t endAddress = startAddress + length - 1;

    if (flashDeviceID == FLASH_INTERNAL)
    {
        return (startAddress >= 0x00000000 && endAddress <= 0x100000) ||
               (startAddress >= 0x12000000 && endAddress <= 0x20000000);
    }
    else if (flashDeviceID == FLASH_SERIAL)
    {
#ifdef USE_SERIAL_FLASH
        return startAddress >= 0x00000000 && endAddress <= EXTERNAL_FLASH_SIZE;
#else
        return false;
#endif
    }
    else
    {
        return false;   //Invalid FLASH ID
    }

    return true;
}

bool FLASH_EraseMemory(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length)
{
    uint32_t numSectors;

    if (FLASH_CheckValidAddressRange(flashDeviceID, startAddress, length) != true)
    {
        return false;
    }

    if (flashDeviceID == FLASH_INTERNAL)
    {
        numSectors = CEIL_DIV(length, INTERNAL_FLASH_PAGE_SIZE);

        if (hal_flash_erase_sector(startAddress, numSectors) != 0)
        {
            return false;
        }

        return true;
    }
#ifdef USE_SERIAL_FLASH
    else if (flashDeviceID == FLASH_SERIAL)
    {
        numSectors = CEIL_DIV(length, sFLASH_PAGESIZE);

        if (hal_exflash_erase_sector(startAddress, numSectors) != 0)
        {
            return false;
        }

        return true;
    }
#endif

    return false;
}

int FLASH_CheckCopyMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                      flash_device_t destinationDeviceID, uint32_t destinationAddress,
                      uint32_t length, uint8_t module_function, uint8_t flags)
{
    size_t dest_size = length;
    if (flags & MODULE_DELTA_UPDATE) {
#if HAS_DELTA_UPDATES
        delta_update_module_header header = { 0 };
        if (!parse_delta_update_module_header(sourceDeviceID, sourceAddress, length, &header)) {
            return FLASH_ACCESS_RESULT_ERROR;
        }
        dest_size = header.dest_size;
#else
        return FLASH_ACCESS_RESULT_BADARG;
#endif // !HAS_DELTA_UPDATES
    } else if (flags & MODULE_COMPRESSED) {
#if HAS_COMPRESSED_OTA
        compressed_module_header header = { 0 };
        if (!parse_compressed_module_header(sourceDeviceID, sourceAddress, length, &header)) {
            return FLASH_ACCESS_RESULT_ERROR;
        }
        dest_size = header.original_size;
#else
        return FLASH_ACCESS_RESULT_BADARG;
#endif // !HAS_COMPRESSED_OTA
    }
    if (!verify_module(sourceDeviceID, sourceAddress, length, destinationDeviceID, destinationAddress, dest_size,
            module_function, flags)) {
        return FLASH_ACCESS_RESULT_BADARG;
    }
    return FLASH_ACCESS_RESULT_OK;
}

int FLASH_CopyMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                     flash_device_t destinationDeviceID, uint32_t destinationAddress,
                     uint32_t length, uint8_t module_function, uint8_t flags)
{
    size_t dest_size = length;
    // TODO: It's time to split this function into a few functions that would separately handle a
    // regular update, compressed update, delta update or bootloader update
#if HAS_DELTA_UPDATES
    delta_update_module_header delta_header = { 0 };
    if (flags & MODULE_DELTA_UPDATE) {
        if (!parse_delta_update_module_header(sourceDeviceID, sourceAddress, length, &delta_header)) {
            return FLASH_ACCESS_RESULT_ERROR;
        }
        dest_size = delta_header.dest_size;
    }
#endif // HAS_DELTA_UPDATES
#if HAS_COMPRESSED_OTA
    compressed_module_header comp_header = { 0 };
    if ((flags & MODULE_COMPRESSED) && !(flags & MODULE_DELTA_UPDATE)) {
        if (!parse_compressed_module_header(sourceDeviceID, sourceAddress, length, &comp_header)) {
            return FLASH_ACCESS_RESULT_ERROR;
        }
        dest_size = comp_header.original_size;
    }
#endif // HAS_COMPRESSED_OTA
    if (!verify_module(sourceDeviceID, sourceAddress, length, destinationDeviceID, destinationAddress, dest_size,
            module_function, flags)) {
        return FLASH_ACCESS_RESULT_BADARG;
    }
#if SOFTDEVICE_MBR_UPDATES
    if (module_function == MODULE_FUNCTION_BOOTLOADER && destinationAddress == USER_FIRMWARE_IMAGE_LOCATION) {
        // Backup user firmware
        dest_size = CEIL_DIV(length, INTERNAL_FLASH_PAGE_SIZE) * INTERNAL_FLASH_PAGE_SIZE;
        if (!FLASH_EraseMemory(FLASH_SERIAL, EXTERNAL_FLASH_RESERVED_ADDRESS, dest_size)) {
            return FLASH_ACCESS_RESULT_ERROR;
        }
        if (!flash_copy(FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION, FLASH_SERIAL, EXTERNAL_FLASH_RESERVED_ADDRESS, dest_size)) {
            return FLASH_ACCESS_RESULT_ERROR;
        }
        // Add backed up partial user firmware to slots, so that it's restored on next boot
        if (!FLASH_AddToNextAvailableModulesSlot(FLASH_SERIAL, EXTERNAL_FLASH_RESERVED_ADDRESS, FLASH_INTERNAL,
                USER_FIRMWARE_IMAGE_LOCATION, dest_size, MODULE_FUNCTION_NONE, 0)) {
            return FLASH_ACCESS_RESULT_ERROR;
        }
    }
#endif // SOFTDEVICE_MBR_UPDATES
    // Do not erase the destination storage now if we're applying a delta update
    if (!(flags & MODULE_DELTA_UPDATE) && !FLASH_EraseMemory(destinationDeviceID, destinationAddress, dest_size)) {
        return FLASH_ACCESS_RESULT_ERROR;
    }
    if (flags & MODULE_DELTA_UPDATE) {
#if HAS_DELTA_UPDATES
        // In the context of delta updates, the source data is the module data that is already
        // flashed on the device
        const flash_device_t src_dev = destinationDeviceID;
        const uintptr_t src_addr = destinationAddress;
        const size_t src_size = delta_header.src_size;
        // Patch data is what is contained in the downloaded module that we need to apply
        const flash_device_t patch_dev = sourceDeviceID;
        const uintptr_t patch_addr = sourceAddress + sizeof(module_info_t) + delta_header.size;
        size_t patch_size = length;
        // Skip the module info and delta update headers
        if (patch_size < sizeof(module_info_t) + delta_header.size + 2 /* Suffix size */ + 4 /* CRC-32 */) { // Sanity check
            return FLASH_ACCESS_RESULT_BADARG;
        }
        patch_size -= sizeof(module_info_t) + delta_header.size + 4;
        // Determine where the patch data ends
        uint16_t suffix_size = 0;
        if (!flash_read(patch_dev, patch_addr + patch_size - 2, (uint8_t*)&suffix_size, 2)) {
            return FLASH_ACCESS_RESULT_ERROR;
        }
        if (patch_size < suffix_size) {
            return FLASH_ACCESS_RESULT_BADARG;
        }
        patch_size -= suffix_size;
        // Use the reserved area in the external flash as an intermediate storage for the patched binary
        const flash_device_t dest_dev = FLASH_SERIAL;
        const uintptr_t dest_addr = EXTERNAL_FLASH_RESERVED_ADDRESS;
        if (!FLASH_EraseMemory(dest_dev, dest_addr, dest_size)) {
            return FLASH_ACCESS_RESULT_ERROR;
        }
        // Apply the patch
        if (!flash_patch(src_dev, src_addr, src_size, dest_dev, dest_addr, dest_size, patch_dev, patch_addr, patch_size)) {
            return FLASH_ACCESS_RESULT_ERROR;
        }
        // Overwrite the "source" module with the patched module
        if (!FLASH_EraseMemory(src_dev, src_addr, dest_size)) {
            return FLASH_ACCESS_RESULT_ERROR;
        }
        if (!flash_copy(dest_dev, dest_addr, src_dev, src_addr, dest_size)) {
            return FLASH_ACCESS_RESULT_ERROR;
        }
#else
        return FLASH_ACCESS_RESULT_BADARG;
#endif // !HAS_DELTA_UPDATES
    } else if (flags & MODULE_COMPRESSED) {
#if HAS_COMPRESSED_OTA
        // Skip the module info and compressed data headers
        if (length < sizeof(module_info_t) + comp_header.size + 2 /* Suffix size */ + 4 /* CRC-32 */) { // Sanity check
            return FLASH_ACCESS_RESULT_BADARG;
        }
        sourceAddress += sizeof(module_info_t) + comp_header.size;
        length -= sizeof(module_info_t) + comp_header.size + 4;
        // Determine where the compressed data ends
        uint16_t suffix_size = 0;
        if (!flash_read(sourceDeviceID, sourceAddress + length - 2, (uint8_t*)&suffix_size, 2)) {
            return FLASH_ACCESS_RESULT_ERROR;
        }
        if (length < suffix_size) {
            return FLASH_ACCESS_RESULT_BADARG;
        }
        length -= suffix_size;
        if (!flash_decompress(sourceDeviceID, sourceAddress, length, destinationDeviceID, destinationAddress, dest_size)) {
            return FLASH_ACCESS_RESULT_ERROR;
        }
#else
        return FLASH_ACCESS_RESULT_BADARG;
#endif // !HAS_COMPRESSED_OTA
    } else {
        // XXX: Device OS versions < 2.0.0 may not be setting this flag in the module slots!
        // Bootloader should rely on the actual flags within the module header!
        if (!(flags & MODULE_DROP_MODULE_INFO) && (flags & MODULE_VERIFY_MASK)) {
            // We may only check the module header if we've been asked to verify it
            uint32_t infoOffset = 0;
            const module_info_t* info = FLASH_ModuleInfo(sourceDeviceID, sourceAddress, &infoOffset);
            if (info->flags & MODULE_INFO_FLAG_DROP_MODULE_INFO) {
                // NB: We have corner cases where the module info is not located in the
                // front of the module but for example after the vector table and we
                // only want to enable this feature in the case it is in the front,
                // hence the module_info_t located at the the source address check.
                if (infoOffset == 0) {
                    // Skip module header
                    flags |= MODULE_DROP_MODULE_INFO;
                } else {
                    return FLASH_ACCESS_RESULT_ERROR;
                }
            }
        }
        if (flags & MODULE_DROP_MODULE_INFO) {
            // Skip the module info header
            if (length < sizeof(module_info_t)) { // Sanity check
                return FLASH_ACCESS_RESULT_BADARG;
            }
            sourceAddress += sizeof(module_info_t);
            length -= sizeof(module_info_t);
        }
        if (!flash_copy(sourceDeviceID, sourceAddress, destinationDeviceID, destinationAddress, length)) {
            return FLASH_ACCESS_RESULT_ERROR;
        }
    }

#if SOFTDEVICE_MBR_UPDATES
    if (module_function == MODULE_FUNCTION_BOOTLOADER && destinationAddress == USER_FIRMWARE_IMAGE_LOCATION) {
        sd_mbr_command_t command = {
            .command = SD_MBR_COMMAND_COPY_BL,
            .params.copy_bl.bl_src = (uint32_t*)destinationAddress,
            // length is in words!
            .params.copy_bl.bl_len = CEIL_DIV(length, 4)
        };

        uint32_t mbr_result = sd_mbr_command(&command);
        if (!mbr_result) {
            // We are anyway returning reset pending state here
            // in order for the bootloader to immediately reset and restore the
            // application module.
            return FLASH_ACCESS_RESULT_RESET_PENDING;
        }
        // We will not reach this point actually, as MBR will reset automatically
        return FLASH_ACCESS_RESULT_RESET_PENDING;
    }
#endif // SOFTDEVICE_MBR_UPDATES
    return FLASH_ACCESS_RESULT_OK;
}

bool FLASH_CompareMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                         flash_device_t destinationDeviceID, uint32_t destinationAddress,
                         uint32_t length)
{
    uint32_t endAddress = sourceAddress + length;

    // check address range
    if (FLASH_CheckValidAddressRange(sourceDeviceID, sourceAddress, length) != true)
    {
        return false;
    }

    if (FLASH_CheckValidAddressRange(destinationDeviceID, destinationAddress, length) != true)
    {
        return false;
    }

    uint8_t src_buf[COPY_BLOCK_SIZE];
    uint8_t dest_buf[COPY_BLOCK_SIZE];
    uint32_t copy_len = 0;

    /* Program source to destination */
    while (sourceAddress < endAddress)
    {
        copy_len = (endAddress - sourceAddress) >= COPY_BLOCK_SIZE ? COPY_BLOCK_SIZE : (endAddress - sourceAddress);

        // Read data from source memory address
        if (sourceDeviceID == FLASH_SERIAL)
        {
            if (hal_exflash_read(sourceAddress, src_buf, copy_len))
            {
                return false;
            }
        }
        else
        {
            if (hal_flash_read(sourceAddress, src_buf, copy_len))
            {
                return false;
            }
        }

        // Read data from destination memory address
        if (sourceDeviceID == FLASH_SERIAL)
        {
            if (hal_exflash_read(destinationAddress, dest_buf, copy_len))
            {
                return false;
            }
        }
        else
        {
            if (hal_flash_read(destinationAddress, dest_buf, copy_len))
            {
                return false;
            }
        }

        if (memcmp(src_buf, dest_buf, copy_len))
        {
            /* Failed comparison check */
            return false;
        }

        sourceAddress += copy_len;
        destinationAddress += copy_len;
    }

    /* Passed comparison check */
    return true;
}

bool FLASH_AddToNextAvailableModulesSlot(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                                         flash_device_t destinationDeviceID, uint32_t destinationAddress,
                                         uint32_t length, uint8_t function, uint8_t flags)
{
    platform_flash_modules_t flash_modules[MAX_MODULES_SLOT];
    uint8_t flash_module_index = MAX_MODULES_SLOT;

    //Read the flash modules info from the dct area
    dct_read_app_data_copy(DCT_FLASH_MODULES_OFFSET, flash_modules, sizeof(flash_modules));

    //fill up the next available modules slot and return true else false
    //slot 0 is reserved for factory reset module so start from flash_module_index = 1
    for (flash_module_index = GEN_START_SLOT; flash_module_index < MAX_MODULES_SLOT; flash_module_index++)
    {
        if(flash_modules[flash_module_index].magicNumber == 0xABCD)
        {
            continue;
        }
        else
        {
            flash_modules[flash_module_index].sourceDeviceID = sourceDeviceID;
            flash_modules[flash_module_index].sourceAddress = sourceAddress;
            flash_modules[flash_module_index].destinationDeviceID = destinationDeviceID;
            flash_modules[flash_module_index].destinationAddress = destinationAddress;
            flash_modules[flash_module_index].length = length;
            flash_modules[flash_module_index].magicNumber = 0xABCD;
            flash_modules[flash_module_index].module_function = function;
            flash_modules[flash_module_index].flags = flags;

            dct_write_app_data(&flash_modules[flash_module_index],
                               offsetof(application_dct_t, flash_modules[flash_module_index]),
                               sizeof(platform_flash_modules_t));

            return true;
        }
    }

    return false;
}

bool FLASH_AddToFactoryResetModuleSlot(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                                       flash_device_t destinationDeviceID, uint32_t destinationAddress,
                                       uint32_t length, uint8_t function, uint8_t flags)
{
    platform_flash_modules_t flash_module;
    platform_flash_modules_t flash_module_current;

    //Read the flash modules info from the dct area
    dct_read_app_data_copy(DCT_FLASH_MODULES_OFFSET + (FAC_RESET_SLOT * sizeof(platform_flash_modules_t)), &flash_module_current, sizeof(flash_module_current));
    flash_module = flash_module_current;

    flash_module.sourceDeviceID = sourceDeviceID;
    flash_module.sourceAddress = sourceAddress;
    flash_module.destinationDeviceID = destinationDeviceID;
    flash_module.destinationAddress = destinationAddress;
    flash_module.length = length;
    flash_module.magicNumber = 0x0FAC;
    flash_module.module_function = function;
    flash_module.flags = flags;

    if(memcmp(&flash_module, &flash_module_current, sizeof(flash_module)) != 0)
    {
        //Only write dct app data if factory reset module slot is different
        dct_write_app_data(&flash_module, DCT_FLASH_MODULES_OFFSET + (FAC_RESET_SLOT * sizeof(platform_flash_modules_t)), sizeof(platform_flash_modules_t));
    }

    return true;
}

bool FLASH_ClearFactoryResetModuleSlot(void)
{
    // Mark slot as unused
    const size_t magic_num_offs = DCT_FLASH_MODULES_OFFSET + sizeof(platform_flash_modules_t) * FAC_RESET_SLOT + \
            offsetof(platform_flash_modules_t, magicNumber);

    const uint16_t magic_num = 0xffff;
    return (dct_write_app_data(&magic_num, magic_num_offs, sizeof(magic_num)) == 0);
    return 0;
}

int FLASH_ApplyFactoryResetImage(copymem_fn_t copy)
{
    platform_flash_modules_t flash_module;
    int restoreFactoryReset = FLASH_ACCESS_RESULT_ERROR;

    //Read the flash modules info from the dct area
    dct_read_app_data_copy(DCT_FLASH_MODULES_OFFSET + (FAC_RESET_SLOT * sizeof(flash_module)), &flash_module, sizeof(flash_module));

    if(flash_module.magicNumber == 0x0FAC)
    {
        //Restore Factory Reset Firmware (slot 0 is factory reset module)
        restoreFactoryReset = copy(flash_module.sourceDeviceID,
                                   flash_module.sourceAddress,
                                   flash_module.destinationDeviceID,
                                   flash_module.destinationAddress,
                                   flash_module.length,
                                   flash_module.module_function,
                                   flash_module.flags);
    }

    return restoreFactoryReset;
}

bool FLASH_IsFactoryResetAvailable(void)
{
    return !FLASH_ApplyFactoryResetImage(FLASH_CheckCopyMemory);
}

bool FLASH_RestoreFromFactoryResetModuleSlot(void)
{
    return !FLASH_ApplyFactoryResetImage(FLASH_CopyMemory);
}

// This function is called in bootloader to perform the memory update process
int FLASH_UpdateModules(void (*flashModulesCallback)(bool isUpdating))
{
    // FAC_RESET_SLOT is reserved for factory reset module
    const size_t module_count = MAX_MODULES_SLOT - GEN_START_SLOT;
    const size_t module_offs = DCT_FLASH_MODULES_OFFSET + sizeof(platform_flash_modules_t) * GEN_START_SLOT;
    platform_flash_modules_t modules[module_count];
    int r = dct_read_app_data_copy(module_offs, modules, sizeof(platform_flash_modules_t) * module_count);
    if (r != 0) {
        return FLASH_ACCESS_RESULT_ERROR;
    }
    int result = FLASH_ACCESS_RESULT_OK;
    bool has_modules = false;
    bool has_bootloader = false;
    size_t offs = module_offs;
    for (size_t i = 0; i < module_count; ++i) {
        platform_flash_modules_t* const module = &modules[i];
        if (module->magicNumber == 0xabcd) {
            if (!has_modules) {
                has_modules = true;
                // Turn on RGB_COLOR_MAGENTA toggling during flash update
                if (flashModulesCallback) {
                    flashModulesCallback(true);
                }
            }
            if (module->module_function != MODULE_FUNCTION_BOOTLOADER) {
                // Copy memory from source to destination based on flash device ID
                r = FLASH_CopyMemory(module->sourceDeviceID, module->sourceAddress, module->destinationDeviceID,
                        module->destinationAddress, module->length, module->module_function, module->flags);
                if (r != FLASH_ACCESS_RESULT_OK && result != FLASH_ACCESS_RESULT_RESET_PENDING) {
                    // Propagate errors, prioritize FLASH_ACCESS_RESULT_RESET_PENDING over errors
                    result = r;
                }
                // Mark the slot as unused
                module->magicNumber = 0xffff;
                r = dct_write_app_data(&module->magicNumber, offs + offsetof(platform_flash_modules_t, magicNumber),
                        sizeof(module->magicNumber));
                if (r != 0 && result != FLASH_ACCESS_RESULT_RESET_PENDING) {
                    result = FLASH_ACCESS_RESULT_ERROR;
                }
            } else {
                has_bootloader = true;
            }
        }
        offs += sizeof(platform_flash_modules_t);
    }
    // Perform bootloader update last as it will cause an automatic reset
    if (has_bootloader) {
        offs = module_offs;
        for (size_t i = 0; i < module_count; ++i) {
            platform_flash_modules_t* const module = &modules[i];
            if (module->magicNumber == 0xabcd && module->module_function == MODULE_FUNCTION_BOOTLOADER) {
                // Mark the slot as unused BEFORE updating the module
                module->magicNumber = 0xffff;
                r = dct_write_app_data(&module->magicNumber, offs + offsetof(platform_flash_modules_t, magicNumber),
                        sizeof(module->magicNumber));
                if (r != 0 && result != FLASH_ACCESS_RESULT_RESET_PENDING) {
                    result = FLASH_ACCESS_RESULT_ERROR;
                }
                r = FLASH_CopyMemory(module->sourceDeviceID, module->sourceAddress, module->destinationDeviceID,
                        module->destinationAddress, module->length, module->module_function, module->flags);
                if (r != FLASH_ACCESS_RESULT_OK && result != FLASH_ACCESS_RESULT_RESET_PENDING) {
                    result = r;
                }
            }
            offs += sizeof(platform_flash_modules_t);
        }
    }
    // Turn off RGB_COLOR_MAGENTA toggling
    if (has_modules && flashModulesCallback) {
        flashModulesCallback(false);
    }
    return result;
}

const module_info_t* FLASH_ModuleInfo(uint8_t flashDeviceID, uint32_t startAddress, uint32_t* infoOffset)
{
#ifdef USE_SERIAL_FLASH
    static module_info_t ex_module_info; // FIXME: This is not thread-safe
#endif

    const module_info_t* info = NULL;
    uint32_t offset = 0;

    if(flashDeviceID == FLASH_INTERNAL)
    {
        if (((*(__IO uint32_t*)startAddress) & APP_START_MASK) == 0x20000000)
        {
            offset = 0x200;
            startAddress += offset;
        }

        info = (const module_info_t*)startAddress;
    }
#ifdef USE_SERIAL_FLASH
    else if(flashDeviceID == FLASH_SERIAL)
    {
        uint8_t serialFlashData[4];
        uint32_t first_word = 0;

        hal_exflash_read(startAddress, serialFlashData, 4);
        first_word = (uint32_t)(serialFlashData[0] | (serialFlashData[1] << 8) | (serialFlashData[2] << 16) | (serialFlashData[3] << 24));
        if ((first_word & APP_START_MASK) == 0x20000000)
        {
            offset = 0x200;
            startAddress += offset;
        }

        memset((uint8_t *)&ex_module_info, 0x00, sizeof(module_info_t));
        hal_exflash_read(startAddress, (uint8_t *)&ex_module_info, sizeof(module_info_t));

        info = &ex_module_info;
#endif
    }

    if (infoOffset && info)
    {
        *infoOffset = offset;
    }

    return info;
}

uint32_t FLASH_ModuleAddress(uint8_t flashDeviceID, uint32_t startAddress)
{
    const module_info_t* module_info = FLASH_ModuleInfo(flashDeviceID, startAddress, NULL);

    if (module_info != NULL)
    {
        return (uint32_t)module_info->module_start_address;
    }

    return 0;
}

uint32_t FLASH_ModuleLength(uint8_t flashDeviceID, uint32_t startAddress)
{
    const module_info_t* module_info = FLASH_ModuleInfo(flashDeviceID, startAddress, NULL);

    if (module_info != NULL)
    {
        return ((uint32_t)module_info->module_end_address - (uint32_t)module_info->module_start_address);
    }

    return 0;
}

uint16_t FLASH_ModuleVersion(uint8_t flashDeviceID, uint32_t startAddress)
{
    const module_info_t* module_info = FLASH_ModuleInfo(flashDeviceID, startAddress, NULL);

    if (module_info != NULL)
    {
        return module_info->module_version;
    }

    return 0;
}

bool FLASH_isUserModuleInfoValid(uint8_t flashDeviceID, uint32_t startAddress, uint32_t expectedAddress)
{
    const module_info_t* module_info = FLASH_ModuleInfo(flashDeviceID, startAddress, NULL);

    return (module_info != NULL
            && ((uint32_t)(module_info->module_start_address) == expectedAddress)
            && ((uint32_t)(module_info->module_end_address)<=0x100000)
            && (module_info->platform_id==PLATFORM_ID));
}

bool FLASH_VerifyCRC32(uint8_t flashDeviceID, uint32_t startAddress, uint32_t length)
{
    if(flashDeviceID == FLASH_INTERNAL && length > 0)
    {
        uint32_t expectedCRC = __REV((*(__IO uint32_t*) (startAddress + length)));
        uint32_t computedCRC = Compute_CRC32((uint8_t*)startAddress, length, NULL);

        if (expectedCRC == computedCRC)
        {
            return true;
        }
    }
#ifdef USE_SERIAL_FLASH
    else if(flashDeviceID == FLASH_SERIAL && length > 0)
    {
        uint8_t serialFlashData[4]; // FIXME: Use XIP or a larger buffer
        hal_exflash_read((startAddress + length), serialFlashData, 4);
        uint32_t expectedCRC = (uint32_t)(serialFlashData[3] | (serialFlashData[2] << 8) | (serialFlashData[1] << 16) | (serialFlashData[0] << 24));

        uint32_t endAddress = startAddress + length;
        uint32_t len = 0;
        uint32_t computedCRC = 0;

        do {
            len = endAddress - startAddress;
            if (len > sizeof(serialFlashData)) {
                len = sizeof(serialFlashData);
            }
            hal_exflash_read(startAddress, serialFlashData, len);

            computedCRC = Compute_CRC32(serialFlashData, len, &computedCRC);

            startAddress += len;
        } while (startAddress < endAddress);

        if (expectedCRC == computedCRC)
        {
            return true;
        }
#endif
    }

    return false;
}

void FLASH_ClearFlags(void)
{
    return;
}

void FLASH_WriteProtection_Enable(uint32_t FLASH_Sectors)
{
    // TBD
    return;
}

void FLASH_WriteProtection_Disable(uint32_t FLASH_Sectors)
{
    // TBD
    return;
}

void FLASH_Erase(void)
{
    // This is too dangerous.
    //FLASH_EraseMemory(FLASH_INTERNAL, CORE_FW_ADDRESS, FIRMWARE_IMAGE_SIZE);
    return;
}

void FLASH_Backup(uint32_t FLASH_Address)
{
    // TBD
    return;
}

void FLASH_Restore(uint32_t FLASH_Address)
{
#ifdef USE_SERIAL_FLASH
    //CRC verification Disabled by default
    //FLASH_CopyMemory(FLASH_SERIAL, FLASH_Address, FLASH_INTERNAL, CORE_FW_ADDRESS, FIRMWARE_IMAGE_SIZE, 0, 0);
#else
    //commented below since FIRMWARE_IMAGE_SIZE != Actual factory firmware image size
    //FLASH_CopyMemory(FLASH_INTERNAL, FLASH_Address, FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION, FIRMWARE_IMAGE_SIZE, true);
    //FLASH_AddToFactoryResetModuleSlot() is now called in HAL_Core_Config() in core_hal.c
#endif
}

void FLASH_Begin(uint32_t FLASH_Address, uint32_t imageSize)
{
    system_flags.OTA_FLASHED_Status_SysFlag = 0x0000;
    Save_SystemFlags();

#ifdef USE_SERIAL_FLASH
    FLASH_EraseMemory(FLASH_SERIAL, FLASH_Address, imageSize);
#else
    FLASH_EraseMemory(FLASH_INTERNAL, FLASH_Address, imageSize);
#endif
}

int FLASH_Update(const uint8_t *pBuffer, uint32_t address, uint32_t bufferSize)
{
    int ret = -1;
#ifdef USE_SERIAL_FLASH
    ret = hal_exflash_write(address, pBuffer, bufferSize);
#else
    ret = hal_flash_write(address, pBuffer, bufferSize);
#endif
    return ret;
}
