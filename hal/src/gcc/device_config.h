/**
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#pragma once

#include <string>
#include <stdexcept>
#include <cstring>
#include <vector>
#include <optional>

#include "filesystem.h"
#include "spark_protocol_functions.h"
#include "module_info.h"

namespace particle {

namespace config {

class ModuleDependencyInfo {
public:
    ModuleDependencyInfo() :
            func_(MODULE_FUNCTION_NONE),
            index_(0),
            version_(0) {
    }

    ModuleDependencyInfo& function(module_function_t func) {
        func_ = func;
        return *this;
    }

    module_function_t function() const {
        return func_;
    }

    ModuleDependencyInfo& index(int index) {
        index_ = index;
        return *this;
    }

    int index() const {
        return index_;
    }

    ModuleDependencyInfo& version(int version) {
        version_ = version;
        return *this;
    }

    int version() const {
        return version_;
    }

private:
    module_function_t func_;
    int index_;
    int version_;
};

class ModuleInfo {
public:
    using Dependencies = std::vector<ModuleDependencyInfo>;

    ModuleInfo() :
            func_(MODULE_FUNCTION_NONE),
            storage_(MODULE_STORE_MAIN),
            maxSize_(0),
            index_(0),
            version_(0),
            validChecked_(0),
            validResult_(0) {
    }

    ModuleInfo& function(module_function_t func) {
        func_ = func;
        return *this;
    }

    module_function_t function() const {
        return func_;
    }

    ModuleInfo& index(int index) {
        index_ = index;
        return *this;
    }

    int index() const {
        return index_;
    }

    ModuleInfo& version(int version) {
        version_ = version;
        return *this;
    }

    int version() const {
        return version_;
    }

    ModuleInfo& dependencies(const Dependencies& deps) {
        deps_ = deps;
        return *this;
    }

    const Dependencies& dependencies() const {
        return deps_;
    }

    ModuleInfo& storage(module_store_t storage) {
        storage_ = storage;
        return *this;
    }

    module_store_t storage() const {
        return storage_;
    }

    ModuleInfo& maximumSize(size_t size) {
        maxSize_ = size;
        return *this;
    }

    size_t maximumSize() const {
        return maxSize_;
    }

    ModuleInfo& validityChecked(int flags) {
        validChecked_ = flags;
        return *this;
    }

    int validityChecked() const {
        return validChecked_;
    }

    ModuleInfo& validityResult(int flags) {
        validResult_ = flags;
        return *this;
    }

    int validityResult() const {
        return validResult_;
    }

    ModuleInfo& hash(std::string_view hash) {
        hash_ = hash;
        return *this;
    }

    const std::string& hash() const {
        return hash_;
    }

private:
    Dependencies deps_;
    std::string hash_;
    module_function_t func_;
    module_store_t storage_;
    size_t maxSize_;
    int index_;
    int version_;
    int validChecked_;
    int validResult_;
};

class Describe {
public:
    using Modules = std::vector<ModuleInfo>;

    Describe() = default;

    Describe& platformId(int platformId) {
        platformId_ = platformId;
        return *this;
    }

    int platformId() const {
        return platformId_.value_or(0);
    }

    Describe& modules(const Modules& modules) {
        modules_ = modules;
        return *this;
    }

    const Modules& modules() const {
        return modules_;
    }

    bool isValid() const {
        return platformId_.has_value();
    }

    std::string toString() const;
    static Describe fromString(std::string_view str);

private:
    Modules modules_;
    std::optional<int> platformId_;
};

} // namespace config

} // namespace particle

/**
 * Reads the device configuration and returns true if the device should start.
 * @param argc
 * @param argv
 * @return
 */
bool read_device_config(int argc, char* argv[]);

/**
 * The external configuration data.
 */
struct Configuration
{
    std::string device_id;
    std::string device_key;
    std::string server_key;
    std::string describe;
    uint16_t log_level = 0;
    ProtocolFactory protocol = PROTOCOL_LIGHTSSL;
    int platform_id;
};

/**
 * The device configuration in internal form.
 */
struct DeviceConfig
{
    particle::config::Describe describe;
    uint8_t device_id[12];
    uint8_t device_key[1024];
    uint8_t server_key[1024];
    ProtocolFactory protocol;
    int platform_id;

    void read(Configuration& configuration);

    size_t fetchDeviceID(uint8_t* dest, size_t destLen)
    {
        if (destLen>12)
            destLen = 12;
        if (dest)
            memcpy(dest, device_id, destLen);
        return 12;
    }

    ProtocolFactory get_protocol() { return protocol; }
};

extern DeviceConfig deviceConfig;
