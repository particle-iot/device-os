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

#include "update.h"

#if SYSTEM_CONTROL_ENABLED

#include "system_update.h"
#include "common.h"

#include "protocol_defs.h"

#include "update.pb.h"

#include <memory>

namespace particle {

namespace control {

using namespace protocol;
using namespace common;

namespace {

std::unique_ptr<FileTransfer::Descriptor> g_desc;

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

} // namespace

int prepareFirmwareUpdateRequest(ctrl_request* req) {
    particle_ctrl_PrepareFirmwareUpdateRequest pbReq = {};
    int ret = decodeRequestMessage(req, particle_ctrl_PrepareFirmwareUpdateRequest_fields, &pbReq);
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
    g_desc->chunk_size = 4096; // TODO: Make configurable
    g_desc->file_address = 0;
    g_desc->chunk_address = 0;
    ret = Spark_Prepare_For_Firmware_Update(*g_desc, 0, nullptr);
    if (ret != 0) {
        g_desc.reset();
        return ret;
    }
    g_desc->chunk_address = g_desc->file_address;
    particle_ctrl_PrepareFirmwareUpdateReply pbRep = {};
    pbRep.chunk_size = g_desc->chunk_size;
    ret = encodeReplyMessage(req, particle_ctrl_PrepareFirmwareUpdateReply_fields, &pbRep);
    if (ret != 0) {
        cancelFirmwareUpdate();
        return ret;
    }
    return 0;
}

int finishFirmwareUpdateRequest(ctrl_request* req) {
    particle_ctrl_FinishFirmwareUpdateRequest pbReq = {};
    int ret = decodeRequestMessage(req, particle_ctrl_FinishFirmwareUpdateRequest_fields, &pbReq);
    if (ret != 0) {
        return ret;
    }
    if (!g_desc) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (g_desc->chunk_address != g_desc->file_address + g_desc->file_length) {
        cancelFirmwareUpdate();
        return SYSTEM_ERROR_INVALID_STATE;
    }
    unsigned flags = UpdateFlag::SUCCESS;
    if (pbReq.validate_only) {
        flags |= UpdateFlag::VALIDATE_ONLY;
    }
    ret = Spark_Finish_Firmware_Update(*g_desc, flags, nullptr);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

int cancelFirmwareUpdateRequest(ctrl_request* req) {
    return cancelFirmwareUpdate();
}

int saveFirmwareChunkRequest(ctrl_request* req) {
    particle_ctrl_SaveFirmwareChunkRequest pbReq = {};
    DecodedString pbData(&pbReq.data);
    int ret = decodeRequestMessage(req, particle_ctrl_SaveFirmwareChunkRequest_fields, &pbReq);
    if (ret != 0) {
        return ret;
    }
    if (!g_desc) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (pbData.size == 0 || g_desc->chunk_address + pbData.size > g_desc->file_address + g_desc->file_length) {
        cancelFirmwareUpdate();
        return SYSTEM_ERROR_INVALID_STATE;
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

} // namespace particle::control

} // namespace particle

#endif // SYSTEM_CONTROL_ENABLED
