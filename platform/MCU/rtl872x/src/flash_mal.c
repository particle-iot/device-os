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
#include "check.h"
#include "rtl8721d.h"
#include "rtl_header.h"
#include "km0_km4_ipc.h"
#include "interrupts_hal.h"
#include "ota_flash_hal.h"
#include <stdlib.h>

// Decompression of firmware modules is only supported in the bootloader
#if (HAL_PLATFORM_COMPRESSED_OTA) && (MODULE_FUNCTION == MOD_FUNC_BOOTLOADER)
#define HAS_COMPRESSED_OTA 1
#else
#define HAS_COMPRESSED_OTA 0
#endif

#define CEIL_DIV(A, B)        (((A) + (B) - 1) / (B))

#define COPY_BLOCK_SIZE 256

#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
__attribute__((section(".boot.ipc_data"), aligned(32))) platform_flash_modules_aligned sModule;
#endif

static int hal_memory_read(uintptr_t addr, uint8_t* data_buf, size_t data_size) {
    memcpy(data_buf, (void*)addr, data_size);
    return 0;
}

static int hal_memory_write(uintptr_t addr, const uint8_t* data_buf, size_t data_size) {
    memcpy((void*)addr, data_buf, data_size);
    return 0;
}

#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
#define FLASH_MAL_OTA_TEMPORARY_MEMORY_BUFFER (2 * 1024 * 1024)
static uint8_t otaTemporaryMemoryBuffer[FLASH_MAL_OTA_TEMPORARY_MEMORY_BUFFER] __attribute__((section(".psram")));
#endif // MODULE_FUNCTION == MOD_FUNC_BOOTLOADER

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
        case FLASH_ADDRESS: {
            ok = (hal_memory_read(addr, buf, size) == 0);
            break;
        }
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
        case FLASH_ADDRESS: {
            ok = (hal_memory_write(addr, buf, size) == 0);
            break;
        }
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

int calculate_prefix_size(flash_device_t source, uint32_t sourceAddress, uint32_t length) {
    if (length < sizeof(module_info_t)) {
        return -1;
    }

    module_info_t info = {};
    if (FLASH_ModuleInfo(&info, source, sourceAddress, NULL) != SYSTEM_ERROR_NONE) {
        return -1;
    }

    size_t prefix_size = sizeof(module_info_t);


    if (info.flags & MODULE_INFO_FLAG_PREFIX_EXTENSIONS) {
        size_t pos = prefix_size;
        while (pos < length) {
            module_info_extension_t ext = {};
            if (!flash_read(source, sourceAddress + pos, (uint8_t*)&ext, sizeof(ext))) {
                return -1;
            }
            pos += ext.length;
            if (ext.type == MODULE_INFO_EXTENSION_END) {
                break;
            }
        }
        prefix_size = pos;
    }
    return prefix_size;
}

