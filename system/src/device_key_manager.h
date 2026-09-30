#include "filesystem.h"
#include "dct.h"

#include <optional>
#include <cstddef>

namespace particle::system {

const size_t MAX_DEVICE_PRIVATE_KEY_SIZE = DCT_ALT_DEVICE_PRIVATE_KEY_SIZE; // 192
const size_t MAX_DEVICE_PUBLIC_KEY_SIZE = DCT_ALT_DEVICE_PUBLIC_KEY_SIZE; // 128

class DeviceKeyManager {
public:
    DeviceKeyManager() = default;

    int generateNewKey();
    int getNewPrivateKey(char* buf, size_t bufSize);
    int getNewPublicKey(char* buf, size_t bufSize);
    int applyNewKey();
    int clearNewKey();

    static DeviceKeyManager& instance();

private:
    std::optional<bool> hasNewKey_;

    int openKeyFile(fs::File& file);
};

} // namespace particle::system
