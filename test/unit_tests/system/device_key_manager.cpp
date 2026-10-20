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

#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>
#include <fakeit.hpp>

#include "device_key_manager.h"
#include "file_util.h"
#include "filesystem.h"
#include "system_error.h"
#include "coap_api.h"

#include "mock/dct_hal_mock.h"
#include "stub/dct.h"

#include "mbedtls/pk.h"
#include "mbedtls/ecp.h"

#include <optional>
#include <random>
#include <string>
#include <vector>
#include <memory>
#include <cstring>

using namespace particle;
using namespace particle::system;
using namespace particle::test;

using namespace fakeit;

namespace {

const auto CANDIDATE_KEY_FILE = "/sys/new_device_key";

std::mt19937& rng() {
    static std::mt19937 gen(std::random_device{}());
    return gen;
}

std::string randBytes(size_t size) {
    std::uniform_int_distribution<int> dist(0, 255);
    std::string s(size, '\0');
    for (auto& c: s) {
        c = (char)dist(rng());
    }
    return s;
}

EcpPrivateKey makePrivateKey(const std::string& data) {
    std::unique_ptr<char[]> buf(new char[data.size()]);
    std::memcpy(buf.get(), data.data(), data.size());
    return EcpPrivateKey(std::move(buf), data.size());
}

std::string keyData(const CryptoKey& key) {
    return std::string(key.data(), key.size());
}

lfs_t* lfs() {
    return &filesystem_get_instance(FILESYSTEM_INSTANCE_DEFAULT, nullptr)->instance;
}

void writeFile(const char* path, const std::string& data) {
    fs::File f;
    REQUIRE(f.open(path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) == 0);
    REQUIRE(f.write(data.data(), data.size()) == (int)data.size());
    REQUIRE(f.close() == 0);
}

std::optional<std::string> readFile(const char* path) {
    fs::File f;
    int r = f.open(path, LFS_O_RDONLY);
    if (r == SYSTEM_ERROR_FILESYSTEM_NOENT) {
        return std::nullopt;
    }
    REQUIRE(r == 0);
    int size = f.size();
    REQUIRE(size >= 0);
    std::string data(size, '\0');
    REQUIRE(f.read(data.data(), size) == size);
    return data;
}

size_t dirEntryCount(const char* path) {
    lfs_dir_t dir = {};
    REQUIRE(lfs_dir_open(lfs(), &dir, path) == 0);
    size_t count = 0;
    lfs_info info = {};
    int r = 0;
    while ((r = lfs_dir_read(lfs(), &dir, &info)) > 0) {
        if (std::strcmp(info.name, ".") != 0 && std::strcmp(info.name, "..") != 0) {
            ++count;
        }
    }
    REQUIRE(r == 0);
    REQUIRE(lfs_dir_close(lfs(), &dir) == 0);
    return count;
}

void resetFilesystem() {
    REQUIRE(rmrf("/sys") == 0);
    REQUIRE(rmrf("/tmp") == 0);
    REQUIRE(lfs_mkdir(lfs(), "/sys") == 0);
    REQUIRE(lfs_mkdir(lfs(), "/tmp") == 0);
}

// Parses a DER-encoded EC private key and returns its DER-encoded public key
std::string derivePublicKey(const CryptoKey& privKey) {
    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);
    REQUIRE(mbedtls_pk_parse_key(&pk, (const uint8_t*)privKey.data(), privKey.size(), nullptr, 0) == 0);
    REQUIRE(mbedtls_pk_get_type(&pk) == MBEDTLS_PK_ECKEY);
    REQUIRE(mbedtls_pk_ec(pk)->grp.id == MBEDTLS_ECP_DP_SECP256R1);
    uint8_t buf[MAX_DEVICE_PUBLIC_KEY_SIZE] = {};
    int r = mbedtls_pk_write_pubkey_der(&pk, buf, sizeof(buf));
    mbedtls_pk_free(&pk);
    REQUIRE(r > 0);
    // mbedtls_pk_write_pubkey_der() writes the key to the end of the buffer
    return std::string((const char*)buf + sizeof(buf) - r, r);
}

} // namespace

