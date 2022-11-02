/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
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

#undef LOG_COMPILE_TIME_LEVEL

#include <memory>

#include "server_key_manager.h"
#include "ota_flash_hal.h"
#include "core_hal.h"
#include "dct.h"
#include "parse_server_address.h"
#include "system_error.h"
#include "logging.h"
#include "check.h"

namespace particle {

namespace {

// EC P-256 public key
const uint8_t FACTORY_SERVER_PUBLIC_KEY[] = {
    0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02,
    0x01, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03,
    0x42, 0x00, 0x04, 0x50, 0x9b, 0xfc, 0x18, 0x56, 0x48, 0xc3, 0x3f, 0x80,
    0x90, 0x7a, 0xe1, 0x32, 0x60, 0xdf, 0x33, 0x28, 0x21, 0x15, 0x20, 0x9e,
    0x54, 0xa2, 0x2f, 0x2b, 0x10, 0x59, 0x84, 0xa4, 0x63, 0x62, 0xc0, 0x7c,
    0x26, 0x79, 0xf6, 0xe4, 0xce, 0x76, 0xca, 0x00, 0x2d, 0x3d, 0xe4, 0xbf,
    0x2e, 0x9e, 0x3a, 0x62, 0x15, 0x1c, 0x48, 0x17, 0x9b, 0xd8, 0x09, 0xdd,
    0xce, 0x9c, 0x5d, 0xc3, 0x0f, 0x54, 0xb8
};

const auto FACTORY_SERVER_ADDRESS = "$id.udp.particle.io";

const uint16_t FACTORY_SERVER_PORT = 5684;

int setServerPublicKey(const uint8_t* key, size_t size) {
    LOG(INFO, "Updating server public key:");
    LOG_DUMP(INFO, key, size);
    LOG_PRINTF(INFO, " (%u bytes)\r\n", (unsigned)size);
    if (size > DCT_ALT_SERVER_PUBLIC_KEY_SIZE) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    // FIXME: HAL_FLASH_Write_ServerPublicKey() expects a fixed-size buffer
    std::unique_ptr<uint8_t[]> buf(new(std::nothrow) uint8_t[DCT_ALT_SERVER_PUBLIC_KEY_SIZE]);
    if (!buf) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    memcpy(buf.get(), key, size);
    memset(buf.get() + size, 0xff, DCT_ALT_SERVER_PUBLIC_KEY_SIZE - size);
    HAL_FLASH_Write_ServerPublicKey(buf.get(), true /* udp */);
    return 0;
}

int setServerAddress(const char* address, uint16_t port) {
    LOG(INFO, "Updating server address: %s:%u", address, (unsigned)port);
    ServerAddress addr = {};
    CHECK(parseServerAddressString(&addr, address));
    addr.port = port;
    // FIXME: HAL_FLASH_Write_ServerAddress() expects a fixed-size buffer
    std::unique_ptr<uint8_t[]> buf(new(std::nothrow) uint8_t[DCT_ALT_SERVER_PUBLIC_KEY_SIZE]);
    if (!buf) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    memset(buf.get(), 0xff, DCT_ALT_SERVER_PUBLIC_KEY_SIZE);
    CHECK(encodeServerAddressData(&addr, buf.get(), DCT_ALT_SERVER_PUBLIC_KEY_SIZE));
    HAL_FLASH_Write_ServerAddress(buf.get(), true /* udp */);
    return 0;
}

inline bool usingUdp() {
    return HAL_Feature_Get(FEATURE_CLOUD_UDP);
}

} // namespace

int ServerKeyManager::validateServerMovedRequestSignature(const ServerMovedRequest& req) {
    return 0; // TODO
}

int ServerKeyManager::applyServerMovedRequestSettings(const ServerMovedRequest& req) {
    if (!usingUdp()) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    CHECK(setServerPublicKey(req.publicKey, req.publicKeySize));
    CHECK(setServerAddress(req.address, req.port));
    return 0;
}

int ServerKeyManager::restoreFactoryDefaults() {
    if (!usingUdp()) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    CHECK(setServerPublicKey(FACTORY_SERVER_PUBLIC_KEY, sizeof(FACTORY_SERVER_PUBLIC_KEY)));
    CHECK(setServerAddress(FACTORY_SERVER_ADDRESS, FACTORY_SERVER_PORT));
    return 0;
}

ServerKeyManager* ServerKeyManager::instance() {
    static ServerKeyManager mgr;
    return &mgr;
}

} // namespace particle
