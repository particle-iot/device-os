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

// Decompression of firmware modules is only supported in the bootloader
#if (HAL_PLATFORM_COMPRESSED_OTA) && (MODULE_FUNCTION == MOD_FUNC_BOOTLOADER)
#define HAS_COMPRESSED_OTA 0 // Temporarily disable it for now, since the bootloader update doesn't perform memory copy
#else
#define HAS_COMPRESSED_OTA 0
#endif

#define CEIL_DIV(A, B)        (((A) + (B) - 1) / (B))

#define COPY_BLOCK_SIZE 256

#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
__attribute__((section(".boot.ipc_data"))) platform_flash_modules_t sModule;
#endif

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

bool FLASH_CheckValidAddressRange(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length) {
    // FIXME: remove magic numbers
    uint32_t endAddress = startAddress + length - 1;
    if (flashDeviceID == FLASH_INTERNAL) {
        return (startAddress >= 0x08000000 && endAddress <= 0x08800000);
    } else if (flashDeviceID == FLASH_SERIAL) {
#ifdef USE_SERIAL_FLASH
        return startAddress >= 0x00000000 && endAddress <= EXTERNAL_FLASH_SIZE;
#else
        return false;
#endif
    }
    return false;
}

bool FLASH_EraseMemory(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length) {
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
#ifdef USE_SERIAL_FLASH
        numSectors = CEIL_DIV(length, sFLASH_PAGESIZE);
        if (hal_exflash_erase_sector(startAddress, numSectors) != 0) {
            return false;
        }
        return true;
#else
        return false;
#endif
    }
    return false;
}

int FLASH_CopyMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                     flash_device_t destinationDeviceID, uint32_t destinationAddress,
                     uint32_t length, uint8_t module_function, uint8_t flags) {
    size_t dest_size = length;
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
        if (length < sizeof(module_info_t) + comp_header.size + 2 /* Prefix size */ + 4 /* CRC-32 */) { // Sanity check
            return FLASH_ACCESS_RESULT_BADARG;
        }
        sourceAddress += sizeof(module_info_t) + comp_header.size;
        length -= sizeof(module_info_t) + comp_header.size + 4;
        // Determine where the compressed data ends
        uint16_t prefix_size = 0;
        if (!flash_read(sourceDeviceID, sourceAddress + length - 2, (uint8_t*)&prefix_size, 2)) {
            return FLASH_ACCESS_RESULT_ERROR;
        }
        if (length < prefix_size) {
            return FLASH_ACCESS_RESULT_BADARG;
        }
        length -= prefix_size;
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
        // Read data from source memory address
        if (sourceDeviceID == FLASH_INTERNAL) {
            if (hal_flash_read(sourceAddress, src_buf, copy_len)) {
                return false;
            }
        } else if (sourceDeviceID == FLASH_SERIAL) {
#ifdef USE_SERIAL_FLASH
            if (hal_exflash_read(sourceAddress, src_buf, copy_len)) {
                return false;
            }
#else
            return false;
#endif
        }
        // Read data from destination memory address
        if (sourceDeviceID == FLASH_INTERNAL) {
            if (hal_flash_read(destinationAddress, dest_buf, copy_len)) {
                return false;
            }
        } else if (sourceDeviceID == FLASH_SERIAL) {
#ifdef USE_SERIAL_FLASH
            if (hal_exflash_read(destinationAddress, dest_buf, copy_len)) {
                return false;
            }
#else
            return false;
#endif
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
    DCache_Invalidate((uint32_t)msg->data, sizeof(int));
    ipcResult = *((int*)msg->data);
}
#endif

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
    for (size_t i = 0; i < module_count; ++i) {
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
                // Copy memory from source to destination based on flash device ID
                r = FLASH_CopyMemory(module->sourceDeviceID, module->sourceAddress, module->destinationDeviceID,
                        module->destinationAddress, module->length, module->module_function, module->flags);
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
                    uint8_t ipcMsgType = (module->destinationAddress == KM4_BOOTLOADER_START_ADDRESS) ? KM0_KM4_IPC_MSG_BOOTLOADER_UPDATE : KM0_KM4_IPC_MSG_KM0_PART1_UPDATE;
                    memcpy(&sModule, module, sizeof(platform_flash_modules_t));
                    int ret = km0_km4_ipc_send_request(KM0_KM4_IPC_CHANNEL_GENERIC, ipcMsgType, &sModule,
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
                module->magicNumber = 0xffff;
                r = dct_write_app_data(&module->magicNumber, offs + offsetof(platform_flash_modules_t, magicNumber), sizeof(module->magicNumber));
                if (r != 0 && result != FLASH_ACCESS_RESULT_RESET_PENDING) {
                    result = FLASH_ACCESS_RESULT_ERROR;
                }
            }
        }
        offs += sizeof(platform_flash_modules_t);
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
    CHECK_TRUE(infoOut, SYSTEM_ERROR_INVALID_ARGUMENT);
    uint32_t offset = 0;
    if (flashDeviceID == FLASH_INTERNAL) {
        rtl_binary_header* header = (rtl_binary_header*)startAddress;
        if (header->signature_high == RTL_HEADER_SIGNATURE_HIGH && header->signature_low == RTL_HEADER_SIGNATURE_LOW) {
            offset = sizeof(rtl_binary_header);
        }
        int ret = hal_flash_read(startAddress + offset, (uint8_t*)infoOut, sizeof(module_info_t));
        if (ret != SYSTEM_ERROR_NONE) {
            return ret;
        }
    } else if (flashDeviceID == FLASH_SERIAL) {
#ifdef USE_SERIAL_FLASH
        rtl_binary_header header = {};
        int ret = hal_exflash_read(startAddress, (uint8_t*)&header, sizeof(rtl_binary_header));
        if (ret != SYSTEM_ERROR_NONE) {
            return ret;
        }
        if (header.signature_high == RTL_HEADER_SIGNATURE_HIGH && header.signature_low == RTL_HEADER_SIGNATURE_LOW) {
            offset = sizeof(rtl_binary_header);
        }
        ret = hal_exflash_read(startAddress + offset, (uint8_t*)infoOut, sizeof(module_info_t));
        if (ret != SYSTEM_ERROR_NONE) {
            return ret;
        }
#else
        return SYSTEM_ERROR_NOT_SUPPORTED;
#endif
    }
    if (infoOffset) {
        *infoOffset = offset;
    }
    return SYSTEM_ERROR_NONE;
}

