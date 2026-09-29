#include <optional>
#include <cstddef>

namespace particle::system {

class DeviceKeyManager {
public:
    DeviceKeyManager() = default;

    int generateNewKey();
    int getNewKey(char* buf, size_t bufSize);
    int applyNewKey();
    int clearNewKey();

    static DeviceKeyManager& instance();

private:
    std::optional<bool> hasNewKey_;
};

} // namespace particle::system
