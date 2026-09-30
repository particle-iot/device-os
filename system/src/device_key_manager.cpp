#include "device_key_manager.h"
#include "eckeygen.h"
#include "mbedtls_util.h"
#include "file_util.h"
#include "check.h"

#include <algorithm>
#include <memory>
#include <cstring>

namespace particle::system {

namespace {

const auto CANDIDATE_KEY_FILE = "/sys/new_device_key";

int readAll(fs::File& file, char* buf, size_t bufSize) {
    size_t bytesToRead = CHECK(file.size()) - CHECK(file.tell());
    if (!bufSize) {
        return bytesToRead;
    }
    if (bytesToRead > bufSize) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    size_t bytesRead = CHECK(file.read(buf, bytesToRead));
    if (bytesRead != bytesToRead) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    return bytesRead;
}

} // namespace

int DeviceKeyManager::generateNewKey() {
    std::unique_ptr<char[]> key(new(std::nothrow) char[MAX_DEVICE_PRIVATE_KEY_SIZE]);
    if (!key) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    int r = gen_ec_key((uint8_t*)key.get(), MAX_DEVICE_PRIVATE_KEY_SIZE, mbedtls_default_rng, nullptr /* p_rng */);
    if (r <= 0) {
        return SYSTEM_ERROR_CRYPTO;
    }
    size_t keySize = r;

    fs::File f;
    char tmpPath[TEMP_PATH_LEN + 1] = {};
    CHECK(createTempFile(f, tmpPath, sizeof(tmpPath), LFS_O_WRONLY));
    CHECK(f.write(key.get(), keySize));
    CHECK(f.close());

    CHECK(fs::rename(tmpPath, CANDIDATE_KEY_FILE));
    hasNewKey_ = true;
    return 0;
}

int DeviceKeyManager::getNewPrivateKey(char* buf, size_t bufSize) {
    fs::File f;
    CHECK(openKeyFile(f));

    size_t keySize = CHECK(readAll(f, buf, bufSize));
    return keySize;
}

int DeviceKeyManager::getNewPublicKey(char* buf, size_t bufSize) {
    fs::File f;
    CHECK(openKeyFile(f));

    size_t privSize = CHECK(f.size());
    std::unique_ptr<char[]> priv(new(std::nothrow) char[privSize]);
    if (!priv) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    CHECK(readAll(f, priv.get(), privSize));
    CHECK(f.close());

    // Extract the public key
    std::unique_ptr<char[]> pub(new(std::nothrow) char[MAX_DEVICE_PUBLIC_KEY_SIZE]);
    if (!pub) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    int r = extract_public_ec_key_length((uint8_t*)pub.get(), MAX_DEVICE_PUBLIC_KEY_SIZE, (const uint8_t*)priv.get(), privSize);
    if (r < 0) {
        return SYSTEM_ERROR_CRYPTO;
    }
    size_t pubSize = r;
    if (pubSize > bufSize) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    // extract_public_ec_key_length() writes to the end of the buffer
    std::memcpy(buf, pub.get() + MAX_DEVICE_PUBLIC_KEY_SIZE - pubSize, pubSize);
    return pubSize;
}

int DeviceKeyManager::applyNewKey() {
    fs::File f;
    CHECK(openKeyFile(f));

    size_t keySize = CHECK(f.size());
    std::unique_ptr<char[]> key(new(std::nothrow) char[keySize]);
    if (!key) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    CHECK(readAll(f, key.get(), keySize));
    CHECK(f.close());

    int r = dct_write_app_data(key.get(), DCT_ALT_DEVICE_PRIVATE_KEY_OFFSET, keySize);
    if (r != 0) {
        return SYSTEM_ERROR_IO;
    }

    CHECK(rmrf(CANDIDATE_KEY_FILE));
    hasNewKey_ = false;
    return 0;
}

int DeviceKeyManager::clearNewKey() {
    CHECK(rmrf(CANDIDATE_KEY_FILE));
    hasNewKey_ = false;
    return 0;
}

DeviceKeyManager& DeviceKeyManager::instance() {
    static DeviceKeyManager mgr;
    return mgr;
}

int DeviceKeyManager::openKeyFile(fs::File& file) {
    if (hasNewKey_.has_value() && !hasNewKey_.value()) {
        return SYSTEM_ERROR_KEY_NOT_FOUND;
    }

    fs::File f;
    int r = f.open(CANDIDATE_KEY_FILE, LFS_O_RDONLY);
    if (r < 0) {
        if (r == SYSTEM_ERROR_FILESYSTEM_NOENT) {
            hasNewKey_ = false;
            return SYSTEM_ERROR_KEY_NOT_FOUND;
        }
        return r;
    }
    file = std::move(f);
    return 0;
}

} // namespace particle::system
