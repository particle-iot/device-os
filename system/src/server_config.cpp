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

#include "hal_platform.h"

#if (PLATFORM_ID != PLATFORM_GCC && PLATFORM_ID != PLATFORM_NEWHAL) || defined(UNIT_TEST)

#include <memory>
#include <cstdint>

#include <mbedtls/pk.h>
#include <mbedtls/asn1.h>

#include "server_config.h"
#include "ota_flash_hal.h" // For ServerAddress
#include "core_hal.h"
#include "dct.h"
#include "sha256.h"
#include "mbedtls_util.h"
#include "parse_server_address.h"
#include "system_error.h"
#include "endian_util.h"
#include "logging.h"
#include "scope_guard.h"
#include "check.h"

namespace particle {

namespace {

// EC public key in DER format
const uint8_t DEFAULT_UDP_SERVER_PUBLIC_KEY[] = {
    0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02,
    0x01, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03,
    0x42, 0x00, 0x04, 0x50, 0x9b, 0xfc, 0x18, 0x56, 0x48, 0xc3, 0x3f, 0x80,
    0x90, 0x7a, 0xe1, 0x32, 0x60, 0xdf, 0x33, 0x28, 0x21, 0x15, 0x20, 0x9e,
    0x54, 0xa2, 0x2f, 0x2b, 0x10, 0x59, 0x84, 0xa4, 0x63, 0x62, 0xc0, 0x7c,
    0x26, 0x79, 0xf6, 0xe4, 0xce, 0x76, 0xca, 0x00, 0x2d, 0x3d, 0xe4, 0xbf,
    0x2e, 0x9e, 0x3a, 0x62, 0x15, 0x1c, 0x48, 0x17, 0x9b, 0xd8, 0x09, 0xdd,
    0xce, 0x9c, 0x5d, 0xc3, 0x0f, 0x54, 0xb8
};

// RSA public key in DER format
const uint8_t DEFAULT_TCP_SERVER_PUBLIC_KEY[] = {
    0x30, 0x82, 0x01, 0x22, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86,
    0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 0x0f, 0x00,
    0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01, 0x00, 0xbe, 0xcc, 0xbe,
    0x43, 0xdb, 0x8e, 0xea, 0x15, 0x27, 0xa6, 0xbb, 0x52, 0x6d, 0xe1, 0x51,
    0x2b, 0xa0, 0xab, 0xcc, 0xa1, 0x64, 0x77, 0x48, 0xad, 0x7c, 0x66, 0xfc,
    0x80, 0x7f, 0xf6, 0x99, 0xa5, 0x25, 0xf2, 0xf2, 0xda, 0xe0, 0x43, 0xcf,
    0x3a, 0x26, 0xa4, 0x9b, 0xa1, 0x87, 0x03, 0x0e, 0x9a, 0x8d, 0x23, 0x9a,
    0xbc, 0xea, 0x99, 0xea, 0x68, 0xd3, 0x5a, 0x14, 0xb1, 0x26, 0x0f, 0xbd,
    0xaa, 0x6d, 0x6f, 0x0c, 0xac, 0xc4, 0x77, 0x2c, 0xd1, 0xc5, 0xc8, 0xb1,
    0xd1, 0x7b, 0x68, 0xe0, 0x25, 0x73, 0x7b, 0x52, 0x89, 0x68, 0x20, 0xbd,
    0x06, 0xc6, 0xf0, 0xe6, 0x00, 0x30, 0xc0, 0xe0, 0xcf, 0xf6, 0x1b, 0x3a,
    0x45, 0xe9, 0xc4, 0x5b, 0x55, 0x17, 0x06, 0xa3, 0xd3, 0x4a, 0xc6, 0xd5,
    0xb8, 0xd2, 0x17, 0x02, 0xb5, 0x27, 0x7d, 0x8d, 0xe4, 0xd4, 0x7d, 0xd3,
    0xed, 0xc0, 0x1d, 0x8a, 0x7c, 0x25, 0x1e, 0x21, 0x4a, 0x51, 0xae, 0x57,
    0x06, 0xdd, 0x60, 0xbc, 0xa1, 0x34, 0x90, 0xaa, 0xcc, 0x09, 0x9e, 0x3b,
    0x3a, 0x41, 0x4c, 0x3c, 0x9d, 0xf3, 0xfd, 0xfd, 0xb7, 0x27, 0xc1, 0x59,
    0x81, 0x98, 0x54, 0x60, 0x4a, 0x62, 0x7a, 0xa4, 0x9a, 0xbf, 0xdf, 0x92,
    0x1b, 0x3e, 0xfc, 0xa7, 0xe4, 0xa4, 0xb3, 0x3a, 0x9a, 0x5f, 0x57, 0x93,
    0x8e, 0xeb, 0x19, 0x64, 0x95, 0x22, 0x4a, 0x2c, 0xd5, 0x60, 0xf5, 0xf9,
    0xd0, 0x03, 0x50, 0x83, 0x69, 0xc0, 0x6b, 0x53, 0xf0, 0xf0, 0xda, 0xf8,
    0x13, 0x82, 0x1f, 0xcc, 0xbb, 0x5f, 0xe2, 0xc1, 0xdf, 0x3a, 0xe9, 0x7f,
    0x5d, 0xe2, 0x7d, 0xb9, 0x50, 0x80, 0x3c, 0x58, 0x33, 0xef, 0x8c, 0xf3,
    0x80, 0x3f, 0x11, 0x01, 0xd2, 0x68, 0x86, 0x5f, 0x3c, 0x5e, 0xe6, 0xc1,
    0x8e, 0x32, 0x2b, 0x28, 0xcb, 0xb5, 0xcc, 0x1b, 0xa8, 0x50, 0x5e, 0xa7,
    0x0d, 0x02, 0x03, 0x01, 0x00, 0x01
};

// EC public key in DER format
const uint8_t SERVER_MOVED_SIGNATURE_PUBLIC_KEY[] = {
    0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02,
    0x01, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03,
    0x42, 0x00, 0x04, 0x91, 0xc4, 0x75, 0x6c, 0xfa, 0xc7, 0x74, 0xed, 0xa6,
    0x0f, 0xd9, 0xa3, 0x69, 0xde, 0x2e, 0x39, 0x73, 0xab, 0xce, 0x1e, 0x2a,
    0x7a, 0x86, 0x9e, 0x4b, 0x35, 0xc1, 0x8a, 0xe3, 0x87, 0x17, 0xd6, 0xf0,
    0x37, 0x26, 0x35, 0xcd, 0xaa, 0x7c, 0xc2, 0x56, 0x62, 0x64, 0x48, 0x9d,
    0x28, 0x12, 0xf5, 0xe7, 0xa3, 0x91, 0x35, 0x96, 0xfb, 0x20, 0xb1, 0xa3,
    0x43, 0x00, 0xfa, 0x7d, 0x67, 0xf3, 0x4d
};

const auto DEFAULT_UDP_SERVER_ADDRESS = "$id.udp.particle.io";
const auto DEFAULT_TCP_SERVER_ADDRESS = "device.tcp.particle.io";

const uint16_t DEFAULT_UDP_SERVER_PORT = 5684;
const uint16_t DEFAULT_TCP_SERVER_PORT = 5683;

int updateServerSettings(const char* addr, uint16_t port, const uint8_t* pubKey, size_t pubKeySize, bool udp) {
    LOG(INFO, "Updating %s server settings", udp ? "UDP" : "TCP");
    LOG(INFO, "Address: \"%s\"; port: %u", addr, (unsigned)port);
    LOG(TRACE, "Public key (%u bytes):", (unsigned)pubKeySize);
    LOG_DUMP(TRACE, pubKey, pubKeySize);
    LOG_PRINT(TRACE, "\r\n");
    // Parse and validate the new server address
    ServerAddress saddr = {};
    CHECK(parseServerAddressString(&saddr, addr));
    saddr.port = port;
    // Erase both the current server key and address so that we don't potentially end up with a
    // key and address from different servers in the DCT
    size_t maxPubKeySize = udp ? DCT_ALT_SERVER_PUBLIC_KEY_SIZE : DCT_SERVER_PUBLIC_KEY_SIZE;
    if (pubKeySize > maxPubKeySize) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    size_t maxAddrSize = udp ? DCT_ALT_SERVER_ADDRESS_SIZE : DCT_SERVER_ADDRESS_SIZE;
    size_t bufSize = std::max(maxPubKeySize, maxAddrSize);
    std::unique_ptr<uint8_t[]> buf(new(std::nothrow) uint8_t[bufSize]);
    if (!buf) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    memset(buf.get(), 0xff, bufSize);
    size_t addrOffs = udp ? DCT_ALT_SERVER_ADDRESS_OFFSET : DCT_SERVER_ADDRESS_OFFSET;
    int r = dct_write_app_data(buf.get(), addrOffs, maxAddrSize);
    if (r != 0) {
        return SYSTEM_ERROR_IO;
    }
    size_t pubKeyOffs = udp ? DCT_ALT_SERVER_PUBLIC_KEY_OFFSET : DCT_SERVER_PUBLIC_KEY_OFFSET;
    r = dct_write_app_data(buf.get(), pubKeyOffs, maxPubKeySize);
    if (r != 0) {
        return SYSTEM_ERROR_IO;
    }
    // Write the server key
    r = dct_write_app_data(pubKey, pubKeyOffs, pubKeySize);
    if (r != 0) {
        return SYSTEM_ERROR_IO;
    }
    // Serialize and write the server address
    CHECK(encodeServerAddressData(&saddr, buf.get(), maxAddrSize));
    r = dct_write_app_data(buf.get(), addrOffs, maxAddrSize);
    if (r != 0) {
        return SYSTEM_ERROR_IO;
    }
    return 0;
}

int validateServerSettings(bool udp) {
    size_t maxPubKeySize = udp ? DCT_ALT_SERVER_PUBLIC_KEY_SIZE : DCT_SERVER_PUBLIC_KEY_SIZE;
    size_t maxAddrSize = udp ? DCT_ALT_SERVER_ADDRESS_SIZE : DCT_SERVER_ADDRESS_SIZE;
    size_t bufSize = std::max(maxPubKeySize, maxAddrSize);
    std::unique_ptr<uint8_t[]> buf(new(std::nothrow) uint8_t[bufSize]);
    if (!buf) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    // Load and validate the server address
    size_t addrOffs = udp ? DCT_ALT_SERVER_ADDRESS_OFFSET : DCT_SERVER_ADDRESS_OFFSET;
    int r = dct_read_app_data_copy(addrOffs, buf.get(), maxAddrSize);
    if (r != 0) {
        return SYSTEM_ERROR_IO;
    }
    ServerAddress addr = {};
    parseServerAddressData(&addr, buf.get(), maxAddrSize);
    if (addr.addr_type == INVALID_INTERNET_ADDRESS) {
        return SYSTEM_ERROR_INVALID_SERVER_SETTINGS;
    }
    // Load the server key
    size_t pubKeyOffs = udp ? DCT_ALT_SERVER_PUBLIC_KEY_OFFSET : DCT_SERVER_PUBLIC_KEY_OFFSET;
    r = dct_read_app_data_copy(pubKeyOffs, buf.get(), maxPubKeySize);
    if (r != 0) {
        return SYSTEM_ERROR_IO;
    }
    // Determine the actual size of the key
    uint8_t* p = buf.get();
    size_t pubKeySize = 0;
    r = mbedtls_asn1_get_tag(&p, buf.get() + maxPubKeySize, &pubKeySize, MBEDTLS_ASN1_CONSTRUCTED |
            MBEDTLS_ASN1_SEQUENCE);
    if (r != 0) {
        return SYSTEM_ERROR_INVALID_SERVER_SETTINGS;
    }
    pubKeySize += p - buf.get(); // Include size of the tag and length fields
    // Parse the key
    mbedtls_pk_context pk = {};
    mbedtls_pk_init(&pk);
    SCOPE_GUARD({
        mbedtls_pk_free(&pk);
    });
    r = mbedtls_pk_parse_public_key(&pk, buf.get(), pubKeySize);
    if (r != 0) {
        return SYSTEM_ERROR_INVALID_SERVER_SETTINGS;
    }
    return 0;
}

inline bool udpEnabled() {
    // XXX: The current gen platforms don't really allow changing the cloud transport protocol,
    // but let's keep this check in place until the support for TCP is removed from the codebase
    return HAL_Feature_Get(FEATURE_CLOUD_UDP);
}

} // namespace

int ServerConfig::updateSettings(const ServerSettings& conf) {
    bool udp = udpEnabled();
    auto port = conf.port;
    if (!port) {
        port = udp ? DEFAULT_UDP_SERVER_PORT : DEFAULT_TCP_SERVER_PORT;
    }
    CHECK(updateServerSettings(conf.address, port, conf.publicKey, conf.publicKeySize, udp));
    return 0;
}

int ServerConfig::validateSettings() const {
    CHECK(validateServerSettings(udpEnabled()));
    return 0;
}

int ServerConfig::validateServerMovedRequest(const ServerMovedRequest& req) const {
    if (!udpEnabled()) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    // Load the signature key
    mbedtls_pk_context pk = {};
    mbedtls_pk_init(&pk);
    SCOPE_GUARD({
        mbedtls_pk_free(&pk);
    });
    CHECK_MBEDTLS(mbedtls_pk_parse_public_key(&pk, SERVER_MOVED_SIGNATURE_PUBLIC_KEY, sizeof(SERVER_MOVED_SIGNATURE_PUBLIC_KEY)));
    // Hash the server settings
    Sha256 sha;
    CHECK(sha.init());
    CHECK(sha.start());
    uint32_t u32 = nativeToLittleEndian<uint32_t>(req.publicKeySize);
    CHECK(sha.update((const char*)&u32, sizeof(u32)));
    CHECK(sha.update((const char*)req.publicKey, req.publicKeySize));
    size_t addrLen = strlen(req.address);
    u32 = nativeToLittleEndian<uint32_t>(addrLen);
    CHECK(sha.update((const char*)&u32, sizeof(u32)));
    CHECK(sha.update(req.address, addrLen));
    auto port = req.port;
    if (!port) {
        port = DEFAULT_UDP_SERVER_PORT;
    }
    uint16_t u16 = nativeToLittleEndian<uint16_t>(port);
    CHECK(sha.update((const char*)&u16, sizeof(u16)));
    char hash[Sha256::HASH_SIZE] = {};
    CHECK(sha.finish(hash));
    // Verify the signature
    int r = mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, (const uint8_t*)hash, sizeof(hash), req.signature, req.signatureSize);
    if (r != 0) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    return 0;
}

int ServerConfig::restoreDefaultSettings() {
    if (udpEnabled()) {
        CHECK(updateServerSettings(DEFAULT_UDP_SERVER_ADDRESS, DEFAULT_UDP_SERVER_PORT, DEFAULT_UDP_SERVER_PUBLIC_KEY,
                sizeof(DEFAULT_UDP_SERVER_PUBLIC_KEY), true /* udp */));
    } else {
        CHECK(updateServerSettings(DEFAULT_TCP_SERVER_ADDRESS, DEFAULT_TCP_SERVER_PORT, DEFAULT_TCP_SERVER_PUBLIC_KEY,
                sizeof(DEFAULT_TCP_SERVER_PUBLIC_KEY), false));
    }
    return 0;
}

ServerConfig* ServerConfig::instance() {
    static ServerConfig conf;
    return &conf;
}

} // namespace particle

#endif // (PLATFORM_ID != PLATFORM_GCC && PLATFORM_ID != PLATFORM_NEWHAL) || defined(UNIT_TEST)
