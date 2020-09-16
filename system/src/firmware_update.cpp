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

#include "logging.h"

LOG_SOURCE_CATEGORY("system.ota");

#include "firmware_update.h"

#include "system_task.h"
#include "system_event.h"
#include "system_led_signal.h"

#include "ota_flash_hal.h"
#include "timer_hal.h"

#include "scope_guard.h"
#include "check.h"
#include "debug.h"

#if HAL_PLATFORM_RESUMABLE_OTA
#include "simple_file_storage.h"
#include "sha256.h"
#endif // HAL_PLATFORM_RESUMABLE_OTA

#include "spark_wiring_system.h"
#include "spark_wiring_rgb.h"

#include <cstdio>
#include <cstdarg>

namespace particle {

namespace system {

namespace {

#if HAL_PLATFORM_RESUMABLE_OTA

// Name of the file storing the transfer state
const auto TRANSFER_STATE_FILE = "/sys/fw_transfer";

// Interval at which the transfer state file is synced
const system_tick_t TRANSFER_STATE_SYNC_INTERVAL = 2000;

// Minimum number of bytes that needs to be received before the transfer state file is synced
const size_t TRANSFER_STATE_SYNC_BYTES = 8192;

// The data stored in the OTA section is read in blocks of this size
const size_t OTA_FLASH_READ_BLOCK_SIZE = 128;

// The same buffer is used as a temporary storage for a SHA-256 hash
static_assert(OTA_FLASH_READ_BLOCK_SIZE >= Sha256::HASH_SIZE, "OTA_FLASH_READ_BLOCK_SIZE is too small");

struct PersistentTransferState {
    char fileHash[Sha256::HASH_SIZE]; // SHA-256 of the update binary
    char partialHash[Sha256::HASH_SIZE]; // SHA-256 of the partially transferred data
    uint32_t fileSize; // Size of the update binary
    uint32_t partialSize; // Size of the partially transferred data
} /* __attribute__((packed)) */;

#endif // HAL_PLATFORM_RESUMABLE_OTA

} // namespace

namespace detail {

#if HAL_PLATFORM_RESUMABLE_OTA

struct TransferState {
    SimpleFileStorage file; // File storing the transfer state
    Sha256 partialHash; // SHA-256 of the partially transferred data
    Sha256 tempHash; // Intermediate SHA-256 checksum
    PersistentTransferState persist; // Persistently stored transfer state
    system_tick_t lastSyncTime; // Time when the file was last synced
    size_t bytesToSync; // Number of bytes received since the file was last synced
    size_t startOffset; // Offset at which the transfer started

