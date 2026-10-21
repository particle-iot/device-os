#include "device_key_manager.h"
#include "eckeygen.h"
#include "mbedtls_util.h"
#include "file_util.h"
#include "check.h"

#include <algorithm>
#include <memory>
#include <cstring>

// This is a core feature so keep it buildable (but not necessarily fully supported) on all platforms
// including GCC/newhal
#if (PLATFORM_ID != PLATFORM_GCC && PLATFORM_ID != PLATFORM_NEWHAL) || defined(UNIT_TEST)
#include "dct.h"
#define HAS_DCT 1
#else
#define HAS_DCT 0
#endif

namespace particle::system {

namespace {

const auto CANDIDATE_KEY_FILE = "/sys/new_device_key";

#if HAS_DCT

const size_t MAX_DEVICE_PRIVATE_KEY_SIZE = DCT_ALT_DEVICE_PRIVATE_KEY_SIZE; // 192
const size_t MAX_DEVICE_PUBLIC_KEY_SIZE = DCT_ALT_DEVICE_PUBLIC_KEY_SIZE; // 128

// Saves the device key to its default storage
int saveNewDeviceKey(const char* data, size_t size) {
    if (size > MAX_DEVICE_PRIVATE_KEY_SIZE) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    int r = dct_write_app_data(data, DCT_ALT_DEVICE_PRIVATE_KEY_OFFSET, size);
    if (r != 0) { // `r` is not a system_error_t
        return SYSTEM_ERROR_IO;
    }
    return 0;
}

#else // !HAS_DCT

const size_t MAX_DEVICE_PRIVATE_KEY_SIZE = 1024;
const size_t MAX_DEVICE_PUBLIC_KEY_SIZE = 1024;

int saveNewDeviceKey(const char* data, size_t size) {
    return SYSTEM_ERROR_NOT_SUPPORTED; // TODO
}

#endif // !HAS_DCT

} // namespace

int EcpPrivateKey::getPublicKey(EcpPublicKey& pub) const {
    if (!isValid()) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    const auto bufSize = MAX_DEVICE_PUBLIC_KEY_SIZE; // FIXME: This is a generic key class
    std::unique_ptr<char[]> pubData(new(std::nothrow) char[bufSize]);
    if (!pubData) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    int r = extract_public_ec_key_length((uint8_t*)pubData.get(), bufSize, (const uint8_t*)this->data(), this->size());
    if (r < 0) {
        return SYSTEM_ERROR_CRYPTO;
    }
    size_t pubSize = r;
    // extract_public_ec_key_length() writes the key to the end of the buffer
    std::memmove(pubData.get(), pubData.get() + bufSize - pubSize, pubSize);
    pub = EcpPublicKey(std::move(pubData), pubSize);
    return pubSize;
}

int DeviceKeyManager::setCandidateKey(const EcpPrivateKey& key) {
    if (!key.isValid()) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    fs::File f;
    char tmpPath[TEMP_PATH_LEN + 1] = {};
    CHECK(createTempFile(f, tmpPath, sizeof(tmpPath), LFS_O_WRONLY));
    CHECK(f.write(key.data(), key.size()));
    CHECK(f.close());

    CHECK(fs::rename(tmpPath, CANDIDATE_KEY_FILE));
    hasCandidateKey_ = true;
    return 0;
}

int DeviceKeyManager::getCandidateKey(EcpPrivateKey& key) {
    CHECK(loadCandidateKey(key));
    return 0;
}

int DeviceKeyManager::acceptCandidateKey() {
    EcpPrivateKey k;
    CHECK(loadCandidateKey(k));
    CHECK(saveNewDeviceKey(k.data(), k.size()));
    CHECK(rmrf(CANDIDATE_KEY_FILE));
    hasCandidateKey_ = false;
    return 0;
}

int DeviceKeyManager::clearCandidateKey() {
    if (hasCandidateKey_.has_value() && !hasCandidateKey_.value()) {
        return 0;
    }
    CHECK(rmrf(CANDIDATE_KEY_FILE));
    hasCandidateKey_ = false;
    return 0;
}

int DeviceKeyManager::generateKey(EcpPrivateKey& key) {
    const size_t bufSize = MAX_DEVICE_PRIVATE_KEY_SIZE;
    std::unique_ptr<char[]> privData(new(std::nothrow) char[bufSize]);
    if (!privData) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    int r = gen_ec_key((uint8_t*)privData.get(), bufSize, mbedtls_default_rng, nullptr /* p_rng */);
    if (r <= 0) {
        return SYSTEM_ERROR_CRYPTO;
    }
    size_t privSize = r;
    key = EcpPrivateKey(std::move(privData), privSize);
    return 0;
}

DeviceKeyManager& DeviceKeyManager::instance() {
    static DeviceKeyManager mgr;
    return mgr;
}

int DeviceKeyManager::loadCandidateKey(EcpPrivateKey& key) {
    if (hasCandidateKey_.has_value() && !hasCandidateKey_.value()) {
        return SYSTEM_ERROR_CRYPTO_KEY_NOT_FOUND;
    }
    fs::File f;
    int r = f.open(CANDIDATE_KEY_FILE, LFS_O_RDONLY);
    if (r < 0) {
        if (r == SYSTEM_ERROR_FILESYSTEM_NOENT) {
            hasCandidateKey_ = false;
            return SYSTEM_ERROR_CRYPTO_KEY_NOT_FOUND;
        }
        return r;
    }

    size_t privSize = CHECK(f.size());
    if (!privSize) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    if (privSize > MAX_DEVICE_PRIVATE_KEY_SIZE) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    std::unique_ptr<char[]> privData(new(std::nothrow) char[privSize]);
    if (!privData) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    size_t bytesRead = CHECK(f.read(privData.get(), privSize));
    if (bytesRead != privSize) {
        return SYSTEM_ERROR_BAD_DATA;
    }

    key = EcpPrivateKey(std::move(privData), privSize);
    hasCandidateKey_ = true;
    return 0;
}

} // namespace particle::system
