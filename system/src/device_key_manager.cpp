#include "device_key_manager.h"
#include "eckeygen.h"
#include "mbedtls_util.h"
#include "file_util.h"
#include "dct.h"
#include "check.h"

#include <algorithm>
#include <memory>

namespace particle::system {

const auto CANDIDATE_KEY_FILE = "/sys/new_device_key";
const size_t MAX_KEY_SIZE = DCT_ALT_DEVICE_PRIVATE_KEY_SIZE;

int DeviceKeyManager::generateNewKey() {
    std::unique_ptr<char[]> buf(new(std::nothrow) char[MAX_KEY_SIZE]);
    if (!buf) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    int r = gen_ec_key((uint8_t*)buf.get(), MAX_KEY_SIZE, mbedtls_default_rng, nullptr /* p_rng */);
    if (r <= 0) {
        return SYSTEM_ERROR_CRYPTO;
    }
    size_t keySize = r;

    fs::File f;
    char tmpPath[TEMP_PATH_LEN + 1] = {};
    CHECK(createTempFile(f, tmpPath, sizeof(tmpPath), LFS_O_WRONLY));
    CHECK(f.write(buf.get(), keySize));
    CHECK(f.close());

    CHECK(fs::rename(tmpPath, CANDIDATE_KEY_FILE));
    hasNewKey_ = true;
    return 0;
}

int DeviceKeyManager::getNewKey(char* buf, size_t bufSize) {
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

    size_t keySize = CHECK(f.size());
    if (bufSize > 0) {
        if (keySize > bufSize) {
            return SYSTEM_ERROR_TOO_LARGE;
        }
        size_t n = CHECK(f.read(buf, keySize));
        if (n != keySize) {
            return SYSTEM_ERROR_BAD_DATA;
        }
    }
    return keySize;
}

int DeviceKeyManager::applyNewKey() {
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

    size_t keySize = CHECK(f.size());
    if (keySize > MAX_KEY_SIZE) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    std::unique_ptr<char[]> key(new(std::nothrow) char[keySize]);
    if (!key) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    size_t n = CHECK(f.read(key.get(), keySize));
    if (n != keySize) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    CHECK(f.close());

    r = dct_write_app_data(key.get(), DCT_ALT_DEVICE_PRIVATE_KEY_OFFSET, keySize);
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

} // namespace particle::system