    TransferState() :
            file(TRANSFER_STATE_FILE),
            persist(),
            lastSyncTime(0),
            bytesToSync(0),
            startOffset(0) {
    }
};

#endif // HAL_PLATFORM_RESUMABLE_OTA

} // namespace detail

FirmwareUpdate::FirmwareUpdate() :
        updating_(false),
        ledOverridden_(false) {
}

int FirmwareUpdate::startUpdate(size_t fileSize, const char* fileHash, size_t* partialSize, FirmwareUpdateFlags flags) {
    const bool validateOnly = flags & FirmwareUpdateFlag::VALIDATE_ONLY;
#if HAL_PLATFORM_RESUMABLE_OTA
    const bool discardData = flags & FirmwareUpdateFlag::DISCARD_DATA;
    bool nonResumable = flags & FirmwareUpdateFlag::NON_RESUMABLE;
    if (!fileHash || !partialSize) {
        nonResumable = true;
    }
#endif
    if (updating_) {
        ERROR_MESSAGE("Firmware update is already in progress");
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!System.updatesEnabled() && !System.updatesForced()) {
        return SYSTEM_ERROR_OTA_UPDATES_DISABLED;
    }
    if (!fileSize || fileSize > HAL_OTA_FlashLength()) {
        return SYSTEM_ERROR_OTA_INVALID_SIZE;
    }
    size_t fileOffset = 0;
#if HAL_PLATFORM_RESUMABLE_OTA
    if ((discardData || nonResumable) && !validateOnly) {
        clearTransferState();
    }
    // Do not load the transfer state if both VALIDATE_ONLY and DISCARD_DATA are set. DISCARD_DATA
    // would have cleared it anyway if it wasn't a dry-run
    if (!nonResumable && !(validateOnly && discardData)) {
        const int r = initTransferState(fileSize, fileHash);
        if (r >= 0) {
            fileOffset = transferState_->persist.partialSize;
            if (validateOnly) {
                transferState_.reset();
            }
        } else {
            // Not a critical error
            LOG(ERROR, "Failed to initialize persistent transfer state: %d", r);
            if (!validateOnly) {
                clearTransferState();
            }
        }
    }
#endif // HAL_PLATFORM_RESUMABLE_OTA
    if (!validateOnly) {
        // Erase the OTA section if we're not resuming the previous transfer
        if (!fileOffset && !HAL_FLASH_Begin(HAL_OTA_FlashAddress(), fileSize, nullptr)) {
#if HAL_PLATFORM_RESUMABLE_OTA
            transferState_.reset();
#endif
            return SYSTEM_ERROR_FLASH_IO;
        }
        system_set_flag(SYSTEM_FLAG_OTA_UPDATE_PENDING, 0, nullptr);
        // TODO: Use the LED service for the update indication
        ledOverridden_ = LED_RGB_IsOverRidden();
        if (!ledOverridden_) {
            RGB.control(true);
            // Get base color used for the update indication
            const LEDStatusData* status = led_signal_status(LED_SIGNAL_FIRMWARE_UPDATE, nullptr);
            RGB.color(status ? status->color : RGB_COLOR_MAGENTA);
        }
        SPARK_FLASH_UPDATE = 1; // TODO: Get rid of legacy state variables
        updating_ = true;
        // Generate a system event
        fileDesc_ = FileTransfer::Descriptor();
        fileDesc_.file_length = fileSize;
        fileDesc_.file_address = HAL_OTA_FlashAddress();
        fileDesc_.chunk_size = HAL_OTA_ChunkSize();
        fileDesc_.chunk_address = fileDesc_.file_address;
        fileDesc_.store = FileTransfer::Store::FIRMWARE;
        system_notify_event(firmware_update, firmware_update_begin, &fileDesc_);
    }
    if (partialSize) {
        *partialSize = fileOffset;
    }
    return 0;
}

int FirmwareUpdate::finishUpdate(FirmwareUpdateFlags flags) {
    const bool validateOnly = flags & FirmwareUpdateFlag::VALIDATE_ONLY;
    const bool cancel = flags & FirmwareUpdateFlag::CANCEL;
#if HAL_PLATFORM_RESUMABLE_OTA
    const bool discardData = flags & FirmwareUpdateFlag::DISCARD_DATA;
#endif
    if (!cancel) {
        if (!updating_) {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        if (!validateOnly) {
            int r = 0;
#if HAL_PLATFORM_RESUMABLE_OTA
            if (transferState_) {
                r = finalizeTransferState();
            }
            if (r < 0 || discardData) {
                clearTransferState();
            }
#endif
            if (r >= 0) {
                // TODO: Cache the validation result so that it's not performed twice
                r = HAL_FLASH_End(nullptr /* reserved */);
            }
            endUpdate(r >= 0);
            if (r < 0) {
                return r;
            }
            system_pending_shutdown(RESET_REASON_UPDATE); // Always restart for now
        } else {
            CHECK(HAL_FLASH_OTA_Validate(true /* userDepsOptional */,
                    (module_validation_flags_t)(MODULE_VALIDATION_INTEGRITY | MODULE_VALIDATION_DEPENDENCIES_FULL),
                    nullptr /* reserved */));
        }
    } else if (updating_ && !validateOnly) {
#if HAL_PLATFORM_RESUMABLE_OTA
        if (discardData) {
            clearTransferState();
        }
#endif
        endUpdate(false /* ok */);
    }
    return 0;
}

int FirmwareUpdate::saveChunk(const char* chunkData, size_t chunkSize, size_t chunkOffset, size_t partialSize) {
    if (!updating_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    TimingFlashUpdateTimeout = 0; // TODO: Get rid of legacy state variables
    const uintptr_t addr = HAL_OTA_FlashAddress() + chunkOffset;
    int r = HAL_FLASH_Update((const uint8_t*)chunkData, addr, chunkSize, nullptr);
    if (r != 0) {
        ERROR_MESSAGE("Failed to save firmware data: %d", r);
        endUpdate(false /* ok */);
        return SYSTEM_ERROR_FLASH_IO;
    }
#if HAL_PLATFORM_RESUMABLE_OTA
    if (transferState_) {
        r = updateTransferState(chunkData, chunkSize, chunkOffset, partialSize);
        if (r != 0) {
            // Not a critical error
            LOG(ERROR, "Failed to update transfer state: %d", r);
            clearTransferState();
        }
    }
#endif
    if (!ledOverridden_) {
        LED_Toggle(LED_RGB);
    }
    // Generate a system event
    fileDesc_.chunk_address = fileDesc_.file_address + chunkOffset;
    fileDesc_.chunk_size = chunkSize;
    system_notify_event(firmware_update, firmware_update_progress, &fileDesc_);
    return 0;
}

void FirmwareUpdate::process() {
    if (!updating_) {
        return;
    }
#if HAL_PLATFORM_RESUMABLE_OTA
    if (transferState_) {
        const auto state = transferState_.get();
        if (state->bytesToSync >= TRANSFER_STATE_SYNC_BYTES &&
                HAL_Timer_Get_Milli_Seconds() - state->lastSyncTime >= TRANSFER_STATE_SYNC_INTERVAL) {
            const int r = state->file.sync();
            if (r >= 0) {
                state->lastSyncTime = HAL_Timer_Get_Milli_Seconds();
                state->bytesToSync = 0;
            } else {
                // Not a critical error
                LOG(ERROR, "Failed to update transfer state: %d", r);
                clearTransferState();
            }
        }
    }
#endif
}

FirmwareUpdate* FirmwareUpdate::instance() {
    static FirmwareUpdate instance;
    return &instance;
}

#if HAL_PLATFORM_RESUMABLE_OTA

int FirmwareUpdate::initTransferState(size_t fileSize, const char* fileHash) {
    std::unique_ptr<detail::TransferState> state(new(std::nothrow) detail::TransferState());
    if (!state) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    CHECK(state->partialHash.init());
    CHECK(state->tempHash.init());
    bool resumeTransfer = false;
    const auto persist = &state->persist;
    const int r = state->file.load(persist, sizeof(PersistentTransferState));
    if (r == sizeof(PersistentTransferState)) {
        if (persist->fileSize == fileSize && persist->partialSize <= fileSize &&
                memcmp(persist->fileHash, fileHash, Sha256::HASH_SIZE) == 0) {
            // Compute the hash of the partially transferred data stored in the OTA section
            CHECK(state->partialHash.start());
            char buf[OTA_FLASH_READ_BLOCK_SIZE] = {};
            uintptr_t addr = HAL_OTA_FlashAddress();
            const uintptr_t endAddr = addr + persist->partialSize;
            while (addr < endAddr) {
                const size_t n = std::min(endAddr - addr, sizeof(buf));
                CHECK(HAL_OTA_Flash_Read(addr, (uint8_t*)buf, n));
                CHECK(state->partialHash.update(buf, n));
                addr += n;
            }
            CHECK(state->tempHash.copyFrom(state->partialHash));
            CHECK(state->tempHash.finish(buf));
            if (memcmp(persist->partialHash, buf, Sha256::HASH_SIZE) == 0) {
                resumeTransfer = true;
            }
        }
    } else if (r < 0 && r != SYSTEM_ERROR_NOT_FOUND && r != SYSTEM_ERROR_BAD_DATA) {
        return r;
    }
    if (!resumeTransfer) {
        state->file.clear();
        memcpy(persist->fileHash, fileHash, Sha256::HASH_SIZE);
        memset(persist->partialHash, 0, Sha256::HASH_SIZE);
        persist->fileSize = fileSize;
        persist->partialSize = 0;
        CHECK(state->partialHash.start()); // Reset SHA-256 context
    }
    state->startOffset = persist->partialSize;
    transferState_ = std::move(state);
    return 0;
}

int FirmwareUpdate::updateTransferState(const char* chunkData, size_t chunkSize, size_t chunkOffset, size_t partialSize) {
    const auto state = transferState_.get();
    if (!state) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    const auto persist = &state->persist;
    const size_t partialSizeBefore = persist->partialSize;
    // Check if the chunk is adjacent to or overlaps with the contiguous fragment of the data for
    // which we have already calculated the checksum
    if (persist->partialSize >= chunkOffset && persist->partialSize < chunkOffset + chunkSize) {
        const auto n = chunkOffset + chunkSize - persist->partialSize;
        CHECK(state->partialHash.update(chunkData + persist->partialSize - chunkOffset, n));
        persist->partialSize += n;
    }
    // Chunks are not necessarily transferred sequentially. We may need to read them back from the
    // OTA section to calculate the checksum of the data transferred so far
    if (partialSize > persist->partialSize) {
        char buf[OTA_FLASH_READ_BLOCK_SIZE] = {};
        uintptr_t addr = HAL_OTA_FlashAddress() + persist->partialSize;
        const uintptr_t endAddr = addr + partialSize - persist->partialSize;
        while (addr < endAddr) {
            const size_t n = std::min(endAddr - addr, sizeof(buf));
            CHECK(HAL_OTA_Flash_Read(addr, (uint8_t*)buf, n));
            CHECK(state->partialHash.update(buf, n));
            addr += n;
        }
        persist->partialSize = partialSize;
    }
    if (persist->partialSize > partialSizeBefore) {
        CHECK(state->tempHash.copyFrom(state->partialHash));
        CHECK(state->tempHash.finish(persist->partialHash));
        CHECK(state->file.save(persist, sizeof(PersistentTransferState)));
        // Avoid syncing the file on the very first chunk received
        if (partialSizeBefore == state->startOffset) {
            state->lastSyncTime = HAL_Timer_Get_Milli_Seconds();
        }
        state->bytesToSync += persist->partialSize - partialSizeBefore;
    }
    if (state->bytesToSync >= TRANSFER_STATE_SYNC_BYTES &&
            HAL_Timer_Get_Milli_Seconds() - state->lastSyncTime >= TRANSFER_STATE_SYNC_INTERVAL) {
        CHECK(state->file.sync());
        state->lastSyncTime = HAL_Timer_Get_Milli_Seconds();
        state->bytesToSync = 0;
    }
    return 0;
}

int FirmwareUpdate::finalizeTransferState() {
    const auto state = transferState_.get();
    if (!state) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    const auto persist = &state->persist;
    if (persist->partialSize != persist->fileSize) {
        return SYSTEM_ERROR_OTA_INVALID_SIZE;
    }
    if (memcmp(persist->partialHash, persist->fileHash, Sha256::HASH_SIZE) != 0) {
        ERROR_MESSAGE("Integrity check of a resumed update has failed");
        return SYSTEM_ERROR_OTA_RESUMED_UPDATE_FAILED;
    }
    state->file.close();
    transferState_.reset();
    return 0;
}

void FirmwareUpdate::clearTransferState() {
    if (transferState_) {
        transferState_->file.clear();
        transferState_.reset();
    } else {
        SimpleFileStorage::clear(TRANSFER_STATE_FILE);
    }
}

#endif // HAL_PLATFORM_RESUMABLE_OTA

void FirmwareUpdate::endUpdate(bool ok) {
    if (!updating_) {
        return;
    }
#if HAL_PLATFORM_RESUMABLE_OTA
    transferState_.reset();
#endif
    if (!ledOverridden_) {
        RGB.control(false);
    }
    SPARK_FLASH_UPDATE = 0;
    updating_ = false;
    system_notify_event(firmware_update, ok ? firmware_update_complete : firmware_update_failed, &fileDesc_);
}

} // namespace system

} // namespace particle
