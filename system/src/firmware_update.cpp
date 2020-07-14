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

LOG_SOURCE_CATEGORY("sys.ota");

#include "firmware_update.h"

#include "util/simple_file_storage.h"

#include "ota_flash_hal.h"
#include "core_hal.h"
#include "timer_hal.h"

#if HAL_PLATFORM_RESUMABLE_OTA
#include "sha256.h"
#endif // HAL_PLATFORM_RESUMABLE_OTA

#include "scope_guard.h"
#include "check.h"
#include "debug.h"

#include <cstdio>
#include <cstdarg>

namespace particle::system {

namespace {

#if HAL_PLATFORM_RESUMABLE_OTA

const auto TRANSFER_STATE_FILE = "/sys/fw_transfer";
const system_tick_t TRANSFER_STATE_SYNC_INTERVAL = 1000;

struct PersistentTransferState {
    char fileHash[Sha256::HASH_SIZE]; // SHA-256 of the update binary
    char partialHash[Sha256::HASH_SIZE]; // SHA-256 of the partially transferred data
    uint32_t fileSize; // Size of the update binary
    uint32_t partialSize; // Size of the partially transferred data
    uint32_t stateCrc32; // CRC-32 of the above structure fields
} __attribute__((packed));

inline uint32_t peristentStateCrc32(const PersistentTransferState& state) {
    return HAL_Core_Compute_CRC32((const uint8_t*)&state, offsetof(PersistentTransferState, stateCrc32));
}

#endif // HAL_PLATFORM_RESUMABLE_OTA

} // namespace

namespace detail {

#if HAL_PLATFORM_RESUMABLE_OTA

struct TransferState {
    SimpleFileStorage file; // File storing the transfer state
    Sha256 partialHash; // SHA-256 of the partially transferred data
    Sha256 tempHash; // Intermediate SHA-256 checksum
    char fileHash[Sha256::HASH_SIZE]; // SHA-256 of the update binary
    system_tick_t lastSynced; // Time when the file was last synced
    size_t fileSize; // Size of the update binary
    size_t partialSize; // Size of the partially transferred data
    bool needSync;

    TransferState() :
            file(TRANSFER_STATE_FILE),
            lastSynced(0),
            fileSize(0),
            partialSize(0),
            needSync(false) {
    }
};

#endif // HAL_PLATFORM_RESUMABLE_OTA

} // namespace detail

FirmwareUpdate::FirmwareUpdate() :
        validResult_(0),
        validChecked_(false),
        updating_(false) {
}

int FirmwareUpdate::startUpdate(size_t fileSize, const char* fileHash, size_t* fileOffset, FirmwareUpdateFlags flags) {
    if (!(flags & FirmwareUpdateFlag::NON_RESUMABLE) && (!fileHash || !fileOffset)) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (updating_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    return 0;
}

int FirmwareUpdate::validateUpdate() {
    return 0;
}

int FirmwareUpdate::finishUpdate(FirmwareUpdateFlags flags) {
    return 0;
}

int FirmwareUpdate::saveChunk(const char* chunkData, size_t chunkSize, size_t chunkOffset, size_t partialSize) {
    return 0;
}

bool FirmwareUpdate::isInProgress() const {
    return false;
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
    PersistentTransferState persist = {};
    const int r = state->file.load(&persist, sizeof(persist));
    if (r == sizeof(persist)) {
        const uint32_t crc = peristentStateCrc32(persist);
        if (persist.stateCrc32 == crc && persist.fileSize == fileSize &&
                memcmp(persist.fileHash, fileHash, Sha256::HASH_SIZE) == 0) {
            // Compute hash of the partially transferred firmware data in the OTA section
            CHECK(state->partialHash.start());
            char buf[128] = {};
            uintptr_t addr = HAL_OTA_FlashAddress();
            const uintptr_t endAddr = addr + persist.partialSize;
            while (addr < endAddr) {
                const size_t n = std::min(endAddr - addr, sizeof(buf));
                CHECK(HAL_OTA_Flash_Read(addr, (uint8_t*)buf, n));
                CHECK(state->partialHash.update(buf, n));
                addr += n;
            }
            CHECK(state->tempHash.copyFrom(state->partialHash));
            static_assert(sizeof(buf) >= Sha256::HASH_SIZE, "");
            CHECK(state->tempHash.finish(buf));
            if (memcmp(persist.partialHash, buf, Sha256::HASH_SIZE) == 0) {
                resumeTransfer = true;
            }
        }
    } else if (r < 0 && r != SYSTEM_ERROR_NOT_FOUND) {
        return r;
    }
    if (resumeTransfer) {
        state->file.close(); // Will be reopened for writing
        state->partialSize = persist.partialSize;
    } else {
        state->file.clear();
        CHECK(state->partialHash.start()); // Reset SHA-256 context
    }
    memcpy(state->fileHash, fileHash, Sha256::HASH_SIZE);
    state->fileSize = fileSize;
    transferState_ = std::move(state);
    return 0;
}

int FirmwareUpdate::updateTransferState(const char* chunkData, size_t chunkSize, size_t chunkOffset, size_t partialSize) {
    const auto state = transferState_.get();
    if (!state) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    bool updateState = false;
    if (chunkOffset == state->partialSize) {
        CHECK(state->partialHash.update(chunkData, chunkSize));
        state->partialSize += chunkSize;
        updateState = true;
    }
    if (partialSize > state->partialSize) {
        char buf[128] = {};
        uintptr_t addr = HAL_OTA_FlashAddress() + state->partialSize;
        const uintptr_t endAddr = addr + partialSize - state->partialSize;
        while (addr < endAddr) {
            const size_t n = std::min(endAddr - addr, sizeof(buf));
            CHECK(HAL_OTA_Flash_Read(addr, (uint8_t*)buf, n));
            CHECK(state->partialHash.update(buf, n));
            addr += n;
        }
        state->partialSize = partialSize;
        updateState = true;
    }
    if (updateState) {
        PersistentTransferState persist = {};
        CHECK(state->tempHash.copyFrom(state->partialHash));
        CHECK(state->tempHash.finish(persist.partialHash));
        memcpy(persist.fileHash, state->fileHash, Sha256::HASH_SIZE);
        persist.fileSize = state->fileSize;
        persist.partialSize = state->partialSize;
        persist.stateCrc32 = peristentStateCrc32(persist);
        CHECK(state->file.save(&persist, sizeof(persist)));
        state->needSync = true;
    }
    if (state->needSync && HAL_Timer_Get_Milli_Seconds() - state->lastSynced >= TRANSFER_STATE_SYNC_INTERVAL) {
        CHECK(state->file.sync());
        state->lastSynced = HAL_Timer_Get_Milli_Seconds();
    }
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

void FirmwareUpdate::errorMessage(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    va_list args2;
    va_copy(args2, args);
    SCOPE_GUARD({
        va_end(args2);
        va_end(args);
    });
    const auto n = vsnprintf(nullptr, 0, fmt, args);
    if (n < 0) {
        return;
    }
    const auto buf = (char*)malloc(n + 1);
    if (!buf) {
        return;
    }
    vsnprintf(buf, n + 1, fmt, args2);
    errMsg_ = CString::wrap(buf);
}

} // namespace particle::system
