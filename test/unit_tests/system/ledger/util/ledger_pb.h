/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

#include "system_ledger.h"

#include <string>
#include <vector>
#include <cstdint>

// Helpers for encoding and decoding the ledger protocol messages (proto_defs/shared/cloud/ledger.proto)
// on the "cloud" side of the tests

namespace particle::test {

// Request sent by the device (particle.cloud.Request)
struct DecodedRequest {
    int type = 0;
    std::vector<std::string> ledgerNames; // GET_INFO, SUBSCRIBE, SET_DATA and GET_DATA requests
    std::string data; // SET_DATA requests
};

// Response sent by the device (particle.cloud.Response)
struct DecodedResponse {
    int result = 0;
    std::string message;
};

struct LedgerInfoEntry {
    std::string name;
    ledger_sync_direction syncDir;
    ledger_scope scopeType = LEDGER_SCOPE_DEVICE;
    std::string scopeId = "scope";
};

struct LedgerUpdateEntry {
    std::string name;
    uint64_t lastUpdated = 0; // 0 means not set
};

DecodedRequest decodeRequest(const std::string& payload);
DecodedResponse decodeResponse(const std::string& payload);

// Responses sent by the cloud (particle.cloud.Response)
std::string encodeResponse(int result);
std::string encodeGetInfoResponse(const std::vector<LedgerInfoEntry>& ledgers);
std::string encodeSubscribeResponse(const std::vector<LedgerUpdateEntry>& ledgers);
std::string encodeGetDataResponse(uint64_t lastUpdated, const std::string& data);

// Requests sent by the cloud (particle.cloud.Request)
std::string encodeRequest(int type);
std::string encodeNotifyUpdateRequest(const std::vector<LedgerUpdateEntry>& ledgers);

} // namespace particle::test
