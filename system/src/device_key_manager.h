#pragma once

#include "dct.h"

#include <memory>
#include <optional>
#include <cstddef>

namespace particle::system {

const size_t MAX_DEVICE_PRIVATE_KEY_SIZE = DCT_ALT_DEVICE_PRIVATE_KEY_SIZE; // 192
const size_t MAX_DEVICE_PUBLIC_KEY_SIZE = DCT_ALT_DEVICE_PUBLIC_KEY_SIZE; // 128

// TODO: Move to an appropriate header
class CryptoKey {
public:
    enum class Type {
        INVALID = 0,
        PRIVATE = 1,
        PUBLIC = 2
    };

    enum class Algorithm {
        INVALID = 0,
        ECP = 1
    };

    CryptoKey() :
            type_(Type::INVALID),
            algo_(Algorithm::INVALID),
            size_(0) {
    }

    CryptoKey(Type type, Algorithm algo) :
            type_(type),
            algo_(algo),
            size_(0) {
    }

    CryptoKey(Type type, Algorithm algo, std::unique_ptr<char[]> data, size_t size) :
            data_(std::move(data)),
            type_(type),
            algo_(algo),
            size_(size) {
    }

    const char* data() const {
        return data_.get();
    }

    size_t size() const {
        return size_;
    }

    Type type() const {
        return type_;
    }

    Algorithm algorithm() const {
        return algo_;
    }

    bool isValid() const {
        return algo_ != Algorithm::INVALID && type_ != Type::INVALID && size_ > 0;
    }

private:
    std::unique_ptr<char[]> data_;
    Type type_;
    Algorithm algo_;
    size_t size_;
};

class EcpPublicKey: public CryptoKey {
public:
    EcpPublicKey() :
            CryptoKey(Type::PUBLIC, Algorithm::ECP) {
    }

    EcpPublicKey(std::unique_ptr<char[]> data, size_t size) :
            CryptoKey(Type::PUBLIC, Algorithm::ECP, std::move(data), size) {
    }
};

class EcpPrivateKey: public CryptoKey {
public:
    EcpPrivateKey() :
            CryptoKey(Type::PRIVATE, Algorithm::ECP) {
    }

    EcpPrivateKey(std::unique_ptr<char[]> data, size_t size) :
            CryptoKey(Type::PRIVATE, Algorithm::ECP, std::move(data), size) {
    }

    int getPublicKey(EcpPublicKey& pub) const;
};

/**
 * Device key manager.
 */
class DeviceKeyManager {
public:
    DeviceKeyManager() = default;

    /**
     * Store a candidate device key.
     *
     * The key is stored in the filesystem and will be loaded instead of the current device key next
     * time the device boots.
     *
     * @param key Private key.
     * @return 0 on success, otherwise an error code defined by `system_error_t`.
     */
    int setCandidateKey(const EcpPrivateKey& key);

    /**
     * Load the candidate device key.
     *
     * @param[out] key Private key.
     * @return 0 on success, `SYSTEM_ERROR_CRYPTO_KEY_NOT_FOUND` if there's no candidate key, or
     *         another error code defined by `system_error_t`.
     */
    int getCandidateKey(EcpPrivateKey& key);

    /**
     * Make the candidate device key the current device key.
     *
     * The key is written to the DCT and removed from the filesystem.
     *
     * @return 0 on success, `SYSTEM_ERROR_CRYPTO_KEY_NOT_FOUND` if there's no candidate key, or
     *         another error code defined by `system_error_t`.
     */
    int acceptCandidateKey();

    /**
     * Remove the candidate device key.
     *
     * @return 0 on success or if there's no candidate key, otherwise an error code defined by
     *         `system_error_t`.
     */
    int clearCandidateKey();

    /**
     * Generate a new device key.
     *
     * @param[out] key Private key.
     * @return 0 on success, otherwise an error code defined by `system_error_t`.
     */
    int generateKey(EcpPrivateKey& key);

    /**
     * Get the manager instance.
     *
     * @return Manager instance.
     */
    static DeviceKeyManager& instance();

private:
    std::optional<bool> hasCandidateKey_;

    int loadCandidateKey(EcpPrivateKey& key);
};

} // namespace particle::system
