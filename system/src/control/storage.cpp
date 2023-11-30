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
#include "system_network.h"
#include "common.h"

#include "ota_flash_hal_impl.h"

#include "delay_hal.h"

#include "protocol_defs.h" // For UpdateFlag enum
#include "nanopb_misc.h"
#include "scope_guard.h"
#include "check.h"

#include "platforms.h"

#include "control/storage.pb.h"

#include <memory>
#include "system_info_encoding.h"

#define PB(_name) particle_ctrl_##_name

namespace particle {

namespace control {

using namespace protocol;
using namespace common;

namespace {

// TODO: Move handling of compressed firmware binaries to the common system code
struct FirmwareUpdate {
    FileTransfer::Descriptor descr; // File transfer descriptor
    size_t bytesLeft; // Number of remaining bytes to receive
    size_t bytesWritten; // Number of bytes written to the OTA section
};

std::unique_ptr<FirmwareUpdate> g_update;

void cancelFirmwareUpdate() {
    if (!g_update) {
        return;
    }
    const int ret = Spark_Finish_Firmware_Update(g_update->descr, UpdateFlag::ERROR, nullptr);
    if (ret < 0) {
        LOG(WARN, "Spark_Finish_Firmware_Update() failed: %d", ret);
    }
    g_update.reset();
}

void firmwareUpdateCompletionHandler(int result, void* data) {
    if (!g_update) {
        return;
    }
    const int ret = Spark_Finish_Firmware_Update(g_update->descr, UpdateFlag::SUCCESS, nullptr);
    if (ret < 0) {
        LOG(ERROR, "Spark_Finish_Firmware_Update() failed: %d", ret);
    }
    g_update.reset();
}

} // namespace

int startFirmwareUpdateRequest(ctrl_request* req) {
    PB(StartFirmwareUpdateRequest) pbReq = {};
    CHECK(decodeRequestMessage(req, PB(StartFirmwareUpdateRequest_fields), &pbReq));
    cancelFirmwareUpdate(); // Cancel current transfer
    std::unique_ptr<FirmwareUpdate> update(new(std::nothrow) FirmwareUpdate);
    CHECK_TRUE(update, SYSTEM_ERROR_NO_MEMORY);
    if (pbReq.format == PB(FileFormat_BIN)) {
        update->descr.file_length = pbReq.size;
    } else {
        LOG(ERROR, "Unknown binary format: %u", (unsigned)pbReq.format);
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    update->descr.store = FileTransfer::Store::FIRMWARE;
    update->descr.chunk_size = 1024; // TODO: Determine depending on free RAM?
    update->descr.chunk_address = 0;
    update->descr.file_address = 0;
    int ret = Spark_Prepare_For_Firmware_Update(update->descr, 0, nullptr);
    if (ret != 0) {
        LOG(ERROR, "Spark_Prepare_For_Firmware_Update() failed: %d", ret);
        return ret;
    }
    update->descr.chunk_address = update->descr.file_address;
    update->bytesLeft = pbReq.size;
    update->bytesWritten = 0;
    g_update = std::move(update);
    PB(StartFirmwareUpdateReply) pbRep = {};
    pbRep.chunk_size = g_update->descr.chunk_size;
    ret = encodeReplyMessage(req, PB(StartFirmwareUpdateReply_fields), &pbRep);
    if (ret != 0) {
        cancelFirmwareUpdate();
        return ret;
    }
    return 0;
}

void finishFirmwareUpdateRequest(ctrl_request* req) {
    PB(FinishFirmwareUpdateRequest) pbReq = {};
    int ret = decodeRequestMessage(req, PB(FinishFirmwareUpdateRequest_fields), &pbReq);
    if (ret != 0) {
        goto done;
    }
    if (!g_update || g_update->bytesLeft > 0) {
        ret = SYSTEM_ERROR_INVALID_STATE;
        goto done;
    }
    LOG_DEBUG(TRACE, "Firmware size: %u", (unsigned)g_update->bytesWritten);
    // Validate the update
    ret = Spark_Finish_Firmware_Update(g_update->descr, UpdateFlag::SUCCESS | UpdateFlag::VALIDATE_ONLY, nullptr);
    if (ret >= 0 && !pbReq.validate_only) {
        // Reply to the host and apply the update
        system_ctrl_set_result(req, 0 /* result */, firmwareUpdateCompletionHandler, nullptr, nullptr);
        return;
    }
done:
    cancelFirmwareUpdate();
    system_ctrl_set_result(req, ret, nullptr, nullptr, nullptr);
}

int cancelFirmwareUpdateRequest(ctrl_request* req) {
    cancelFirmwareUpdate();
    return 0;
}

int firmwareUpdateDataRequest(ctrl_request* req) {
    NAMED_SCOPE_GUARD(guard, {
        cancelFirmwareUpdate();
    });
    PB(FirmwareUpdateDataRequest) pbReq = {};
    DecodedString pbData(&pbReq.data);
    CHECK(decodeRequestMessage(req, PB(FirmwareUpdateDataRequest_fields), &pbReq));
    if (!g_update) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (pbData.size == 0 || pbData.size > g_update->bytesLeft) {
        return SYSTEM_ERROR_OUT_OF_RANGE;
    }

    g_update->descr.chunk_size = pbData.size;
    const int ret = Spark_Save_Firmware_Chunk(g_update->descr, (const uint8_t*)pbData.data, nullptr);
    if (ret != 0) {
        return ret;
    }
    g_update->descr.chunk_address += pbData.size;
    g_update->bytesLeft -= pbData.size;

    guard.dismiss();
    return 0;
}

int getModuleInfo(ctrl_request* req) {
    PB(GetModuleInfoReply) pbRep = {};
    using namespace particle::system;
    EncodeFirmwareModules modules(&pbRep.modules, EncodeFirmwareModules::Flag::FORCE_SHA);
    CHECK(encodeReplyMessage(req, PB(GetModuleInfoReply_fields), &pbRep));
    return 0;
}

#if HAL_PLATFORM_ASSETS
int getAssetInfo(ctrl_request* req) {
    PB(GetAssetInfoReply) pbRep = {};
    using namespace particle::system;
    EncodeAssets assetsAvailable(&pbRep.available, AssetManager::instance().availableAssets());
    EncodeAssets assetsRequired(&pbRep.required, AssetManager::instance().requiredAssets());
    CHECK(encodeReplyMessage(req, PB(GetAssetInfoReply_fields), &pbRep));
    return 0;
}
#endif // HAL_PLATFORM_ASSETS

} // namespace particle::control

} // namespace particle

#endif // SYSTEM_CONTROL_ENABLED