int FLASH_ModuleCrcSuffix(module_info_crc_t* crc, module_info_suffix_t* suffix, uint8_t flashDeviceID, uint32_t endAddress) {
    typedef int (*readFn_t)(uintptr_t addr, uint8_t* data_buf, size_t size);
    readFn_t read = NULL;

    if (flashDeviceID == FLASH_INTERNAL) {
        read = hal_flash_read;
    } else if (flashDeviceID == FLASH_SERIAL) {
#ifdef USE_SERIAL_FLASH
        read = hal_exflash_read;
#else
        returrn SYSTEM_ERROR_NOT_SUPPORTED;
#endif
    }
    if (read) {
        if (crc) {
            int ret = read(endAddress, (uint8_t*)crc, sizeof(module_info_crc_t));
            if (ret != SYSTEM_ERROR_NONE) {
                return ret;
            }
        }
        if (suffix) {
            // suffix [endAddress] crc32
            endAddress = endAddress - sizeof(module_info_suffix_t);
            int ret = read(endAddress, (uint8_t*)suffix, sizeof(module_info_suffix_t));
            if (ret != SYSTEM_ERROR_NONE) {
                return ret;
            }
        }
    }
    return SYSTEM_ERROR_NONE;
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
    if (flashDeviceID == FLASH_INTERNAL && length > 0) {
        uint32_t expectedCRC = __REV((*(__IO uint32_t*)(startAddress + length)));
        uint32_t computedCRC = Compute_CRC32((uint8_t*)startAddress, length, NULL);
        if (expectedCRC == computedCRC) {
            return true;
        }
    } else if (flashDeviceID == FLASH_SERIAL && length > 0) {
#ifdef USE_SERIAL_FLASH
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
        if (expectedCRC == computedCRC) {
            return true;
        }
#else
        return false;
#endif
    }
    return false;
}

void FLASH_Begin(flash_device_t flashDeviceID, uint32_t FLASH_Address, uint32_t imageSize) {
    system_flags.OTA_FLASHED_Status_SysFlag = 0x0000;
    Save_SystemFlags();
    if (flashDeviceID == FLASH_INTERNAL) {
        FLASH_EraseMemory(FLASH_INTERNAL, FLASH_Address, imageSize);
    } else if (flashDeviceID == FLASH_SERIAL) {
#ifdef USE_SERIAL_FLASH
        FLASH_EraseMemory(FLASH_SERIAL, FLASH_Address, imageSize);
#endif
    }
}

int FLASH_Update(flash_device_t flashDeviceID, const uint8_t *pBuffer, uint32_t address, uint32_t bufferSize) {
    int ret = -1;
    if (flashDeviceID == FLASH_INTERNAL) {
        ret = hal_flash_write(address, pBuffer, bufferSize);
    } else if (flashDeviceID == FLASH_SERIAL) {
#ifdef USE_SERIAL_FLASH
        ret = hal_exflash_write(address, pBuffer, bufferSize);
#endif
    }
    return ret;
}