static bool verify_module(flash_device_t src_dev, uintptr_t src_addr, size_t src_size, flash_device_t dest_dev,
        uintptr_t dest_addr, size_t dest_size, uint8_t module_func, uint8_t flags) {
    if (!FLASH_CheckValidAddressRange(src_dev, src_addr, src_size)) {
        return false;
    }
    if (!FLASH_CheckValidAddressRange(dest_dev, dest_addr, dest_size)) {
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
        module_info_t info = {};
        if (FLASH_ModuleInfo(&info, src_dev, src_addr, NULL) == SYSTEM_ERROR_NONE) {
            module_info_start_addr = (uintptr_t)info.module_start_address;
            module_info_size = info.module_end_address - info.module_start_address;
            module_info_platform_id = info.platform_id;
            module_info_func = info.module_function;
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
            return false;
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
    int prefix_size = calculate_prefix_size(dev, addr, size);
    if (prefix_size <= 0) {
        return false;
    }
    if (size < prefix_size + sizeof(compressed_module_header)) {
        return false;
    }
    if (!flash_read(dev, addr + prefix_size, (uint8_t*)header, sizeof(compressed_module_header))) {
        return false;
    }
    if (header->size > sizeof(compressed_module_header) && size < (size_t)prefix_size + header->size) {
        return false;
    }
    if (header->method != 0) { // Raw Deflate
        return false;
    }
    return true;
}

#endif // HAS_COMPRESSED_OTA

bool FLASH_CheckValidAddressRange(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length) {
#ifndef USE_SERIAL_FLASH
    if (flashDeviceID == FLASH_SERIAL) {
        return false;
    }
#endif

    // FIXME: remove magic numbers
    uint32_t endAddress = startAddress + length - 1;
    if (flashDeviceID == FLASH_INTERNAL) {
        return (startAddress >= 0x08000000 && endAddress < 0x08800000);
    } else if (flashDeviceID == FLASH_SERIAL) {
        return startAddress >= 0x00000000 && endAddress < EXTERNAL_FLASH_SIZE;
    } else if (flashDeviceID == FLASH_ADDRESS) {
        // FIXME:
#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
        if (startAddress >= (uintptr_t)otaTemporaryMemoryBuffer && length <= sizeof(otaTemporaryMemoryBuffer)) {
            return true;
        }
#else
        return false;
#endif // MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
    }
    return false;
}

bool FLASH_EraseMemory(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length) {
#ifndef USE_SERIAL_FLASH
    if (flashDeviceID == FLASH_SERIAL) {
        return false;
    }
#endif

    uint32_t numSectors;
    if (FLASH_CheckValidAddressRange(flashDeviceID, startAddress, length) != true) {
        return false;
    }
    if (flashDeviceID == FLASH_INTERNAL) {
        numSectors = CEIL_DIV(length, INTERNAL_FLASH_PAGE_SIZE);
        if (hal_flash_erase_sector(startAddress, numSectors) != 0) {
            return false;
        }
        return true;
    } else if (flashDeviceID == FLASH_SERIAL) {
        numSectors = CEIL_DIV(length, sFLASH_PAGESIZE);
        if (hal_exflash_erase_sector(startAddress, numSectors) != 0) {
            return false;
        }
        return true;
    } else if (flashDeviceID == FLASH_ADDRESS) {
        // Not supported
        return true;
    }
    return false;
}

int FLASH_CopyMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                     flash_device_t destinationDeviceID, uint32_t destinationAddress,
                     uint32_t length, uint8_t module_function, uint8_t flags) {
    size_t dest_size = length;
    int prefix_size = calculate_prefix_size(sourceDeviceID, sourceAddress, length);
    if (prefix_size <= 0) {
        return FLASH_ACCESS_RESULT_ERROR;
    }
#if HAS_COMPRESSED_OTA
    compressed_module_header comp_header = { 0 };
    if (flags & MODULE_COMPRESSED) {
        if (!parse_compressed_module_header(sourceDeviceID, sourceAddress, length, &comp_header)) {
            return FLASH_ACCESS_RESULT_ERROR;
        }
        dest_size = comp_header.original_size;
    }
#endif // HAS_COMPRESSED_OTA

    // Check if the memory copy can be performed.
    if (!verify_module(sourceDeviceID, sourceAddress, length, destinationDeviceID, destinationAddress, dest_size,
            module_function, flags)) {
        return FLASH_ACCESS_RESULT_BADARG;
    }
    if (!FLASH_EraseMemory(destinationDeviceID, destinationAddress, dest_size)) {
        return FLASH_ACCESS_RESULT_ERROR;
    }
    if (flags & MODULE_COMPRESSED) {
#if HAS_COMPRESSED_OTA
        // Skip the module info and compressed data headers
        if (length < (size_t)prefix_size + comp_header.size + 2 /* Prefix size */ + 4 /* CRC-32 */) { // Sanity check
            return FLASH_ACCESS_RESULT_BADARG;
        }
        sourceAddress += prefix_size + comp_header.size;
        length -= prefix_size + comp_header.size + 4;
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
        if (!flash_copy(sourceDeviceID, sourceAddress, destinationDeviceID, destinationAddress, length)) {
            return FLASH_ACCESS_RESULT_ERROR;
        }
    }
    return FLASH_ACCESS_RESULT_OK;
}

bool FLASH_CompareMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                         flash_device_t destinationDeviceID, uint32_t destinationAddress, uint32_t length) {
#ifndef USE_SERIAL_FLASH
    if (flashDeviceID == FLASH_SERIAL) {
        return false;
    }
#endif

    uint32_t endAddress = sourceAddress + length;
    // check address range
    if (FLASH_CheckValidAddressRange(sourceDeviceID, sourceAddress, length) != true) {
        return false;
    }
    if (FLASH_CheckValidAddressRange(destinationDeviceID, destinationAddress, length) != true) {
        return false;
    }
    uint8_t src_buf[COPY_BLOCK_SIZE];
    uint8_t dest_buf[COPY_BLOCK_SIZE];
    uint32_t copy_len = 0;
    /* Program source to destination */
    while (sourceAddress < endAddress) {
        copy_len = (endAddress - sourceAddress) >= COPY_BLOCK_SIZE ? COPY_BLOCK_SIZE : (endAddress - sourceAddress);
        if(!flash_read(sourceDeviceID, sourceAddress, src_buf, copy_len)) {
            return false;
        }
        if(!flash_read(destinationDeviceID, destinationAddress, dest_buf, copy_len)) {
            return false;
        }
        if (memcmp(src_buf, dest_buf, copy_len)) {
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
                                         uint32_t length, uint8_t function, uint8_t flags) {
    platform_flash_modules_t flash_modules[MAX_MODULES_SLOT];
    uint8_t flash_module_index = MAX_MODULES_SLOT;

    //Read the flash modules info from the dct area
    dct_read_app_data_copy(DCT_FLASH_MODULES_OFFSET, flash_modules, sizeof(flash_modules));

    //fill up the next available modules slot and return true else false
    //slot 0 is reserved for factory reset module so start from flash_module_index = 1
    for (flash_module_index = GEN_START_SLOT; flash_module_index < MAX_MODULES_SLOT; flash_module_index++) {
        if(flash_modules[flash_module_index].magicNumber == 0xABCD) {
            continue;
        } else {
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

bool FLASH_IsFactoryResetAvailable(void) {
    return false;
}

#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
volatile int ipcResult = -1;
static void bldUpdateCallback(km0_km4_ipc_msg_t* msg, void* context) {
    if (!msg->data) {
        ipcResult = SYSTEM_ERROR_BAD_DATA;
    } else {
        DCache_Invalidate((uint32_t)msg->data, sizeof(int));
        ipcResult = ((int32_t*)(msg->data))[0];
    }
}

extern uintptr_t platform_user_part_flash_end;
extern uintptr_t platform_system_part1_flash_start;

typedef struct platform_flash_mal_memory_region {
    uintptr_t start;
    uintptr_t end;
} platform_flash_mal_memory_region;

typedef struct platform_flash_mal_overlap_region {
    platform_flash_mal_memory_region src;
    platform_flash_mal_memory_region dst;
} platform_flash_mal_overlap_region;

static bool isWithinOtaRegion(uintptr_t address, flash_device_t location) {
    return address >= (uintptr_t)&platform_system_part1_flash_start && address < (uintptr_t)&platform_user_part_flash_end &&
            location == FLASH_INTERNAL;
}

static bool isValidUpdateSlot(platform_flash_modules_t* module) {
    return module->magicNumber == 0xabcd;
}

static bool isSourceDestinationOverlap(platform_flash_modules_t* module, platform_flash_mal_overlap_region* overlap) {
    if (!isValidUpdateSlot(module)) {
        return false;
    }

    size_t srcAddr = module->sourceAddress;
    size_t dstAddr = module->destinationAddress;
    size_t srcAddrEnd = module->sourceAddress + module->length;
    size_t dstAddrEnd = module->destinationAddress + module->length;

    if (!(isWithinOtaRegion(srcAddr, module->sourceDeviceID) && isWithinOtaRegion(dstAddr, module->destinationDeviceID))) {
        return false;
    }

#if HAS_COMPRESSED_OTA
    compressed_module_header comp_header = {};
    if (module->flags & MODULE_COMPRESSED) {
        if (!parse_compressed_module_header(module->sourceDeviceID, module->sourceAddress, module->length, &comp_header)) {
            // This module will not be decompressed anyway
            return false;
        }
        dstAddrEnd = dstAddr + comp_header.original_size;
    }
#endif // HAS_COMPRESSED_OTA

    // Align to flash page size
    dstAddrEnd = (dstAddrEnd + INTERNAL_FLASH_PAGE_SIZE - 1) / INTERNAL_FLASH_PAGE_SIZE * INTERNAL_FLASH_PAGE_SIZE;

    if (overlap) {
        overlap->src.start = srcAddr;
        overlap->src.end = srcAddrEnd;
        overlap->dst.start = dstAddr;
        overlap->dst.end = dstAddrEnd;
    }

    if (srcAddr >= dstAddr && srcAddr < dstAddrEnd) {
        return true;
    }
    if (srcAddrEnd > dstAddr && srcAddrEnd <= dstAddrEnd) {
        return true;
    }
    return false;
}

static size_t countValidUpdateSlots(platform_flash_modules_t* modules, size_t count) {
    size_t valid = 0;
    for (platform_flash_modules_t* module = modules; module < modules + count; module++) {
        if (isValidUpdateSlot(module)) {
            valid++;
        }
    }
    return valid;
}

#endif

static int invalidateUpdateSlot(platform_flash_modules_t* module, size_t offset) {
    module->magicNumber = 0xffff;
    return dct_write_app_data(&module->magicNumber, offset + offsetof(platform_flash_modules_t, magicNumber), sizeof(module->magicNumber));
}

int FLASH_AddMfgSystemModuleSlot(void) {
    int ret = SYSTEM_ERROR_INTERNAL;
    uint32_t systemPartOffset = MFG_COMBINED_FW_START_ADDRESS + FLASH_ModuleLength(FLASH_INTERNAL, MFG_COMBINED_FW_START_ADDRESS) + 4/*CRC32*/;
    if (systemPartOffset > MFG_COMBINED_FW_START_ADDRESS) {
        module_info_t info;
        ret = FLASH_ModuleInfo(&info, FLASH_INTERNAL, systemPartOffset, NULL);
        if (ret == SYSTEM_ERROR_NONE && info.module_function == MODULE_FUNCTION_SYSTEM_PART && module_info_matches_platform(&info)) {
            if (!FLASH_VerifyCRC32(FLASH_INTERNAL, systemPartOffset, module_length(&info))) {
                return SYSTEM_ERROR_BAD_DATA;
            }
            uint32_t copyLength = info.module_end_address - info.module_start_address + 4/*CRC32*/;
            uint8_t slotFlags = MODULE_VERIFY_CRC | MODULE_VERIFY_DESTINATION_IS_START_ADDRESS | MODULE_VERIFY_FUNCTION;
            if (!FLASH_AddToNextAvailableModulesSlot(FLASH_INTERNAL, systemPartOffset, FLASH_INTERNAL,
                    (uint32_t)info.module_start_address, copyLength, MOD_FUNC_SYSTEM_PART, slotFlags)) {
                ret = SYSTEM_ERROR_NO_MEMORY;
            }
        }
    }
    return ret;
}

// This function is called in bootloader to perform the memory update process
int FLASH_UpdateModules(void (*flashModulesCallback)(bool isUpdating)) {
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

    for (size_t i = 0; i < module_count; ++i, offs += sizeof(platform_flash_modules_t)) {
        platform_flash_modules_t* const module = &modules[i];
        if (module->magicNumber == 0xabcd) {
            bool resetSlot = true;
            if (!has_modules) {
                has_modules = true;
                // Turn on RGB_COLOR_MAGENTA toggling during flash update
                if (flashModulesCallback) {
                    flashModulesCallback(true);
                }
            }
            if (module->module_function != MODULE_FUNCTION_BOOTLOADER) {
#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
                r = 0;
                platform_flash_mal_overlap_region overlap = {};
                if (isSourceDestinationOverlap(module, &overlap)) {
                    if (countValidUpdateSlots(modules, module_count) > 1) {
                        // Can't apply for now, wait until the rest of the updates are applied
                        r = result = FLASH_ACCESS_RESULT_RESET_PENDING;
                        // Skip the current slot
                        resetSlot = false;
                    } else {
                        // Can apply the update, copy first into RAM, invalidate slot, copy back into flash, update slot
                        // There is a chance that the update process will be interrupted here and we'll lose the update
                        // request (slot), but this should be ok, as this whole process only applies to system-part1 updates
                        // and the worst that will happen is that we'll boot back into the old system-part1 (with the rest of
                        // the modules potentially updated). This is not dangerous and does not go against the dependency direction.
                        bool ok = FLASH_CopyMemory(module->sourceDeviceID, module->sourceAddress, FLASH_ADDRESS, (uintptr_t)otaTemporaryMemoryBuffer, module->length, 0, 0) == FLASH_ACCESS_RESULT_OK;
                        ok = ok && (invalidateUpdateSlot(module, offs)) == 0;
                        ok = ok && FLASH_CopyMemory(FLASH_ADDRESS, (uintptr_t)otaTemporaryMemoryBuffer, module->sourceDeviceID, overlap.dst.end, module->length, 0, 0) == FLASH_ACCESS_RESULT_OK;
                        // Finally patch the module slot
                        if (ok) {
                            module->sourceAddress = overlap.dst.end;
                            module->magicNumber = 0xabcd;
                            ok = !dct_write_app_data(module, offs, sizeof(*module));
                        }
                        if (!ok) {
                            r = FLASH_ACCESS_RESULT_ERROR;
                            // Keep the module update slot if it's still valid
                            resetSlot = false;
                        }
                    }
                }
#endif // MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
                // Copy memory from source to destination based on flash device ID
                if (!r) {
                    r = FLASH_CopyMemory(module->sourceDeviceID, module->sourceAddress, module->destinationDeviceID,
                            module->destinationAddress, module->length, module->module_function, module->flags);
                }
                if (r != FLASH_ACCESS_RESULT_OK && result != FLASH_ACCESS_RESULT_RESET_PENDING) {
                    // Propagate errors, prioritize FLASH_ACCESS_RESULT_RESET_PENDING over errors
                    result = r;
                }
            } else {
#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
                // Send IPC message to KM0 to perform the memory copy
                // Check if the memory copy can be performed.
                if (verify_module(module->sourceDeviceID, module->sourceAddress, module->length, module->destinationDeviceID,
                        module->destinationAddress, module->length, module->module_function, module->flags)) {
                    memcpy(&sModule.module, module, sizeof(platform_flash_modules_t));
                    if (sModule.module.flags & MODULE_DROP_MODULE_INFO) {
                        // Skip the module info header
                        if (sModule.module.length < sizeof(module_info_t)) { // Sanity check
                            result = FLASH_ACCESS_RESULT_BADARG;
                        }
                        sModule.module.sourceAddress += sizeof(module_info_t);
                        sModule.module.length -= sizeof(module_info_t);
                    }
                    int ret = km0_km4_ipc_send_request(KM0_KM4_IPC_CHANNEL_GENERIC, KM0_KM4_IPC_MSG_BOOTLOADER_UPDATE, &sModule.module,
                                                        sizeof(platform_flash_modules_t), bldUpdateCallback, NULL);
                    if (ret != 0 || ipcResult != 0) {
                        result = FLASH_ACCESS_RESULT_ERROR;
                        resetSlot = false;
                    } else {
                        has_bootloader = true;
                    }
                } else {
                    result = FLASH_ACCESS_RESULT_BADARG;
                }
#else
                result = FLASH_ACCESS_RESULT_BADARG;
#endif
            }
            if (resetSlot) {
                // Mark the slot as unused
                r = invalidateUpdateSlot(module, offs);
                if (r != 0 && result != FLASH_ACCESS_RESULT_RESET_PENDING) {
                    result = FLASH_ACCESS_RESULT_ERROR;
                }
            }
            if (has_bootloader) {
                break; // other valid module slots will be handled after reset
            }
        }
    }
    if (has_bootloader) {
        result = FLASH_ACCESS_RESULT_RESET_PENDING;
    }
    // Turn off RGB_COLOR_MAGENTA toggling
    if (has_modules && flashModulesCallback) {
        flashModulesCallback(false);
    }
    return result;
}

int FLASH_ModuleInfo(module_info_t* const infoOut, uint8_t flashDeviceID, uint32_t startAddress, uint32_t* infoOffset) {
#ifndef USE_SERIAL_FLASH
    if (flashDeviceID == FLASH_SERIAL) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
#endif
    CHECK_TRUE(infoOut, SYSTEM_ERROR_INVALID_ARGUMENT);

    rtl_binary_header header = {};
    if (!flash_read(flashDeviceID, startAddress, (uint8_t*)&header, sizeof(rtl_binary_header))) {
        goto error_return;
    }
    uint32_t offset = 0;
    if (header.signature_high == RTL_HEADER_SIGNATURE_HIGH && header.signature_low == RTL_HEADER_SIGNATURE_LOW) {
        offset = sizeof(rtl_binary_header);
    }
    if (!flash_read(flashDeviceID, startAddress + offset, (uint8_t*)infoOut, sizeof(module_info_t))) {
        goto error_return;
    }
    if (infoOffset) {
        *infoOffset = offset;
    }
    return SYSTEM_ERROR_NONE;

error_return:
    return SYSTEM_ERROR_INTERNAL;
}

int FLASH_ModuleCrcSuffix(module_info_crc_t* crc, module_info_suffix_t* suffix, uint8_t flashDeviceID, uint32_t endAddress) {
#ifndef USE_SERIAL_FLASH
    if (flashDeviceID == FLASH_SERIAL) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
#endif
    if (crc) {
        if(!flash_read(flashDeviceID, endAddress, (uint8_t*)crc, sizeof(module_info_crc_t))) {
            goto error_return;
        }
    }
    if (suffix) {
        // suffix [endAddress] crc32
        endAddress = endAddress - sizeof(module_info_suffix_t);
        if(!flash_read(flashDeviceID, endAddress, (uint8_t*)suffix, sizeof(module_info_suffix_t))) {
            goto error_return;
        }
    }
    return SYSTEM_ERROR_NONE;

error_return:
    return SYSTEM_ERROR_INTERNAL;
}

uint32_t FLASH_ModuleAddress(uint8_t flashDeviceID, uint32_t startAddress) {
    module_info_t info = {};
    if (FLASH_ModuleInfo(&info, flashDeviceID, startAddress, NULL) == SYSTEM_ERROR_NONE) {
        return (uint32_t)info.module_start_address;
    }
    return 0;
}

uint32_t FLASH_ModuleLength(uint8_t flashDeviceID, uint32_t startAddress) {
    module_info_t info = {};
    if (FLASH_ModuleInfo(&info, flashDeviceID, startAddress, NULL) == SYSTEM_ERROR_NONE) {
        return ((uint32_t)info.module_end_address - (uint32_t)info.module_start_address);
    }
    return 0;
}

uint16_t FLASH_ModuleVersion(uint8_t flashDeviceID, uint32_t startAddress) {
    module_info_t info = {};
    if (FLASH_ModuleInfo(&info, flashDeviceID, startAddress, NULL) == SYSTEM_ERROR_NONE) {
        return info.module_version;
    }
    return 0;
}

bool FLASH_isUserModuleInfoValid(uint8_t flashDeviceID, uint32_t startAddress, uint32_t expectedAddress) {
    module_info_t info = {};
    int ret = FLASH_ModuleInfo(&info, flashDeviceID, startAddress, NULL);
    return (ret == SYSTEM_ERROR_NONE
            && ((uint32_t)(info.module_start_address) == expectedAddress)
            && ((uint32_t)(info.module_end_address) <= 0x08600000)
            && (info.platform_id == PLATFORM_ID));
}

bool FLASH_VerifyCRC32(uint8_t flashDeviceID, uint32_t startAddress, uint32_t length) {
#ifndef USE_SERIAL_FLASH
    if (flashDeviceID == FLASH_SERIAL) {
        return false;
    }
#endif

    if ((flashDeviceID == FLASH_INTERNAL && length > 0) || (flashDeviceID == FLASH_SERIAL && length > 0)) {
        const int dataBufferSize = 256;
        uint8_t dataBuffer[dataBufferSize];
        if (dataBuffer == NULL) {
            return false;
        }
        flash_read(flashDeviceID, (startAddress + length), dataBuffer, 4);
        uint32_t expectedCRC = (uint32_t)(dataBuffer[3] | (dataBuffer[2] << 8) | (dataBuffer[1] << 16) | (dataBuffer[0] << 24));
        uint32_t endAddress = startAddress + length;
        uint32_t len = 0;
        uint32_t computedCRC = 0;
        do {
            len = endAddress - startAddress;
            if (len > dataBufferSize) {
                len = dataBufferSize;
            }
            flash_read(flashDeviceID, startAddress, dataBuffer, len);
            computedCRC = Compute_CRC32(dataBuffer, len, &computedCRC);
            startAddress += len;
        } while (startAddress < endAddress);
        if (expectedCRC == computedCRC) {
            return true;
        }
    } else if (flashDeviceID == FLASH_ADDRESS && length > 0) {
        uint32_t expectedCRC = __REV((*(__IO uint32_t*)(startAddress + length)));
        uint32_t computedCRC = Compute_CRC32((uint8_t*)startAddress, length, NULL);
        if (expectedCRC == computedCRC) {
            return true;
        }
    }
    return false;
}

static const uint32_t crc32_tab[] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

/**
 * @brief  Computes the 32-bit CRC of a given buffer of byte data.
 * @param  pBuffer: pointer to the buffer containing the data to be computed
 * @param  BufferSize: Size of the buffer to be computed
 * @retval 32-bit CRC
 */
uint32_t Compute_CRC32(const uint8_t *pBuffer, uint32_t bufferSize, uint32_t const *p_crc) {
    uint32_t crc = (p_crc) ? *p_crc : 0;
    crc = crc ^ ~0U;
    while (bufferSize--) {
        crc = crc32_tab[(crc ^ *pBuffer++) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ ~0U;
}

int FLASH_Begin(flash_device_t flashDeviceID, uint32_t FLASH_Address, uint32_t imageSize) {
    system_flags.OTA_FLASHED_Status_SysFlag = 0x0000;
    Save_SystemFlags();

    // Clear all non-factory module slots in the DCT
    const size_t slot_count = MAX_MODULES_SLOT - GEN_START_SLOT;
    size_t slot_offs = DCT_FLASH_MODULES_OFFSET + sizeof(platform_flash_modules_t) * GEN_START_SLOT;
    for (size_t i = 0; i < slot_count; ++i) {
        uint16_t magic_num = 0;
        int r = dct_read_app_data_copy(slot_offs + offsetof(platform_flash_modules_t, magicNumber), &magic_num,
                sizeof(magic_num));
        if (r != 0) {
            return FLASH_ACCESS_RESULT_ERROR;
        }
        if (magic_num == 0xabcd) {
            // Mark the slot as unused
            magic_num = 0xffff;
            r = dct_write_app_data(&magic_num, slot_offs + offsetof(platform_flash_modules_t, magicNumber),
                    sizeof(magic_num));
            if (r != 0) {
                return FLASH_ACCESS_RESULT_ERROR;
            }
        }
        slot_offs += sizeof(platform_flash_modules_t);
    }

    bool ok = false;
    if (flashDeviceID == FLASH_INTERNAL) {
        ok = FLASH_EraseMemory(FLASH_INTERNAL, FLASH_Address, imageSize);
    } else if (flashDeviceID == FLASH_SERIAL) {
#ifdef USE_SERIAL_FLASH
        ok = FLASH_EraseMemory(FLASH_SERIAL, FLASH_Address, imageSize);
#endif
    } else if (flashDeviceID == FLASH_ADDRESS) {
        ok = true;
    }
    if (!ok) {
        return FLASH_ACCESS_RESULT_ERROR;
    }
    return FLASH_ACCESS_RESULT_OK;
}

int FLASH_Update(flash_device_t flashDeviceID, const uint8_t *pBuffer, uint32_t address, uint32_t bufferSize) {
    int ret = -1;
    if (flashDeviceID == FLASH_INTERNAL) {
        ret = hal_flash_write(address, pBuffer, bufferSize);
    } else if (flashDeviceID == FLASH_SERIAL) {
#ifdef USE_SERIAL_FLASH
        ret = hal_exflash_write(address, pBuffer, bufferSize);
#endif
    } else if (flashDeviceID == FLASH_ADDRESS) {
        ret = SYSTEM_ERROR_NOT_SUPPORTED;
    }
    return ret;
}
