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

#include "file_transfer.h"
#include "system_defs.h"

#include <memory>

namespace particle::system {

namespace detail {

#if HAL_PLATFORM_RESUMABLE_OTA
struct TransferState;
#endif

} // namespace detail

/**
 * Firmware update handler.
 */
class FirmwareUpdate {
public:
    /**
     * Start a firmware update.
     *
     * @param fileSize Size of the update binary.
     * @param fileHash SHA-256 checksum of the update binary. This argument can be set to null if
     *        the update is non-resumable (see `FirmwareUpdateFlag::NON_RESUMABLE`).
     * @param partialSize[out] Offset starting from which the transfer of the update binary should
     *        be resumed. This argument can be set to null if the update is non-resumable.
     * @param flags Update flags.
     * @return 0 on success or a negative result code in case of an error.
     */
    int startUpdate(size_t fileSize, const char* fileHash, size_t* partialSize, FirmwareUpdateFlags flags);
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
    bool isRunning() const;
    /**
     * Get the singleton instance of this class.
     */
    static FirmwareUpdate* instance();

private:
    FileTransfer::Descriptor fileDesc_; // File descriptor (used for compatibility with legacy system events)
    bool updating_; // Whether an update is in progress
    bool ledOverridden_; // FIXME

#if HAL_PLATFORM_RESUMABLE_OTA
    std::unique_ptr<detail::TransferState> transferState_; // Transfer state

    int initTransferState(size_t fileSize, const char* fileHash);
    int updateTransferState(const char* chunkData, size_t chunkSize, size_t chunkOffset, size_t partialSize);
    int finalizeTransferState();
    void clearTransferState();
#endif

    FirmwareUpdate();

    void endUpdate(bool ok);
};

inline bool FirmwareUpdate::isRunning() const {
    return updating_;
}

} // namespace particle::system
