/*
 * Copyright (c) 2017 Particle Industries, Inc.  All rights reserved.
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

#include "storage.h"

#if SYSTEM_CONTROL_ENABLED

#include "system_update.h"
#include "common.h"

#include "ota_flash_hal_stm32f2xx.h"
#include "eeprom_hal.h"

#include "protocol_defs.h" // For UpdateFlag enum
#include "nanopb_misc.h"

#include "flash_storage_impl.h"
#include "eeprom_emulation_impl.h"
#include "dct.h"
#include "platforms.h"

#include "storage.pb.h"

#include <memory>

// Make sure platform-specific requests, such as CTRL_REQUEST_DESCRIBE_STORAGE, are updated for a
// newly introduced platform
#if PLATFORM_ID != PLATFORM_PHOTON_PRODUCTION && \
    PLATFORM_ID != PLATFORM_P1 && \
    PLATFORM_ID != PLATFORM_ELECTRON_PRODUCTION
#error "Unsupported platform"
#endif

// TODO: Add support for external flash (available if HAS_SERIAL_FLASH macro is defined)
#ifdef USE_SERIAL_FLASH
#error "External flash is not supported"
#endif

namespace particle {

namespace control {

using namespace protocol;
using namespace common;

namespace {

struct Section;

// Description of a storage
struct Storage {
    uint8_t type;
    uint8_t index;
    uint16_t flags;
    const Section* sections;
    uint8_t sectionCount;
};

// Section IO callbacks
typedef int(*sectionRead)(char* data, size_t size, uintptr_t address);
typedef int(*sectionWrite)(const char* data, size_t size, uintptr_t address);
typedef int(*sectionClear)(uintptr_t address, size_t size);

// Description of a storage section
struct Section {
    uint8_t type;
    uint8_t index;
    uint16_t flags;
    uint32_t address;
    uint32_t size;
    sectionRead read;
    sectionWrite write;
    sectionClear clear;
};

// Internal flash
int internalFlashRead(char* data, size_t size, uintptr_t address) {
    return InternalFlashStore::read(address, data, size);
}

int internalFlashWrite(const char* data, size_t size, uintptr_t address) {
    return InternalFlashStore::write(address, data, size);
}

int internalFlashClear(uintptr_t address, size_t size) {
    return InternalFlashStore::erase(address, size);
}

// DCT/DCD
int configRead(char* data, size_t size, uintptr_t address) {
    return dct_read_app_data_copy(address, data, size);
}

int configWrite(const char* data, size_t size, uintptr_t address) {
    return dct_write_app_data(data, address, size);
}

// EEPROM
int eepromRead(char* data, size_t size, uintptr_t address) {
    HAL_EEPROM_Get(address, data, size); // FIXME: Result code?
    return 0;
}

int eepromWrite(const char* data, size_t size, uintptr_t address) {
    HAL_EEPROM_Put(address, data, size); // FIXME: Result code?
    return 0;
}

int eepromClear(uintptr_t address, size_t size) {
    HAL_EEPROM_Clear(); // FIXME: Result code?
    return 0;
}

const Section INTERNAL_STORAGE_SECTIONS[] = {
    // Bootloader
    { particle_ctrl_SectionType_BOOTLOADER, 0, 0, module_bootloader.start_address, module_bootloader.maximum_size, internalFlashRead, nullptr, nullptr },
#if MODULAR_FIRMWARE
    // System part 1
    { particle_ctrl_SectionType_SYSTEM_FIRMWARE, 0, 0, module_system_part1.start_address, module_system_part1.maximum_size, internalFlashRead, nullptr, nullptr },
    // System part 2
    { particle_ctrl_SectionType_SYSTEM_FIRMWARE, 1, 0, module_system_part2.start_address, module_system_part2.maximum_size, internalFlashRead, nullptr, nullptr },
#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
    // System part 3 (Electron)
    { particle_ctrl_SectionType_SYSTEM_FIRMWARE, 2, 0, module_system_part3.start_address, module_system_part3.maximum_size, internalFlashRead, nullptr, nullptr },
#endif
    // User part
    { particle_ctrl_SectionType_USER_FIRMWARE, 0, 0, module_user.start_address, module_user.maximum_size, internalFlashRead, nullptr, nullptr },
    // OTA backup
    { particle_ctrl_SectionType_OTA_BACKUP, 0, 0, module_ota.start_address, module_ota.maximum_size, internalFlashRead, nullptr, nullptr },
    // Factory backup
    { particle_ctrl_SectionType_FACTORY_BACKUP, 0, particle_ctrl_SectionFlag_NEED_CLEAR, module_factory.start_address, module_factory.maximum_size, internalFlashRead, internalFlashWrite, internalFlashClear },
#else // MODULAR_FIRMWARE
    // Monolithic firmware
    { particle_ctrl_SectionType_MONO_FIRMWARE, 0, 0, module_user_mono.start_address, module_user_mono.maximum_size, internalFlashRead, nullptr, nullptr },
    // Factory backup (monolithic firmware)
    { particle_ctrl_SectionType_FACTORY_BACKUP, 0, particle_ctrl_SectionFlag_NEED_CLEAR, module_factory_mono.start_address, module_factory_mono.maximum_size, internalFlashRead, internalFlashWrite, internalFlashClear },
#endif // !MODULAR_FIRMWARE
    // DCT/DCD
    { particle_ctrl_SectionType_CONFIG, 0, 0, 0, sizeof(application_dct_t), configRead, configWrite, nullptr },
    // EEPROM
    { particle_ctrl_SectionType_EEPROM, 0, 0, 0, FlashEEPROM::capacity(), eepromRead, eepromWrite, eepromClear }
};

const size_t INTERNAL_STORAGE_SECTION_COUNT = sizeof(INTERNAL_STORAGE_SECTIONS) / sizeof(INTERNAL_STORAGE_SECTIONS[0]);

const Storage STORAGE[] = {
    // Internal flash
    { particle_ctrl_StorageType_INTERNAL, 0, 0, INTERNAL_STORAGE_SECTIONS, INTERNAL_STORAGE_SECTION_COUNT }
};

const size_t STORAGE_COUNT = sizeof(STORAGE) / sizeof(STORAGE[0]);

std::unique_ptr<FileTransfer::Descriptor> g_desc;

const Section* storageSection(unsigned storageIndex, unsigned sectionIndex) {
    if (storageIndex >= STORAGE_COUNT) {
        return nullptr;
    }
    const Storage& storage = STORAGE[storageIndex];
    if (sectionIndex >= storage.sectionCount) {
        return nullptr;
    }
    return &storage.sections[sectionIndex];
}

int cancelFirmwareUpdate() {
    int ret = 0;
    if (g_desc) {
        ret = Spark_Finish_Firmware_Update(*g_desc, UpdateFlag::ERROR, nullptr);
        if (ret == 0) {
            g_desc.reset();
        }
    }
    return ret;
}

void finishFirmwareUpdate(int result, void* data) {
    if (g_desc) {
        Spark_Finish_Firmware_Update(*g_desc, UpdateFlag::SUCCESS, nullptr);
    }
}

} // namespace

int startFirmwareUpdateRequest(ctrl_request* req) {
    particle_ctrl_StartFirmwareUpdateRequest pbReq = {};
    int ret = decodeRequestMessage(req, particle_ctrl_StartFirmwareUpdateRequest_fields, &pbReq);
    if (ret != 0) {
        return ret;
    }
    // Cancel current transfer
    ret = cancelFirmwareUpdate();
    if (ret != 0) {
        return ret;
    }
    g_desc.reset(new(std::nothrow) FileTransfer::Descriptor);
    if (!g_desc) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    g_desc->file_length = pbReq.size;
    g_desc->store = FileTransfer::Store::FIRMWARE;
    g_desc->chunk_size = 4096; // TODO: Determine depending on free RAM
    g_desc->file_address = 0;
    g_desc->chunk_address = 0;
    ret = Spark_Prepare_For_Firmware_Update(*g_desc, 0, nullptr);
    if (ret != 0) {
        g_desc.reset();
        return ret;
    }
    g_desc->chunk_address = g_desc->file_address;
    particle_ctrl_StartFirmwareUpdateReply pbRep = {};
    pbRep.chunk_size = g_desc->chunk_size;
    ret = encodeReplyMessage(req, particle_ctrl_StartFirmwareUpdateReply_fields, &pbRep);
    if (ret != 0) {
        cancelFirmwareUpdate();
        return ret;
    }
    return 0;
}

void finishFirmwareUpdateRequest(ctrl_request* req) {
    particle_ctrl_FinishFirmwareUpdateRequest pbReq = {};
    int ret = decodeRequestMessage(req, particle_ctrl_FinishFirmwareUpdateRequest_fields, &pbReq);
    if (ret != 0) {
        goto done;
    }
    if (!g_desc) {
        ret = SYSTEM_ERROR_INVALID_STATE;
        goto done;
    }
    if (g_desc->chunk_address != g_desc->file_address + g_desc->file_length) {
        ret = SYSTEM_ERROR_INVALID_STATE;
        goto cancel;
    }
    // Validate the firmware binary
    ret = Spark_Finish_Firmware_Update(*g_desc, UpdateFlag::SUCCESS | UpdateFlag::VALIDATE_ONLY, nullptr);
    if (ret != 0) {
        goto cancel;
    }
    if (!pbReq.validate_only) {
        // Reply to the host and apply the update
        system_ctrl_set_result(req, ret, finishFirmwareUpdate, nullptr, nullptr);
        return;
    }
cancel:
    cancelFirmwareUpdate();
done:
    system_ctrl_set_result(req, ret, nullptr, nullptr, nullptr);
}

int cancelFirmwareUpdateRequest(ctrl_request* req) {
    return cancelFirmwareUpdate();
}

int saveFirmwareDataRequest(ctrl_request* req) {
    particle_ctrl_SaveFirmwareDataRequest pbReq = {};
    DecodedString pbData(&pbReq.data);
    int ret = decodeRequestMessage(req, particle_ctrl_SaveFirmwareDataRequest_fields, &pbReq);
    if (ret != 0) {
        return ret;
    }
    if (!g_desc) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (pbData.size == 0 || g_desc->chunk_address + pbData.size > g_desc->file_address + g_desc->file_length) {
        cancelFirmwareUpdate();
        return SYSTEM_ERROR_OUT_OF_RANGE;
    }
    g_desc->chunk_size = pbData.size;
    ret = Spark_Save_Firmware_Chunk(*g_desc, (const uint8_t*)pbData.data, nullptr);
    if (ret != 0) {
        cancelFirmwareUpdate();
        return ret;
    }
    g_desc->chunk_address += g_desc->chunk_size;
    return 0;
}

int describeStorageRequest(ctrl_request* req) {
    particle_ctrl_DescribeStorageReply pbRep = {};
    pbRep.storage.funcs.encode = [](pb_ostream_t* strm, const pb_field_t* field, void* const* arg) {
        for (size_t i = 0; i < STORAGE_COUNT; ++i) {
            const Storage& storage = STORAGE[i];
            particle_ctrl_DescribeStorageReply_Storage pbStorage = {};
            pbStorage.type = (particle_ctrl_StorageType)storage.type;
            pbStorage.index = storage.index;
            pbStorage.flags = storage.flags;
            pbStorage.sections.arg = const_cast<Storage*>(&storage);
            pbStorage.sections.funcs.encode = [](pb_ostream_t* strm, const pb_field_t* field, void* const* arg) {
                const Storage* storage = (const Storage*)*arg;
                for (size_t i = 0; i < storage->sectionCount; ++i) {
                    const Section& section = storage->sections[i];
                    particle_ctrl_DescribeStorageReply_Section pbSection = {};
                    pbSection.type = (particle_ctrl_SectionType)section.type;
                    pbSection.index = section.index;
                    pbSection.size = section.size;
                    pbSection.flags = section.flags;
                    if (section.read) {
                        pbSection.flags |= particle_ctrl_SectionFlag_READABLE;
                    }
                    if (section.write) {
                        pbSection.flags |= particle_ctrl_SectionFlag_WRITABLE;
                    }
                    if (section.clear) {
                        pbSection.flags |= particle_ctrl_SectionFlag_CLEARABLE;
                    }
                    if (!pb_encode_tag_for_field(strm, field)) {
                        return false;
                    }
                    if (!pb_encode_submessage(strm, particle_ctrl_DescribeStorageReply_Section_fields, &pbSection)) {
                        return false;
                    }
                }
                return true;
            };
            if (!pb_encode_tag_for_field(strm, field)) {
                return false;
            }
            if (!pb_encode_submessage(strm, particle_ctrl_DescribeStorageReply_Storage_fields, &pbStorage)) {
                return false;
            }
        }
        return true;
    };
    const int ret = encodeReplyMessage(req, particle_ctrl_DescribeStorageReply_fields, &pbRep);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

int loadStorageDataRequest(ctrl_request* req) {
    particle_ctrl_LoadStorageDataRequest pbReq = {};
    int ret = decodeRequestMessage(req, particle_ctrl_LoadStorageDataRequest_fields, &pbReq);
    if (ret != 0) {
        return ret;
    }
    const Section* section = storageSection(pbReq.storage, pbReq.section);
    if (!section) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    if (!section->read) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    if (pbReq.size == 0 || pbReq.offset + pbReq.size > section->size) {
        return SYSTEM_ERROR_OUT_OF_RANGE;
    }
    // Encode the data manually to avoid unnecessary buffer allocations
    pb_ostream_t* strm = pb_ostream_init(nullptr);
    if (!strm) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    const size_t repBufSize = pbReq.size + 16; // Allocate some extra bytes for the tag and length fields
    ret = system_ctrl_alloc_reply_data(req, repBufSize, nullptr);
    if (ret != 0) {
        goto cleanup;
    }
    if (!pb_ostream_from_buffer_ex(strm, (pb_byte_t*)req->reply_data, req->reply_size, nullptr) ||
            !pb_encode_tag(strm, PB_WT_STRING, particle_ctrl_LoadStorageDataReply_data_tag) ||
            !pb_encode_varint(strm, pbReq.size) || // String length
            repBufSize - strm->bytes_written < pbReq.size) {
        ret = SYSTEM_ERROR_INTERNAL;
        goto cleanup;
    }
    ret = section->read(req->reply_data + strm->bytes_written, pbReq.size, section->address + pbReq.offset);
    if (ret != 0) {
        goto cleanup;
    }
    strm->bytes_written += pbReq.size;
    req->reply_size = strm->bytes_written;
cleanup:
    pb_ostream_free(strm, nullptr);
    if (ret != 0) {
        system_ctrl_alloc_reply_data(req, 0, nullptr); // Free reply data
    }
    return ret;
}

int saveStorageDataRequest(ctrl_request* req) {
    particle_ctrl_SaveStorageDataRequest pbReq = {};
    DecodedString pbData(&pbReq.data);
    int ret = decodeRequestMessage(req, particle_ctrl_SaveStorageDataRequest_fields, &pbReq);
    if (ret != 0) {
        return ret;
    }
    const Section* section = storageSection(pbReq.storage, pbReq.section);
    if (!section) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    if (!section->write) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    if (pbData.size == 0 || pbReq.offset + pbData.size > section->size) {
        return SYSTEM_ERROR_OUT_OF_RANGE;
    }
    ret = section->write(pbData.data, pbData.size, section->address + pbReq.offset);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

int clearStorageSectionRequest(ctrl_request* req) {
    particle_ctrl_ClearStorageSectionRequest pbReq = {};
    int ret = decodeRequestMessage(req, particle_ctrl_ClearStorageSectionRequest_fields, &pbReq);
    if (ret != 0) {
        return ret;
    }
    const Section* section = storageSection(pbReq.storage, pbReq.section);
    if (!section) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    if (!section->clear) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    ret = section->clear(section->address, section->size);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

} // namespace particle::control

} // namespace particle

#endif // SYSTEM_CONTROL_ENABLED
