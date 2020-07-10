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

#pragma once

#include "hal_platform.h"

#include "system_defs.h"
#include "c_string.h"

#if HAL_PLATFORM_RESUMABLE_OTA
#include "sha256.h"
#endif

#include <memory>

namespace particle::system {

/**
 * Firmware update handler.
 */
class FirmwareUpdate {
public:
    ~FirmwareUpdate();
    /**
     * Start a firmware update.
     *
     * @param fileSize Size of the update binary.
     * @param fileHash SHA-256 checksum of the update binary. This argument can be set to null if
     *        the update is non-resumable (see `FirmwareUpdateFlag::NON_RESUMABLE`).
     * @param fileOffset[out] Offset starting from which the transfer of the update binary should
     *        be resumed. This argument can be set to null if the update is non-resumable.
     * @param flags Update flags.
     * @return 0 on success or a negative result code in case of an error.
     */
    int startUpdate(size_t fileSize, const char* fileHash, size_t* fileOffset, FirmwareUpdateFlags flags);
    /**
     * Validate the firmware update.
     *
     * @return 0 on success or a negative result code in case of an error.
     */
    int validateUpdate();
    /**
     * Finish the firmware update.
     *
     * @param flags Update flags.
     * @return 0 on success or a negative result code in case of an error.
     */
    int finishUpdate(FirmwareUpdateFlags flags);
    /**
     * Save a chunk of the update binary.
     *
     * @param chunkData Chunk data.
     * @param chunkSize Chunk size.
     * @param chunkOffset Offset of the chunk in the file.
     * @param partialSize Size of the fully transferred contiguous fragment of the file that starts at
     *        the beginning of the file. This argument can be set to 0 if the update is non-resumable
     *        (see `FirmwareUpdateFlag::NON_RESUMABLE`).
     * @return 0 on success or a negative result code in case of an error.
     */
    int saveChunk(const char* chunkData, size_t chunkSize, size_t chunkOffset, size_t partialSize);
    /**
     * Get the firmware update status.
     *
     * @return `true` if a firmware update is in progress.
     */
    bool isInProgress() const;
    /**
     * Get the last error message.
     *
     * This method clears the error message stored in the handler.
     *
     * @return Error message.
     */
    CString takeErrorMessage();
    /**
     * Reset the handler state and free all allocated resources.
     */
    void reset();
    /**
     * Get the singleton instance of this class.
     */
    static FirmwareUpdate* instance();

private:
    CString errMsg_; // Last error message
    int validResult_; // Result of the update validation
    bool validChecked_; // Whether the update has been validated
    bool updating_; // Whether an update is in progress

#if HAL_PLATFORM_RESUMABLE_OTA
    // Persistently stored state of a partially completed file transfer
    struct PersistentTransferState {
        char completeSha256[Sha256::HASH_SIZE]; // SHA-256 of the update binary
        char partialSha256[Sha256::HASH_SIZE]; // SHA-256 of the partially transferred data
        uint32_t completeSize; // Size of the update binary
        uint32_t partialSize; // Size of the partially transferred data
        uint32_t stateCrc32; // CRC-32 of the above structure fields
    } __attribute__((packed));

    // File transfer state
    struct TransferState {
        Sha256 hash; // SHA-256 of the partially transferred data
        Sha256 tempHash; // Intermediate SHA-256 checksum
        PersistentTransferState persist; // Persistently stored state
        size_t size; // Size of the update binary
        size_t offset; // Size of the partially transferred data
    };

    std::unique_ptr<TransferState> transfer_;
#endif // HAL_PLATFORM_RESUMABLE_OTA

    FirmwareUpdate();

    int init();
    void destroy();

    void errorMessage(const char* fmt, ...);
};

inline CString FirmwareUpdate::takeErrorMessage() {
    return std::move(errMsg_);
}

} // namespace particle::system