// Fakes for the platform functions used by the code under test

extern "C" uint32_t HAL_RNG_GetRandomNumber() {
    return rng()();
}

int mbedtls_default_rng(void*, unsigned char* data, size_t size) {
    auto s = randBytes(size);
    std::memcpy(data, s.data(), size);
    return 0;
}

// Referenced by services/src/nanopb_misc.c but not used by the code under test
extern "C" int coap_write_block(coap_message* msg, const char* data, size_t* size, coap_block_callback block_cb,
        coap_error_callback error_cb, void* arg, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

extern "C" int coap_read_block(coap_message* msg, char* data, size_t* size, coap_block_callback block_cb,
        coap_error_callback error_cb, void* arg, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

extern "C" int coap_peek_block(coap_message* msg, char* data, size_t size, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

TEST_CASE("DeviceKeyManager") {
    resetFilesystem();

    DeviceKeyManager mgr;

    SECTION("getCandidateKey()") {
        SECTION("fails with SYSTEM_ERROR_CRYPTO_KEY_NOT_FOUND if there's no candidate key") {
            EcpPrivateKey key;
            CHECK(mgr.getCandidateKey(key) == SYSTEM_ERROR_CRYPTO_KEY_NOT_FOUND);
            CHECK(!key.isValid());
        }

        SECTION("loads a candidate key that was stored by another instance") {
            auto data = randBytes(121);
            writeFile(CANDIDATE_KEY_FILE, data);
            EcpPrivateKey key;
            REQUIRE(mgr.getCandidateKey(key) == 0);
            CHECK(key.isValid());
            CHECK(key.type() == CryptoKey::Type::PRIVATE);
            CHECK(key.algorithm() == CryptoKey::Algorithm::ECP);
            CHECK(keyData(key) == data);
        }

        SECTION("accepts a key of the maximum size") {
            auto data = randBytes(MAX_DEVICE_PRIVATE_KEY_SIZE);
            writeFile(CANDIDATE_KEY_FILE, data);
            EcpPrivateKey key;
            REQUIRE(mgr.getCandidateKey(key) == 0);
            CHECK(keyData(key) == data);
        }

        SECTION("fails with SYSTEM_ERROR_BAD_DATA if the key file is empty") {
            writeFile(CANDIDATE_KEY_FILE, "");
            EcpPrivateKey key;
            CHECK(mgr.getCandidateKey(key) == SYSTEM_ERROR_BAD_DATA);
        }

        SECTION("fails with SYSTEM_ERROR_TOO_LARGE if the key file is too large") {
            writeFile(CANDIDATE_KEY_FILE, randBytes(MAX_DEVICE_PRIVATE_KEY_SIZE + 1));
            EcpPrivateKey key;
            CHECK(mgr.getCandidateKey(key) == SYSTEM_ERROR_TOO_LARGE);
        }
    }

    SECTION("setCandidateKey()") {
        SECTION("stores the key in the filesystem") {
            auto data = randBytes(121);
            REQUIRE(mgr.setCandidateKey(makePrivateKey(data)) == 0);
            CHECK(readFile(CANDIDATE_KEY_FILE) == data);
        }

        SECTION("doesn't leave temporary files behind") {
            REQUIRE(mgr.setCandidateKey(makePrivateKey(randBytes(121))) == 0);
            CHECK(dirEntryCount("/tmp") == 0);
        }

        SECTION("replaces an existing candidate key") {
            REQUIRE(mgr.setCandidateKey(makePrivateKey(randBytes(121))) == 0);
            auto data = randBytes(100);
            REQUIRE(mgr.setCandidateKey(makePrivateKey(data)) == 0);
            CHECK(readFile(CANDIDATE_KEY_FILE) == data);
            EcpPrivateKey key;
            REQUIRE(mgr.getCandidateKey(key) == 0);
            CHECK(keyData(key) == data);
        }

        SECTION("makes the key available via getCandidateKey() after the candidate key was cleared") {
            REQUIRE(mgr.clearCandidateKey() == 0);
            EcpPrivateKey key;
            REQUIRE(mgr.getCandidateKey(key) == SYSTEM_ERROR_CRYPTO_KEY_NOT_FOUND);
            auto data = randBytes(121);
            REQUIRE(mgr.setCandidateKey(makePrivateKey(data)) == 0);
            REQUIRE(mgr.getCandidateKey(key) == 0);
            CHECK(keyData(key) == data);
        }

        SECTION("stores the key persistently") {
            auto data = randBytes(121);
            REQUIRE(mgr.setCandidateKey(makePrivateKey(data)) == 0);
            DeviceKeyManager mgr2; // Simulate a reset
            EcpPrivateKey key;
            REQUIRE(mgr2.getCandidateKey(key) == 0);
            CHECK(keyData(key) == data);
        }

        SECTION("fails with SYSTEM_ERROR_INVALID_ARGUMENT if the key is invalid") {
            CHECK(mgr.setCandidateKey(EcpPrivateKey()) == SYSTEM_ERROR_INVALID_ARGUMENT);
            CHECK(!readFile(CANDIDATE_KEY_FILE));
        }
    }

    SECTION("clearCandidateKey()") {
        SECTION("removes the candidate key") {
            REQUIRE(mgr.setCandidateKey(makePrivateKey(randBytes(121))) == 0);
            REQUIRE(mgr.clearCandidateKey() == 0);
            CHECK(!readFile(CANDIDATE_KEY_FILE));
            EcpPrivateKey key;
            CHECK(mgr.getCandidateKey(key) == SYSTEM_ERROR_CRYPTO_KEY_NOT_FOUND);
        }

        SECTION("removes a candidate key that was stored by another instance") {
            writeFile(CANDIDATE_KEY_FILE, randBytes(121));
            REQUIRE(mgr.clearCandidateKey() == 0);
            CHECK(!readFile(CANDIDATE_KEY_FILE));
        }

        SECTION("succeeds if there's no candidate key") {
            CHECK(mgr.clearCandidateKey() == 0);
            CHECK(mgr.clearCandidateKey() == 0);
        }
    }

    SECTION("acceptCandidateKey()") {
        DctHalMock dctHal;
        std::vector<std::pair<uint32_t, std::string>> dctWrites;
        When(Method(dctHal, writeAppData)).AlwaysDo([&](uint32_t offs, std::string data) {
            dctWrites.emplace_back(offs, std::move(data));
            return 0;
        });

        SECTION("writes the candidate key to the DCT and removes it from the filesystem") {
            auto data = randBytes(121);
            REQUIRE(mgr.setCandidateKey(makePrivateKey(data)) == 0);
            REQUIRE(mgr.acceptCandidateKey() == 0);
            REQUIRE(dctWrites.size() == 1);
            CHECK(dctWrites[0].first == DCT_ALT_DEVICE_PRIVATE_KEY_OFFSET);
            CHECK(dctWrites[0].second == data);
            CHECK(!readFile(CANDIDATE_KEY_FILE));
            EcpPrivateKey key;
            CHECK(mgr.getCandidateKey(key) == SYSTEM_ERROR_CRYPTO_KEY_NOT_FOUND);
        }

        SECTION("accepts a candidate key that was stored by another instance") {
            auto data = randBytes(121);
            writeFile(CANDIDATE_KEY_FILE, data);
            REQUIRE(mgr.acceptCandidateKey() == 0);
            REQUIRE(dctWrites.size() == 1);
            CHECK(dctWrites[0].second == data);
            CHECK(!readFile(CANDIDATE_KEY_FILE));
        }

        SECTION("fails with SYSTEM_ERROR_CRYPTO_KEY_NOT_FOUND if there's no candidate key") {
            CHECK(mgr.acceptCandidateKey() == SYSTEM_ERROR_CRYPTO_KEY_NOT_FOUND);
            CHECK(dctWrites.empty());
        }

        SECTION("keeps the candidate key if writing to the DCT fails") {
            When(Method(dctHal, writeAppData)).AlwaysReturn(-1);
            auto data = randBytes(121);
            REQUIRE(mgr.setCandidateKey(makePrivateKey(data)) == 0);
            CHECK(mgr.acceptCandidateKey() == SYSTEM_ERROR_IO);
            CHECK(readFile(CANDIDATE_KEY_FILE) == data);
            EcpPrivateKey key;
            REQUIRE(mgr.getCandidateKey(key) == 0);
            CHECK(keyData(key) == data);
        }
    }

    SECTION("generateKey()") {
        SECTION("generates a valid secp256r1 private key") {
            EcpPrivateKey key;
            REQUIRE(mgr.generateKey(key) == 0);
            CHECK(key.isValid());
            CHECK(key.type() == CryptoKey::Type::PRIVATE);
            CHECK(key.algorithm() == CryptoKey::Algorithm::ECP);
            CHECK(key.size() <= MAX_DEVICE_PRIVATE_KEY_SIZE);
            derivePublicKey(key); // Parses and validates the key
        }

        SECTION("generates a different key every time") {
            EcpPrivateKey key1, key2;
            REQUIRE(mgr.generateKey(key1) == 0);
            REQUIRE(mgr.generateKey(key2) == 0);
            CHECK(keyData(key1) != keyData(key2));
        }

        SECTION("doesn't store the generated key as the candidate key") {
            EcpPrivateKey key;
            REQUIRE(mgr.generateKey(key) == 0);
            CHECK(!readFile(CANDIDATE_KEY_FILE));
        }

        SECTION("generates a key that can be stored and loaded back") {
            EcpPrivateKey key;
            REQUIRE(mgr.generateKey(key) == 0);
            REQUIRE(mgr.setCandidateKey(key) == 0);
            DeviceKeyManager mgr2;
            EcpPrivateKey loadedKey;
            REQUIRE(mgr2.getCandidateKey(loadedKey) == 0);
            CHECK(keyData(loadedKey) == keyData(key));
        }
    }
}

TEST_CASE("EcpPrivateKey") {
    SECTION("getPublicKey()") {
        SECTION("returns the public key matching the private key") {
            DeviceKeyManager mgr;
            EcpPrivateKey privKey;
            REQUIRE(mgr.generateKey(privKey) == 0);
            EcpPublicKey pubKey;
            int r = privKey.getPublicKey(pubKey);
            REQUIRE(r > 0);
            CHECK((size_t)r == pubKey.size());
            CHECK(pubKey.isValid());
            CHECK(pubKey.type() == CryptoKey::Type::PUBLIC);
            CHECK(pubKey.algorithm() == CryptoKey::Algorithm::ECP);
            CHECK(keyData(pubKey) == derivePublicKey(privKey));
        }

        SECTION("fails with SYSTEM_ERROR_INVALID_STATE if the private key is invalid") {
            EcpPrivateKey privKey;
            EcpPublicKey pubKey;
            CHECK(privKey.getPublicKey(pubKey) == SYSTEM_ERROR_INVALID_STATE);
        }

        SECTION("fails with SYSTEM_ERROR_CRYPTO if the private key can't be parsed") {
            auto privKey = makePrivateKey(randBytes(121));
            EcpPublicKey pubKey;
            CHECK(privKey.getPublicKey(pubKey) == SYSTEM_ERROR_CRYPTO);
        }
    }
}
