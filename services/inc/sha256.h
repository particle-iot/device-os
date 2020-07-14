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

#include "mbedtls_util.h"
#include "check.h"

#include "mbedtls/md.h"

#include <cstring>

namespace particle {

class Sha256 {
public:
    static const size_t HASH_SIZE = 32;
    static const size_t BLOCK_SIZE = 64;

    Sha256();
    ~Sha256();

    int init();
    void destroy();

    int start();
    int finish(char* buf);

    int update(const char* data, size_t size);
    int update(const char* str);

    int copyFrom(const Sha256& src);

private:
    mbedtls_md_context_t ctx_;
};

class HmacSha256 {
public:
    static const size_t HASH_SIZE = 32;
    static const size_t BLOCK_SIZE = 64;

    HmacSha256();
    ~HmacSha256();

    int init();
    void destroy();

    int start(const char* key, size_t keySize);
    int finish(char* buf);

    int update(const char* data, size_t size);
    int update(const char* str);

    int reset();

private:
    mbedtls_md_context_t ctx_;
};

inline Sha256::Sha256() :
        ctx_() {
}

inline Sha256::~Sha256() {
    destroy();
}

inline int Sha256::init() {
    const auto mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!mdInfo) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    mbedtls_md_init(&ctx_);
    CHECK_MBEDTLS(mbedtls_md_setup(&ctx_, mdInfo, 0 /* hmac */));
    return 0;
}

inline void Sha256::destroy() {
    mbedtls_md_free(&ctx_);
}

inline int Sha256::start() {
    CHECK_MBEDTLS(mbedtls_md_starts(&ctx_));
    return 0;
}

inline int Sha256::finish(char* buf) {
    CHECK_MBEDTLS(mbedtls_md_finish(&ctx_, (uint8_t*)buf));
    return 0;
}

inline int Sha256::update(const char* data, size_t size) {
    CHECK_MBEDTLS(mbedtls_md_update(&ctx_, (const uint8_t*)data, size));
    return 0;
}

inline int Sha256::update(const char* str) {
    return update(str, strlen(str));
}

inline int Sha256::copyFrom(const Sha256& src) {
    CHECK_MBEDTLS(mbedtls_md_clone(&ctx_, &src.ctx_));
    return 0;
}

inline HmacSha256::HmacSha256() :
        ctx_() {
}

inline HmacSha256::~HmacSha256() {
    destroy();
}

inline int HmacSha256::init() {
    const auto mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!mdInfo) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    mbedtls_md_init(&ctx_);
    CHECK_MBEDTLS(mbedtls_md_setup(&ctx_, mdInfo, 1 /* hmac */));
    return 0;
}

inline void HmacSha256::destroy() {
    mbedtls_md_free(&ctx_);
}

inline int HmacSha256::start(const char* key, size_t keySize) {
    CHECK_MBEDTLS(mbedtls_md_hmac_starts(&ctx_, (const uint8_t*)key, keySize));
    return 0;
}

inline int HmacSha256::finish(char* buf) {
    CHECK_MBEDTLS(mbedtls_md_hmac_finish(&ctx_, (uint8_t*)buf));
    return 0;
}

inline int HmacSha256::update(const char* data, size_t size) {
    CHECK_MBEDTLS(mbedtls_md_hmac_update(&ctx_, (const uint8_t*)data, size));
    return 0;
}

inline int HmacSha256::update(const char* str) {
    return update(str, strlen(str));
}

inline int HmacSha256::reset() {
    CHECK_MBEDTLS(mbedtls_md_hmac_reset(&ctx_));
    return 0;
}

} // namespace particle
